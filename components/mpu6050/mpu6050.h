/**
 * @file mpu6050.h
 * @brief Driver for the InvenSense MPU6050 6-axis IMU (I2C).
 *
 * References: MPU6050 datasheet
 *             MPU-6000/MPU-6050 Register Map and Descriptions, Revision 3.2,
 * 2011-11-14
 *
 * Usage:
 * @code
 *   static mpu6050_t imu;
 *   mpu6050_status_t st = mpu6050_init(&imu, &hi2c2, MPU6050_ADDR_AD0_HIGH,
 *                                      MPU6050_ACCEL_2G, MPU6050_GYRO_250DPS,
 *                                      MPU6050_RATE_200HZ, MPU6050_DLPF_42HZ);
 *   if (st != MPU6050_OK) { log mpu6050_status_str(st); }
 *
 *   mpu6050_sample_t s;
 *   if (mpu6050_read(&imu, &s) == MPU6050_OK) {
 *     use s.accel_mg[0..2], s.gyro_mdps[0..2], s.temp_c100
 *   }
 * @endcode
 *
 *
 * The conversion math lives in mpu6050_calc.h, separate from the I/O.
 */
#ifndef MPU6050_H
#define MPU6050_H

#include "mpu6050_calc.h"

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/** Result of every driver call. MPU6050_OK is 0, so `if (st)` means failure. */
typedef enum {
  MPU6050_OK = 0,
  MPU6050_ERR_ARG, /**< NULL pointer, invalid address, range, rate or filter */
  MPU6050_ERR_BUS, /**< I2C transfer failed (NACK, timeout); see last_bus_error
                    */
  MPU6050_ERR_ID,  /**< WHO_AM_I did not read 0x68: wrong or absent device */
  MPU6050_ERR_NOT_INIT, /**< mpu6050_read() before a successful mpu6050_init()
                         */
} mpu6050_status_t;

/** 7-bit I2C address, selected by the AD0 pin. */
typedef enum {
  MPU6050_ADDR_AD0_LOW = 0x68,
  MPU6050_ADDR_AD0_HIGH = 0x69,
} mpu6050_addr_t;

/** Low-pass filter bandwidth. Value is DLPF_CFG (0 and 7: filter off). */
typedef enum {
  MPU6050_DLPF_188HZ = 1,
  MPU6050_DLPF_98HZ = 2,
  MPU6050_DLPF_42HZ = 3,
  MPU6050_DLPF_20HZ = 4,
  MPU6050_DLPF_10HZ = 5,
  MPU6050_DLPF_5HZ = 6,
} mpu6050_dlpf_t;

/** Output data rate. Value is SMPLRT_DIV: rate = 1 kHz / (1 + value). */
typedef enum {
  MPU6050_RATE_1000HZ = 0,
  MPU6050_RATE_500HZ = 1,
  MPU6050_RATE_250HZ = 3,
  MPU6050_RATE_200HZ = 4,
  MPU6050_RATE_125HZ = 7,
  MPU6050_RATE_100HZ = 9,
  MPU6050_RATE_50HZ = 19,
} mpu6050_rate_t;

/** Device state. Treat the fields as private: only the driver writes them. */
typedef struct {
  I2C_HandleTypeDef *hi2c;           /**< Bus the chip is on */
  mpu6050_addr_t addr;               /**< Set by the board's AD0 pin */
  mpu6050_accel_range_t accel_range; /**< Accelerometer full-scale range */
  mpu6050_gyro_range_t gyro_range;   /**< Gyroscope full-scale range */
  mpu6050_rate_t rate;               /**< Output data rate */
  mpu6050_dlpf_t dlpf;               /**< Low-pass filter bandwidth */
  bool initialized;                  /**< True after a successful init */
  uint32_t
      last_bus_error; /**< HAL_I2C_GetError() flags from the last bus failure */
} mpu6050_t;

/**
 * @brief Check the chip identity, reset it and apply the configuration.
 *
 * Blocks for 160 to 300 ms depending on the filter. Call only from a thread.
 */
mpu6050_status_t mpu6050_init(mpu6050_t *dev, I2C_HandleTypeDef *hi2c,
                              mpu6050_addr_t addr,
                              mpu6050_accel_range_t accel_range,
                              mpu6050_gyro_range_t gyro_range,
                              mpu6050_rate_t rate, mpu6050_dlpf_t dlpf);

/** @brief Read acceleration, temperature and angular rate in one burst. */
mpu6050_status_t mpu6050_read(mpu6050_t *dev, mpu6050_sample_t *sample);

/** @brief Human-readable name for a status code, for log messages. */
const char *mpu6050_status_str(mpu6050_status_t status);

#endif /* MPU6050_H */
