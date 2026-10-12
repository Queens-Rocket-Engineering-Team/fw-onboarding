/**
 * @file ms5611.h
 * @brief Driver for the TE MS5611-01BA03 barometric pressure sensor (I2C).
 *
 * References: MS5611-01BA03 datasheet (commands, timing, conversion math),
 *             TE AN520 (PROM CRC4).
 *
 * Usage:
 * @code
 *   static ms5611_t baro;
 *   ms5611_status_t st = ms5611_init(&baro, &hi2c2, MS5611_ADDR_CSB_HIGH,
 *                                    MS5611_OSR_4096);
 *   if (st != MS5611_OK) { log ms5611_status_str(st); }
 *
 *   ms5611_sample_t s;
 *   if (ms5611_read(&baro, &s) == MS5611_OK) {
 *     use s.pressure_pa and s.temp_c100
 *   }
 * @endcode
 *
 *
 * The conversion math lives in ms5611_calc.h, separate from the I/O.
 */
#ifndef MS5611_H
#define MS5611_H

#include <stdbool.h>
#include <stdint.h>

#include "ms5611_calc.h"
#include "stm32f1xx_hal.h"

/** Result of every driver call */
typedef enum {
  MS5611_OK = 0,
  MS5611_ERR_ARG, /**< NULL pointer, invalid address or OSR */
  MS5611_ERR_BUS, /**< I2C transfer failed (NACK, timeout), see last_bus_error
                   */
  MS5611_ERR_CRC, /**< PROM CRC mismatch: calibration data is unusable */
  MS5611_ERR_NOT_INIT, /**< ms5611_read() before a successful ms5611_init() */
  MS5611_ERR_NO_DATA,  /**< ADC returned 0: result read before conversion
                          finished */
} ms5611_status_t;

/** 7-bit I2C address, selected by the CSB pin. */
typedef enum {
  MS5611_ADDR_CSB_HIGH = 0x76,
  MS5611_ADDR_CSB_LOW = 0x77,
} ms5611_addr_t;

/** Oversampling ratio. Higher = less noise, longer conversion (datasheet
 * table). */
typedef enum {
  MS5611_OSR_256 = 0x00,  /**< 0.60 ms max, 0.065 mbar RMS noise */
  MS5611_OSR_512 = 0x02,  /**< 1.17 ms max, 0.042 mbar */
  MS5611_OSR_1024 = 0x04, /**< 2.28 ms max, 0.027 mbar */
  MS5611_OSR_2048 = 0x06, /**< 4.54 ms max, 0.018 mbar */
  MS5611_OSR_4096 = 0x08, /**< 9.04 ms max, 0.012 mbar */
} ms5611_osr_t;

/** Device state. Treat the fields as private: only the driver writes them. */
typedef struct {
  I2C_HandleTypeDef *hi2c; /**< Bus the chip is on */
  ms5611_addr_t addr;      /**< Set by the board's CSB pin */
  ms5611_osr_t osr;        /**< Oversampling used for every conversion */
  uint16_t prom[8];        /**< Factory calibration, filled in by init */
  bool initialized;        /**< True after a successful init */
  uint32_t
      last_bus_error; /**< HAL_I2C_GetError() flags from the last bus failure */
} ms5611_t;

/**
 * @brief Reset the chip, read its calibration PROM and verify the CRC.
 *
 * @param dev  Device state to fill in
 * @param hi2c I2C bus the chip is on
 * @param addr I2C address, set by the board's CSB pin
 * @param osr  Oversampling ratio for all later reads
 */
ms5611_status_t ms5611_init(ms5611_t *dev, I2C_HandleTypeDef *hi2c,
                            ms5611_addr_t addr, ms5611_osr_t osr);

/**
 * @brief Run a pressure and a temperature conversion and compensate them.
 *
 * @param dev Initialized device
 * @param out Result, only written on MS5611_OK
 */
ms5611_status_t ms5611_read(ms5611_t *dev, ms5611_sample_t *out);

/** @brief Human-readable name for a status code, for log messages. */
const char *ms5611_status_str(ms5611_status_t status);

#endif /* MS5611_H */