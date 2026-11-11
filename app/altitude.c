#include "altitude.h"

#include <math.h>

/* Float, not double: the Cortex-M3 has no FPU, so both are done in software
 * and float is the cheaper of the two. Precision is still around 1 cm. */
#define LAPSE_RATE_K_PER_M 0.0065f /* temperature drop per meter, |L| */
#define BARO_EXPONENT 0.190266f    /* -R * L / (g0 * M) */
#define CELSIUS_TO_KELVIN 273.15f

float altitude_m(float pressure_pa, float p0_pa, float t0_k) {
  return (t0_k / LAPSE_RATE_K_PER_M) *
         (1.0f - powf(pressure_pa / p0_pa, BARO_EXPONENT));
}

float altitude_c100_to_kelvin(float temp_c100) {
  return temp_c100 / 100.0f + CELSIUS_TO_KELVIN;
}
