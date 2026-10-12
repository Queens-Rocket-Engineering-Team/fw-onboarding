#include "ms5611_calc.h"

void ms5611_compensate(const uint16_t prom[8], uint32_t d1, uint32_t d2,
                       ms5611_sample_t *out) {
  /* First order */
  const int32_t dT = (int32_t)d2 - ((int32_t)prom[5] << 8); /* D2 - TREF */
  int32_t temp = 2000 + (int32_t)(((int64_t)dT * prom[6]) >> 23);
  int64_t off = ((int64_t)prom[2] << 16) + (((int64_t)prom[4] * dT) >> 7);
  int64_t sens = ((int64_t)prom[1] << 15) + (((int64_t)prom[3] * dT) >> 8);

  /* Second order, below 20 degC */
  if (temp < 2000) {
    const int64_t d = (int64_t)temp - 2000;
    int64_t off2 = (5 * d * d) >> 1;
    int64_t sens2 = (5 * d * d) >> 2;

    if (temp < -1500) {
      const int64_t e = (int64_t)temp + 1500;
      off2 += 7 * e * e;
      sens2 += (11 * e * e) >> 1;
    }

    temp -= (int32_t)(((int64_t)dT * dT) >> 31); /* T2 */
    off -= off2;
    sens -= sens2;
  }

  out->temp_c100 = temp;
  out->pressure_pa = (int32_t)(((((int64_t)d1 * sens) >> 21) - off) >> 15);
}

uint8_t ms5611_crc4(const uint16_t prom[8]) {
  uint16_t p[8];
  for (int i = 0; i < 8; i++) {
    p[i] = prom[i];
  }
  p[7] &= 0xFF00U; /* the CRC was computed with its own bits zeroed */

  /* Binary long division of the 16 PROM bytes (MSB of each word first) by
   * the polynomial x^4 + x + 1, placed in the top bits as 0x3000. The
   * remainder left in the top 4 bits is the CRC. */
  uint16_t rem = 0;
  for (int n = 0; n < 16; n++) {
    rem ^=
        (n & 1) ? (uint16_t)(p[n >> 1] & 0x00FFU) : (uint16_t)(p[n >> 1] >> 8);
    for (int b = 0; b < 8; b++) {
      rem = (rem & 0x8000U) ? (uint16_t)((rem << 1) ^ 0x3000U)
                            : (uint16_t)(rem << 1);
    }
  }
  return (uint8_t)((rem >> 12) & 0x0FU);
}