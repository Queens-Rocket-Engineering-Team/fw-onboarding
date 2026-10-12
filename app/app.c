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

#include "ms5611.h"

#define IMU_STACK_BYTES 1024U
#define IMU_PRIORITY 10U

#define HEARTBEAT_STACK_BYTES 1024U
#define HEARTBEAT_PRIORITY 5U

#define BARO_I2C_ADDR                                                          \
  MS5611_ADDR_CSB_HIGH /* STINGER V2.0: MS5611 CSB tied high */

static TX_THREAD imu_thread;
static TX_THREAD heartbeat_thread;
/* Static stack: its size shows up in the .map file, unlike a byte pool. */
static ULONG imu_stack[IMU_STACK_BYTES / sizeof(ULONG)];
static ULONG heartbeat_stack[HEARTBEAT_STACK_BYTES / sizeof(ULONG)];

// static ms5611_t baro;
static mpu6050_t imu;

static void dbg(const char *fmt, ...) {
  char buf[96];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  if (n > 0) {
    HAL_UART_Transmit(
        &huart1, (uint8_t *)buf,
        (uint16_t)((n < (int)sizeof buf) ? n : (int)sizeof buf - 1), 100U);
  }
}

static void imu_entry(ULONG input) {
  (void)input;

  dbg("STINGER: ThreadX running\r\n");

  mpu6050_status_t st =
      mpu6050_init(&imu, &hi2c2, MPU6050_ADDR_AD0_HIGH, MPU6050_ACCEL_16G,
                   MPU6050_GYRO_500DPS, MPU6050_RATE_200HZ, MPU6050_DLPF_42HZ);
  if (st != MPU6050_OK) {
    dbg("IMU: init failed: %s\r\n", mpu6050_status_str(st));
  }

  for (;;) {
    mpu6050_sample_t sample;
    st = mpu6050_read(&imu, &sample);
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

static void heartbeat_entry(ULONG input) {
  (void)input;

  for (;;) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 0.5); /* 500 ms */
  }
}

void tx_application_define(void *first_unused_memory) {
  (void)first_unused_memory;

  // Create IMU read loop and heartbeat threads
  (void)tx_thread_create(&imu_thread, "IMU Read Loop", imu_entry, 0U, imu_stack,
                         sizeof(imu_stack), IMU_PRIORITY, IMU_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
  (void)tx_thread_create(&heartbeat_thread, "Heartbeat", heartbeat_entry, 0U,
                         heartbeat_stack, sizeof(heartbeat_stack),
                         HEARTBEAT_PRIORITY, HEARTBEAT_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
}