/* app.c - ThreadX application entry for the STINGER altimeter template.
 *
 * tx_kernel_enter() (called from main) calls tx_application_define() once,
 * then starts the scheduler.
 *
 * Rule: tx_application_define() only CREATES things (threads, queues,
 * mutexes). Anything that waits or talks to hardware with delays goes at the
 * top of a thread's entry function, because the scheduler isn't running yet
 * and tx_thread_sleep() returns immediately there.
 * 
 * Authors: Ari Klein, Andrew Gault
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_user.h>
#include <math.h>

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

#define BARO_STACK_BYTES 1024U
#define BARO_PRIORITY 10U

#define ALTITUDE_STACK_BYTES 1024U
#define ALTITUDE_PRIORITY 10U

#define HEARTBEAT_STACK_BYTES 1024U
#define HEARTBEAT_PRIORITY 5U

static TX_THREAD imu_thread;
static TX_THREAD baro_thread;
static TX_THREAD altitude_thread;
static TX_THREAD heartbeat_thread;
/* Static stack: its size shows up in the .map file, unlike a byte pool. */
static ULONG imu_stack[IMU_STACK_BYTES / sizeof(ULONG)];
static ULONG heartbeat_stack[HEARTBEAT_STACK_BYTES / sizeof(ULONG)];
static ULONG baro_stack[BARO_STACK_BYTES / sizeof(ULONG)];
static ULONG altitude_stack[ALTITUDE_STACK_BYTES / sizeof(ULONG)];

static ms5611_t baro; // Barometer (Pressure and Temperature)
static mpu6050_t imu; // Inertial Measurement Unit (Gyroscope and Accelerometer).

// Mutexed rocket state - DO NOT CHANGE VALUES UNDER MUTEX UNLESS MUTEX IS AQUIRED:
static uint8_t rocket_state;
static TX_MUTEX rocket_state_mutex;

// Mutexed current pressure:
static LONG pressure;
// static TX_MUTEX pressure_mutex;

// Mutexed current temperature:
static LONG temperature;
// static TX_MUTEX temperature_mutex;

// Other Mutexes
static TX_MUTEX hal_transmit;

// Initial values before launch:
static LONG initial_pressure;
static LONG initial_temperature;

static void dbg(const char *fmt, ...) {
  char buf[96];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  if (n > 0) {
    tx_mutex_get(&hal_transmit, TX_WAIT_FOREVER); // Only grab mutex exactly when needed (i.e. ready to transmit.)
    HAL_UART_Transmit(
        &huart1, (uint8_t *)buf,
        (uint16_t)((n < (int)sizeof buf) ? n : (int)sizeof buf - 1), 100U);
    tx_mutex_put(&hal_transmit);
  }
}

// Helper Functions:

/**
 * @brief Calculates and returns altitude with the formula:
 * altitude = (t0/0.0065)*(1 - (p/p0)^(0.190266)).
 * 
 * @param p0 The initial pressure on ground at time of launch, in Pascals.
 * @param t0 The initial temperature on ground at time of launch, in Kelvin.
 * @param p The pressure in Pascals to calculate altitude at.
 * 
 * @return The calculated altitude.
 */
static LONG calc_altitude(const LONG* p0, const LONG* t0, const LONG* p) {
  // TODO: Test precision, change casting and/or order of operations if necessary.
  return (LONG)((*t0 / 0.0065) * (1 - pow(((LONG)(*p / *p0)), (0.190266))));
}

// Thread Entries:

static void imu_entry(ULONG input) {
  (void)input;

  dbg("STINGER: ThreadX running IMU\r\n");

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

static void baro_entry(ULONG input) {
  (void)input;

  dbg("STINGER: ThreadX running Barometer\r\n");

  ms5611_status_t status =
      ms5611_init(&baro, &hi2c2, MS5611_ADDR_CSB_HIGH, MS5611_OSR_4096);
  if (status != MS5611_OK) {
    dbg("Barometer: init failed: %s\r\n", ms5611_status_str(status));
  }

  for (;;) {
    ms5611_sample_t data;
    status = ms5611_read(&baro, &data);
    if (status == MS5611_OK) {
      dbg("Barometer: Pressure = %d; Temperature = %d\r\n", data.pressure_pa, data.temp_c100); // *** Test states? (If in testing, outputs these values, else will not include in code.)
      if (/*Initial pressure or temperature not assigned and rocket_state == 0*/) { // TODO: Complete. ***
        initial_pressure = data.pressure_pa;
        initial_temperature = data.temp_c100;
      } else {
        pressure = data.pressure_pa;
        temperature = data.temp_c100;
      }
    } else {
      dbg("Barometer: read error: %s\r\n", ms5611_status_str(status));
    }

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 0.01); /* 10 ms: 100 Hz */
  }
}

static void altitude_entry(ULONG input) {
  (void)input;

  dbg("STINGER: ThreadX running Altitude\r\n");

  for (;;) {

    // Possibility of pressure and temperature being grabbed at slightly different times
    // (temperature from time t = (A) ms, pressure from t = (A+10) ms. However, this is a small
    // enough interval to not really matter. Potential fix slows down system.)
    // Just reading data so unless above problem matters, no need to grab mutexes.
    ULONG current_pressure = pressure;
    ULONG altitude = calc_altitude(&initial_pressure, &initial_temperature, &current_pressure);
    
    dbg("Altitude: Altitude = %d", altitude);

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 0.01); /* 10 ms: 100 Hz */
  }
}

static ULONG heartbeat_entry(ULONG input) {
  (void)input;

  for (;;) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 0.5); /* 500 ms */
  }
}

void tx_application_define(void *first_unused_memory) {
  (void)first_unused_memory;

  // Create mutexes:
  tx_mutex_create(&rocket_state_mutex, "Rocket state", TX_INHERIT); // *** Change to TX_NO_INHERIT if design does not need mutex priorities.
  tx_mutex_create(&hal_transmit, "HAL Transmit", TX_NO_INHERIT);
  // tx_mutex_create(&pressure_mutex, "Pressure", TX_INHERIT); // *** Change to TX_NO_INHERIT if design does not need mutex priorities.
  // tx_mutex_create(&temperature_mutex, "Temperature", TX_INHERIT); // *** Change to TX_NO_INHERIT if design does not need mutex priorities.

  // Create IMU read loop, Barometer read loop, and heartbeat threads:
  (void)tx_thread_create(&imu_thread, "IMU Read Loop", imu_entry, 0U, imu_stack,
                         sizeof(imu_stack), IMU_PRIORITY, IMU_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);

  (void)tx_thread_create(&baro_thread, "Barometer Read Loop", baro_entry, 0U, baro_stack,
                         sizeof(baro_stack), BARO_PRIORITY, BARO_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
  
  (void)tx_thread_create(&altitude_thread, "Altitude Calculations", altitude_entry, 0U,
                         heartbeat_stack, sizeof(heartbeat_stack),
                         HEARTBEAT_PRIORITY, HEARTBEAT_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);

  (void)tx_thread_create(&heartbeat_thread, "Heartbeat", heartbeat_entry, 0U,
                         heartbeat_stack, sizeof(heartbeat_stack),
                         HEARTBEAT_PRIORITY, HEARTBEAT_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START);
}

/*
  Questions:
    - What is st from "st = mpu6050_read(&imu, &sample);"? (As in STM32? Status? <- Assuming status.)
    - A lot of these acronyms need to have either a comment stating what they stand for
    at their first occurance, or have longer, more descriptive names (good practice).
    - Does ThreadX have a dynamic priority system in the form of increased priority for how long a task waits?
*/
