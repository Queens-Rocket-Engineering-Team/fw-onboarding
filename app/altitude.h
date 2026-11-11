/* altitude.h - height above a reference point from barometric pressure.
 *
 * Standard-atmosphere formula for the troposphere (valid up to ~11 km):
 *
 *   h = T0 / L * (1 - (P / P0) ^ 0.190266),   L = 0.0065 K/m
 *
 * where 0.190266 = -R * L / (g0 * M) with L = -0.0065 K/m.
 */
#ifndef ALTITUDE_H
#define ALTITUDE_H

#include <stdint.h>

/**
 * @brief Height above the point where the reference was measured.
 *
 * @param pressure_pa Current pressure, Pa
 * @param p0_pa       Reference (ground) pressure, Pa. Must be > 0.
 * @param t0_k        Reference (ground) temperature, kelvin
 * @return Height in meters, negative below the reference
 */
float altitude_m(float pressure_pa, float p0_pa, float t0_k);

/** @brief MS5611 temperature (0.01 degC, 2554 = 25.54 degC) to kelvin. */
float altitude_c100_to_kelvin(float temp_c100);

#endif /* ALTITUDE_H */
