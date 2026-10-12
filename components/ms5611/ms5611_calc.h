/**
 * @file ms5611_calc.h
 * @brief MS5611 conversion math and PROM CRC, no hardware interaction
 *
 * References: MS5611-01BA03 datasheet (first- and second-order compensation),
 *             TE AN520 (PROM CRC4).
 */
#ifndef MS5611_CALC_H
#define MS5611_CALC_H

#include <stdint.h>

/** One compensated reading, plus the raw ADC values it came from. */
typedef struct {
  int32_t pressure_pa; /**< Compensated pressure, Pa */
  int32_t
      temp_c100; /**< Compensated temperature, 0.01 degC (2554 = 25.54 degC) */
} ms5611_sample_t;

/**
 * @brief Convert raw D1/D2 to pressure and temperature (datasheet math).
 *
 * Includes second-order compensation below 20 degC.
 *
 * @param prom Calibration words read from the chip (C1..C6 at prom[1..6])
 * @param d1   Raw pressure ADC value
 * @param d2   Raw temperature ADC value
 * @param out  Result
 */
void ms5611_compensate(const uint16_t prom[8], uint32_t d1, uint32_t d2,
                       ms5611_sample_t *out);

/**
 * @brief Compute the 4-bit CRC over the PROM (TE AN520).
 *
 * Compare the result against (prom[7] & 0x0F).
 */
uint8_t ms5611_crc4(const uint16_t prom[8]);

#endif /* MS5611_CALC_H */