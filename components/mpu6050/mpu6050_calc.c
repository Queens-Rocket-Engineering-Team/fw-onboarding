#include "mpu6050_calc.h"

const mpu6050_accel_range_info_t accel_range_info[] = {
    [MPU6050_ACCEL_2G] = {.bits = 0x00, .counts_per_g = 16384}, /* ±2g */
    [MPU6050_ACCEL_4G] = {.bits = 0x08, .counts_per_g = 8192},  /* ±4g */
    [MPU6050_ACCEL_8G] = {.bits = 0x10, .counts_per_g = 4096},  /* ±8g */
    [MPU6050_ACCEL_16G] = {.bits = 0x18, .counts_per_g = 2048}, /* ±16g */
};

const mpu6050_gyro_range_info_t gyro_range_info[] = {
    [MPU6050_GYRO_250DPS] = {.bits = 0x00,
                             .counts_per_dps_x10 = 1310}, /* ±250°/s */
    [MPU6050_GYRO_500DPS] = {.bits = 0x08,
                             .counts_per_dps_x10 = 655}, /* ±500°/s */
    [MPU6050_GYRO_1000DPS] = {.bits = 0x10,
                              .counts_per_dps_x10 = 328}, /* ±1000°/s */
    [MPU6050_GYRO_2000DPS] = {.bits = 0x18,
                              .counts_per_dps_x10 = 164}, /* ±2000°/s */
};

void mpu6050_convert_raw_to_physical(int16_t ax, int16_t ay, int16_t az,
                                     int16_t temp, int16_t gx, int16_t gy,
                                     int16_t gz,
                                     mpu6050_accel_range_t accel_range,
                                     mpu6050_gyro_range_t gyro_range,
                                     mpu6050_sample_t *sample) {
  const int32_t accel_counts_per_g = accel_range_info[accel_range].counts_per_g;
  const int32_t gyro_counts_per_dps_x10 =
      gyro_range_info[gyro_range].counts_per_dps_x10;

  /* Raw accelerometer counts to milli-g */
  sample->accel_mg[0] = (int16_t)((ax * 1000) / accel_counts_per_g);
  sample->accel_mg[1] = (int16_t)((ay * 1000) / accel_counts_per_g);
  sample->accel_mg[2] = (int16_t)((az * 1000) / accel_counts_per_g);

  /* Raw gyroscope counts to milli-deg/s */
  sample->gyro_mdps[0] = (gx * 10000) / gyro_counts_per_dps_x10;
  sample->gyro_mdps[1] = (gy * 10000) / gyro_counts_per_dps_x10;
  sample->gyro_mdps[2] = (gz * 10000) / gyro_counts_per_dps_x10;

  sample->temp_c100 = (int16_t)(temp * 100 / 340 + 3652); /* 0.01 degC */
}
