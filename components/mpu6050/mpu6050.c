#include "mpu6050.h"

#include <stddef.h>

#include "tx_api.h"

/* Registers (datasheet "Register Map") */
#define MPU6050_REG_SMPLRT_DIV 0x19U
#define MPU6050_REG_CONFIG 0x1AU
#define MPU6050_REG_GYRO_CONFIG 0x1BU
#define MPU6050_REG_ACCEL_CONFIG 0x1CU
#define MPU6050_REG_ACCEL_XOUT_H 0x3BU /* first of 14 sequential data bytes */
#define MPU6050_REG_PWR_MGMT_1 0x6BU
#define MPU6050_REG_WHO_AM_I 0x75U

/* WHO_AM_I register value for the MPU6050, does not include the AD0 pin */
#define MPU6050_WHO_AM_I_VALUE 0x68U

/* PWR_MGMT_1 values */
#define MPU6050_PWR_RESET 0x80U /* DEVICE_RESET */
#define MPU6050_PWR_WAKE 0x01U  /* clear SLEEP, clock from the X gyro PLL */

/* Timing: the datasheet gives 100 ms start-up for register read/write. */
#define MPU6050_RESET_MS 100U
#define MPU6050_I2C_TIMEOUT_MS 10U

/* Output settling time after configuration, indexed by mpu6050_dlpf_t. */
static const uint8_t settle_ms[] = {
    [MPU6050_DLPF_188HZ] = 60, [MPU6050_DLPF_98HZ] = 70,
    [MPU6050_DLPF_42HZ] = 80,  [MPU6050_DLPF_20HZ] = 130,
    [MPU6050_DLPF_10HZ] = 150, [MPU6050_DLPF_5HZ] = 200,
};

static ULONG ms_to_ticks(uint32_t ms) {
  return (ULONG)((ms * TX_TIMER_TICKS_PER_SECOND + 999U) / 1000U) + 1U;
}

static bool addr_valid(mpu6050_addr_t addr) {
  return addr == MPU6050_ADDR_AD0_LOW || addr == MPU6050_ADDR_AD0_HIGH;
}

static bool rate_valid(mpu6050_rate_t rate) {
  switch (rate) {
  case MPU6050_RATE_1000HZ:
  case MPU6050_RATE_500HZ:
  case MPU6050_RATE_250HZ:
  case MPU6050_RATE_200HZ:
  case MPU6050_RATE_125HZ:
  case MPU6050_RATE_100HZ:
  case MPU6050_RATE_50HZ:
    return true;
  }
  return false;
}

static mpu6050_status_t bus_failed(mpu6050_t *dev) {
  dev->last_bus_error = HAL_I2C_GetError(dev->hi2c);
  return MPU6050_ERR_BUS;
}

static mpu6050_status_t write_reg(mpu6050_t *dev, uint8_t reg, uint8_t val) {
  if (HAL_I2C_Mem_Write(dev->hi2c, (uint16_t)(dev->addr << 1), reg,
                        I2C_MEMADD_SIZE_8BIT, &val, 1,
                        MPU6050_I2C_TIMEOUT_MS) != HAL_OK) {
    return bus_failed(dev);
  }
  return MPU6050_OK;
}

static mpu6050_status_t read_regs(mpu6050_t *dev, uint8_t reg, uint8_t *buf,
                                  uint16_t len) {
  if (HAL_I2C_Mem_Read(dev->hi2c, (uint16_t)(dev->addr << 1), reg,
                       I2C_MEMADD_SIZE_8BIT, buf, len,
                       MPU6050_I2C_TIMEOUT_MS) != HAL_OK) {
    return bus_failed(dev);
  }
  return MPU6050_OK;
}

mpu6050_status_t mpu6050_init(mpu6050_t *dev, I2C_HandleTypeDef *hi2c,
                              mpu6050_addr_t addr,
                              mpu6050_accel_range_t accel_range,
                              mpu6050_gyro_range_t gyro_range,
                              mpu6050_rate_t rate, mpu6050_dlpf_t dlpf) {
  if (dev == NULL || hi2c == NULL || !addr_valid(addr) ||
      (unsigned)accel_range > MPU6050_ACCEL_16G ||
      (unsigned)gyro_range > MPU6050_GYRO_2000DPS || !rate_valid(rate) ||
      (unsigned)dlpf < MPU6050_DLPF_188HZ ||
      (unsigned)dlpf > MPU6050_DLPF_5HZ) {
    return MPU6050_ERR_ARG;
  }

  dev->hi2c = hi2c;
  dev->addr = addr;
  dev->accel_range = accel_range;
  dev->gyro_range = gyro_range;
  dev->rate = rate;
  dev->dlpf = dlpf;
  dev->initialized = false;
  dev->last_bus_error = 0;

  /* Confirm the right chip answers at this address before writing to it. */
  uint8_t who = 0;
  mpu6050_status_t st = read_regs(dev, MPU6050_REG_WHO_AM_I, &who, 1);
  if (st != MPU6050_OK) {
    return st;
  }
  if (who != MPU6050_WHO_AM_I_VALUE) {
    return MPU6050_ERR_ID;
  }

  /* Reset, then wait out the start-up time. */
  st = write_reg(dev, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_RESET);
  if (st != MPU6050_OK) {
    return st;
  }
  tx_thread_sleep(ms_to_ticks(MPU6050_RESET_MS));

  st = write_reg(dev, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_WAKE);
  if (st != MPU6050_OK) {
    return st;
  }

  /* With the DLPF on, the gyro output rate is 1 kHz. */
  st = write_reg(dev, MPU6050_REG_CONFIG, (uint8_t)dlpf);
  if (st != MPU6050_OK) {
    return st;
  }

  st = write_reg(dev, MPU6050_REG_SMPLRT_DIV, (uint8_t)rate);
  if (st != MPU6050_OK) {
    return st;
  }

  st =
      write_reg(dev, MPU6050_REG_GYRO_CONFIG, gyro_range_info[gyro_range].bits);
  if (st != MPU6050_OK) {
    return st;
  }

  st = write_reg(dev, MPU6050_REG_ACCEL_CONFIG,
                 accel_range_info[accel_range].bits);
  if (st != MPU6050_OK) {
    return st;
  }

  /* Wait out the start-up transient so the first read is already valid. */
  tx_thread_sleep(ms_to_ticks(settle_ms[dlpf]));

  dev->initialized = true;
  return MPU6050_OK;
}

mpu6050_status_t mpu6050_read(mpu6050_t *dev, mpu6050_sample_t *sample) {
  if (dev == NULL || sample == NULL) {
    return MPU6050_ERR_ARG;
  }
  if (!dev->initialized) {
    return MPU6050_ERR_NOT_INIT;
  }

  /* Accel, temperature and gyro come out in one 14-byte burst. */
  uint8_t buf[14];
  mpu6050_status_t st =
      read_regs(dev, MPU6050_REG_ACCEL_XOUT_H, buf, sizeof buf);
  if (st != MPU6050_OK) {
    return st;
  }

  int16_t ax_raw, ay_raw, az_raw, temp_raw, gx_raw, gy_raw, gz_raw;
  ax_raw = (int16_t)((buf[0] << 8) | buf[1]);
  ay_raw = (int16_t)((buf[2] << 8) | buf[3]);
  az_raw = (int16_t)((buf[4] << 8) | buf[5]);
  temp_raw = (int16_t)((buf[6] << 8) | buf[7]);
  gx_raw = (int16_t)((buf[8] << 8) | buf[9]);
  gy_raw = (int16_t)((buf[10] << 8) | buf[11]);
  gz_raw = (int16_t)((buf[12] << 8) | buf[13]);

  mpu6050_convert_raw_to_physical(ax_raw, ay_raw, az_raw, temp_raw, gx_raw,
                                  gy_raw, gz_raw, dev->accel_range,
                                  dev->gyro_range, sample);

  return MPU6050_OK;
}

const char *mpu6050_status_str(mpu6050_status_t status) {
  switch (status) {
  case MPU6050_OK:
    return "ok";
  case MPU6050_ERR_ARG:
    return "invalid argument";
  case MPU6050_ERR_BUS:
    return "I2C bus error";
  case MPU6050_ERR_ID:
    return "WHO_AM_I mismatch";
  case MPU6050_ERR_NOT_INIT:
    return "not initialized";
  }
  return "unknown";
}
