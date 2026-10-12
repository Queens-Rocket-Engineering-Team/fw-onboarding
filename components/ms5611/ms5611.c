#include "ms5611.h"

#include <stddef.h>

#include "tx_api.h"

/* Commands (datasheet "Commands" table). Convert commands are ORed with the
 * OSR bits from ms5611_osr_t. */
#define MS5611_CMD_RESET 0x1EU
#define MS5611_CMD_CONVERT_D1 0x40U /* pressure */
#define MS5611_CMD_CONVERT_D2 0x50U /* temperature */
#define MS5611_CMD_ADC_READ 0x00U
#define MS5611_CMD_PROM_READ 0xA0U /* + 2 * word index (0..7) */

/* Timing (datasheet max values, rounded up to whole milliseconds) */
#define MS5611_RESET_MS 3U /* 2.8 ms */
#define MS5611_I2C_TIMEOUT_MS 10U

/* Max conversion time per OSR: 0.60, 1.17, 2.28, 4.54, 9.04 ms. Indexed by
 * osr >> 1. */
static const uint8_t conv_ms[] = {1, 2, 3, 5, 10};

static ULONG ms_to_ticks(uint32_t ms) {
  return (ULONG)((ms * TX_TIMER_TICKS_PER_SECOND + 999U) / 1000U) + 1U;
}

static bool addr_valid(ms5611_addr_t addr) {
  return addr == MS5611_ADDR_CSB_HIGH || addr == MS5611_ADDR_CSB_LOW;
}

static bool osr_valid(ms5611_osr_t osr) {
  return ((unsigned)osr <= 0x08U) && (((unsigned)osr & 1U) == 0U);
}

static ms5611_status_t bus_failed(ms5611_t *dev) {
  dev->last_bus_error = HAL_I2C_GetError(dev->hi2c);
  return MS5611_ERR_BUS;
}

static ms5611_status_t send_cmd(ms5611_t *dev, uint8_t cmd) {
  if (HAL_I2C_Master_Transmit(dev->hi2c, (uint16_t)(dev->addr << 1), &cmd, 1,
                              MS5611_I2C_TIMEOUT_MS) != HAL_OK) {
    return bus_failed(dev);
  }
  return MS5611_OK;
}

static ms5611_status_t receive(ms5611_t *dev, uint8_t *buf, uint16_t len) {
  if (HAL_I2C_Master_Receive(dev->hi2c, (uint16_t)(dev->addr << 1), buf, len,
                             MS5611_I2C_TIMEOUT_MS) != HAL_OK) {
    return bus_failed(dev);
  }
  return MS5611_OK;
}

/* Start a conversion, wait for it, and read the 24-bit result. */
static ms5611_status_t convert(ms5611_t *dev, uint8_t convert_cmd,
                               uint32_t *raw) {
  uint8_t buf[3];

  ms5611_status_t st = send_cmd(dev, (uint8_t)(convert_cmd | dev->osr));
  if (st != MS5611_OK) {
    return st;
  }

  tx_thread_sleep(ms_to_ticks(conv_ms[dev->osr >> 1]));

  st = send_cmd(dev, MS5611_CMD_ADC_READ);
  if (st != MS5611_OK) {
    return st;
  }
  st = receive(dev, buf, sizeof buf);
  if (st != MS5611_OK) {
    return st;
  }

  *raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) |
         buf[2]; /* MSB first */

  /* The chip returns 0 if the result is read before the conversion is done. */
  return (*raw == 0U) ? MS5611_ERR_NO_DATA : MS5611_OK;
}

ms5611_status_t ms5611_init(ms5611_t *dev, I2C_HandleTypeDef *hi2c,
                            ms5611_addr_t addr, ms5611_osr_t osr) {
  if (dev == NULL || hi2c == NULL || !addr_valid(addr) || !osr_valid(osr)) {
    return MS5611_ERR_ARG;
  }

  dev->hi2c = hi2c;
  dev->addr = addr;
  dev->osr = osr;
  dev->initialized = false;
  dev->last_bus_error = 0;

  /* Reset so the chip reloads its PROM, then wait for it to finish. */
  ms5611_status_t st = send_cmd(dev, MS5611_CMD_RESET);
  if (st != MS5611_OK) {
    return st;
  }
  tx_thread_sleep(ms_to_ticks(MS5611_RESET_MS));

  /* Read the 8 PROM words: factory data, C1..C6, serial/CRC. */
  for (uint8_t i = 0; i < 8U; i++) {
    uint8_t buf[2];
    st = send_cmd(dev, (uint8_t)(MS5611_CMD_PROM_READ + 2U * i));
    if (st != MS5611_OK) {
      return st;
    }
    st = receive(dev, buf, sizeof buf);
    if (st != MS5611_OK) {
      return st;
    }
    dev->prom[i] = (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]); /* MSB first */
  }

  if (ms5611_crc4(dev->prom) != (dev->prom[7] & 0x0FU)) {
    return MS5611_ERR_CRC;
  }

  dev->initialized = true;
  return MS5611_OK;
}

ms5611_status_t ms5611_read(ms5611_t *dev, ms5611_sample_t *out) {
  if (dev == NULL || out == NULL) {
    return MS5611_ERR_ARG;
  }
  if (!dev->initialized) {
    return MS5611_ERR_NOT_INIT;
  }

  uint32_t d1;
  uint32_t d2;

  ms5611_status_t st = convert(dev, MS5611_CMD_CONVERT_D1, &d1);
  if (st != MS5611_OK) {
    return st;
  }
  st = convert(dev, MS5611_CMD_CONVERT_D2, &d2);
  if (st != MS5611_OK) {
    return st;
  }

  ms5611_compensate(dev->prom, d1, d2, out);
  return MS5611_OK;
}

const char *ms5611_status_str(ms5611_status_t status) {
  switch (status) {
  case MS5611_OK:
    return "ok";
  case MS5611_ERR_ARG:
    return "invalid argument";
  case MS5611_ERR_BUS:
    return "I2C bus error";
  case MS5611_ERR_CRC:
    return "PROM CRC mismatch";
  case MS5611_ERR_NOT_INIT:
    return "not initialized";
  case MS5611_ERR_NO_DATA:
    return "ADC returned no data";
  }
  return "unknown";
}