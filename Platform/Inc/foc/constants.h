#ifndef FOC_CONSTANTS_H
#define FOC_CONSTANTS_H

#include "arm_math.h"
#define TWO_PI 6.28318530718f
#define NOMINAL_VBUS_VOLTAGE 12.0f // Set this to your power supply voltage (Volts)
#define MIN_DUTY 0.02f
#define MAX_DUTY 0.98f
#define ARR 4250

const float one_by_sqrt3 = 1.0f / sqrtf(3.0f);

#endif
