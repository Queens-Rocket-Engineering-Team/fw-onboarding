#ifndef ALTITUDE_H
#define ALTITUDE_H

#include <stdint.h>
#include <tx_user.h>
#include<math.h>

#define lapse_rate -0.0065
#define gas_constant 8.3144626
#define gravity 9.80665
#define molar_mass_air 0.0289644



float calculate_altitude(const float pressure_Pa, const float base_pressure_Pa, const float base_temperature_K);

#endif
    
