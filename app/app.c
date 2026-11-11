/* app.c - ThreadX application entry for the STINGER altimeter template.
 *
 * tx_kernel_enter() (called from main) calls tx_application_define() once,
 * then starts the scheduler.
 *
 * Rule: tx_application_define() only CREATES things (threads, queues,
 * mutexes). Anything that waits or talks to hardware with delays goes at the
 * top of a thread's entry function, because the scheduler isn't running yet
 * and tx_thread_sleep() returns immediately there.
 */

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_user.h>

#include "mpu6050.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include "tx_api.h"

#include "i2c.h"   /* hi2c2 */
#include "main.h"  /* LED_Pin, LED_GPIO_Port from the CubeMX user label */
#include "usart.h" /* huart1 */

#include "altitude.h"
#include "ms5611.h"

#define IMU_STACK_BYTES 1024U
#define IMU_PRIORITY 10U

#define HEARTBEAT_STACK_BYTES 1024U
#define HEARTBEAT_PRIORITY 5U

/* Above the IMU: altitude is what decides parachute deployment. */
#define BARO_STACK_BYTES 1024U
#define BARO_PRIORITY 9U

/* Readings averaged at startup for the ground reference. Each read takes
 * about 22 ms at OSR 4096, so this is roughly one second. */
#define BARO_GROUND_SAMPLES 32U

#define BARO_I2C_ADDR                                                          \
  MS5611_ADDR_CSB_HIGH /* STINGER V2.0: MS5611 CSB tied high */

static TX_THREAD imu_thread;
static TX_THREAD heartbeat_thread;
static TX_THREAD baro_thread;
/* Static stack: its size shows up in the .map file, unlike a byte pool. */
static ULONG imu_stack[IMU_STACK_BYTES / sizeof(ULONG)];
static ULONG heartbeat_stack[HEARTBEAT_STACK_BYTES / sizeof(ULONG)];
static ULONG baro_stack[BARO_STACK_BYTES / sizeof(ULONG)];

/* The HAL I2C and UART handles aren't thread-safe. If a thread is preempted
 * in the middle of a transfer and another thread starts one on the same
 * handle, the second gets HAL_BUSY and fails. One mutex per shared resource;
 * TX_INHERIT lends the holder the waiter's priority so a low-priority
 * holder can't hold up a high-priority waiter (priority inversion). */
static TX_MUTEX i2c_mutex;  /* hi2c2: MS5611 and MPU6050 */
static TX_MUTEX uart_mutex; /* huart1: everything printed by dbg() */

static ms5611_t baro;
static mpu6050_t imu;

static void dbg(const char *fmt, ...) {
  char buf[96];
  (void)tx_mutex_get(&uart_mutex, TX_WAIT_FOREVER); // makes threads take turns using serial port
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  if (n > 0) {
    HAL_UART_Transmit(
        &huart1, (uint8_t *)buf,
        (uint16_t)((n < (int)sizeof buf) ? n : (int)sizeof buf - 1), 100U);
  }
  (void)tx_mutex_put(&uart_mutex);
}

/* Hold the bus for a whole driver call, so no other thread's transfer can
 * land between its I2C transactions. */
static void i2c_lock(void) { (void)tx_mutex_get(&i2c_mutex, TX_WAIT_FOREVER); }
static void i2c_unlock(void) { (void)tx_mutex_put(&i2c_mutex); }

static void imu_entry(ULONG input) {
  (void)input;

  dbg("STINGER: ThreadX running\r\n");

  i2c_lock();
  mpu6050_status_t st =
      mpu6050_init(&imu, &hi2c2, MPU6050_ADDR_AD0_HIGH, MPU6050_ACCEL_16G,
                   MPU6050_GYRO_500DPS, MPU6050_RATE_200HZ, MPU6050_DLPF_42HZ);
  i2c_unlock();
  if (st != MPU6050_OK) {
    dbg("IMU: init failed: %s\r\n", mpu6050_status_str(st));
  }

  for (;;) {
    mpu6050_sample_t sample;
    i2c_lock();
    st = mpu6050_read(&imu, &sample);
    i2c_unlock();
    if (st == MPU6050_OK) {
      dbg("IMU: ax=%d ay=%d az=%d temp=%d gx=%ld gy=%ld gz=%ld\r\n",
          sample.accel_mg[0], sample.accel_mg[1], sample.accel_mg[2],
          sample.temp_c100, (long)sample.gyro_mdps[0],
          (long)sample.gyro_mdps[1], (long)sample.gyro_mdps[2]);
    } else {
      dbg("IMU: read error: %s\r\n", mpu6050_status_str(st));
    }

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 0.01); /* 10 ms: 100 Hz */
  }
}

static ms5611_status_t baro_read(ms5611_sample_t *s) {
  i2c_lock();
  ms5611_status_t st = ms5611_read(&baro, s);
  i2c_unlock();
  return st;
}

static void baro_entry(ULONG input) {
  (void)input;

  /* Altitude is meaningless without the sensor, so keep retrying. */
  ms5611_status_t st;
  for (;;) {
    i2c_lock();
    st = ms5611_init(&baro, &hi2c2, BARO_I2C_ADDR, MS5611_OSR_4096);
    i2c_unlock();
    if (st == MS5611_OK) {
      break;
    }
    dbg("BARO: init failed: %s\r\n", ms5611_status_str(st));
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND); /* retry in 1 s */
  }

  /* Ground reference P0 and T0: one reading is noisy, so average a batch. */
  int32_t p_sum = 0;
  int32_t t_sum = 0;
  uint32_t n = 0;
  while (n < BARO_GROUND_SAMPLES) {
    ms5611_sample_t s;
    st = baro_read(&s);
    if (st == MS5611_OK) {
      p_sum += s.pressure_pa;
      t_sum += s.temp_c100;
      n++;
    } else {
      dbg("BARO: read error: %s\r\n", ms5611_status_str(st));
    }
    /* Sleep even on errors: a failing bus returns at once, and without a
     * sleep this loop would starve every lower-priority thread. */
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 100U); /* 10 ms */
  }
  const float p0_pa = (float)p_sum / (float)BARO_GROUND_SAMPLES;
  const float t0_k =
      altitude_c100_to_kelvin((float)t_sum / (float)BARO_GROUND_SAMPLES);
  dbg("BARO: ground P0=%ld Pa T0=%ld cK\r\n", lroundf(p0_pa),
      lroundf(t0_k * 100.0f));

  for (;;) {
    ms5611_sample_t s;
    st = baro_read(&s);
    if (st == MS5611_OK) {
      /* No %f on this board (newlib-nano), so print whole centimeters. */
      const float h_m = altitude_m((float)s.pressure_pa, p0_pa, t0_k);
      dbg("BARO: p=%ld Pa t=%ld alt=%ld cm\r\n", (long)s.pressure_pa,
          (long)s.temp_c100, lroundf(h_m * 100.0f));
    } else {
      dbg("BARO: read error: %s\r\n", ms5611_status_str(st));
    }

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 20U); /* 50 ms */
  }
}

static void heartbeat_entry(ULONG input) {
  (void)input;

  for (;;) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 0.25); /* 250 ms */
  }
}

void tx_application_define(void *first_unused_memory) {
  (void)first_unused_memory;

  // Mutexes first: the threads below use them as soon as they start
  (void)tx_mutex_create(&i2c_mutex, "I2C2 bus", TX_INHERIT);
  (void)tx_mutex_create(&uart_mutex, "USART1 serial", TX_INHERIT);

  // Create IMU read loop, barometer and heartbeat threads
  (void)tx_thread_create(&imu_thread, "IMU Read Loop", imu_entry, 0U, imu_stack,
                         sizeof(imu_stack), IMU_PRIORITY, IMU_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
  (void)tx_thread_create(&baro_thread, "Baro Read Loop", baro_entry, 0U,
                         baro_stack, sizeof(baro_stack), BARO_PRIORITY,
                         BARO_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
  (void)tx_thread_create(&heartbeat_thread, "Heartbeat", heartbeat_entry, 0U,
                         heartbeat_stack, sizeof(heartbeat_stack),
                         HEARTBEAT_PRIORITY, HEARTBEAT_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
}
