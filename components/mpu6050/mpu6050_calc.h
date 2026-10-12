/**
 * @file mpu6050_calc.h
 * @brief MPU6050 conversion math and range info, no hardware interaction
 *
 * References: MPU6050 datasheet
 *             MPU-6000/MPU-6050 Register Map and Descriptions, Revision 3.2,
 * 2011-11-14
 */
#ifndef MPU6050_CALC_H
#define MPU6050_CALC_H

#include <stdint.h>

/** Accelerometer full-scale range. */
typedef enum {
  MPU6050_ACCEL_2G,
  MPU6050_ACCEL_4G,
  MPU6050_ACCEL_8G,
  MPU6050_ACCEL_16G
} mpu6050_accel_range_t;

/** Register setting and sensitivity for one accelerometer range. */
typedef struct {
  uint8_t bits;         /**< ACCEL_CONFIG value selecting this range */
  int32_t counts_per_g; /**< LSB per g */
} mpu6050_accel_range_info_t;

extern const mpu6050_accel_range_info_t accel_range_info[4];

/** Gyroscope full-scale range. */
typedef enum {
  MPU6050_GYRO_250DPS,
  MPU6050_GYRO_500DPS,
  MPU6050_GYRO_1000DPS,
  MPU6050_GYRO_2000DPS
} mpu6050_gyro_range_t;

/** Register setting and sensitivity for one gyroscope range. */
typedef struct {
  uint8_t bits;               /**< GYRO_CONFIG value selecting this range */
  int32_t counts_per_dps_x10; /**< LSB per deg/s, x10 to keep the fraction */
} mpu6050_gyro_range_info_t;

extern const mpu6050_gyro_range_info_t gyro_range_info[4];

/** One converted reading. Axes are X, Y, Z. */
typedef struct {
  int16_t accel_mg[3];  /**< Acceleration, milli-g */
  int32_t gyro_mdps[3]; /**< Angular rate, milli-deg/s */
  int16_t temp_c100;    /**< Temperature, 0.01 degC (2554 = 25.54 degC) */
} mpu6050_sample_t;

/** @brief Convert raw register counts to physical units. */
void mpu6050_convert_raw_to_physical(int16_t ax, int16_t ay, int16_t az,
                                     int16_t temp, int16_t gx, int16_t gy,
                                     int16_t gz,
                                     mpu6050_accel_range_t accel_range,
                                     mpu6050_gyro_range_t gyro_range,
                                     mpu6050_sample_t *sample);

#endif /* MPU6050_CALC_H */
