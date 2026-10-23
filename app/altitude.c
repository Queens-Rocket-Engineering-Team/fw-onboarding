#include "altitude.h"


static float calculate_altitude(const float pressure_Pa, const float base_pressure_Pa, const float base_temperature_K) {
    if (base_pressure_Pa <= 0.0f) 
        return 0.0f;
    return ((base_temperature_K / lapse_rate) * (powf((pressure_Pa / base_pressure_Pa), (gas_constant*lapse_rate*-1.0) / (gravity*molar_mass_air)) - 1 ));
}