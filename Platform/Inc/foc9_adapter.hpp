#pragma once

#include "as5048a/types.h"
#include "foc9/axis.hpp"

/* C++-only access to the actual objects. Never include this file from C. */
extern Axis axis1;
extern Axis axis2;
extern Axis axis3;

extern AS5048A_Angle_t encoder1_angle;
extern AS5048A_Angle_t encoder2_angle;
extern AS5048A_Angle_t encoder3_angle;
