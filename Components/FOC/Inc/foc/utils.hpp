#ifndef FOCUTILS_LIB_H
#define FOCUTILS_LIB_H

#include "math.h"
#include "stdint.h"

// sign function
#define _sign(a) (((a) < 0) ? -1 : ((a) > 0))
#ifndef _round
#define _round(x) ((x) >= 0 ? (long)((x) + 0.5f) : (long)((x) - 0.5f))
#endif
#define _constrain(amt, low, high)                                             \
    ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define _sqrt(a) (_sqrtApprox(a))
#define _isset(a) ((a) != (NOT_SET))
#define _UNUSED(v) (void)(v)
#define _powtwo(x) (1 << (x))

#define _swap(a, b)                                                            \
    {                                                                          \
        auto temp = a;                                                         \
        a = b;                                                                 \
        b = temp;                                                              \
    }

// utility defines
#define _2_SQRT3 1.15470053838f
#define _SQRT3 1.73205080757f
#define _1_SQRT3 0.57735026919f
#define _SQRT3_2 0.86602540378f
#define _SQRT2 1.41421356237f
#define _120_D2R 2.09439510239f
#define _PI 3.14159265359f
#define _PI_2 1.57079632679f
#define _PI_3 1.0471975512f
#define _2PI 6.28318530718f
#define _3PI_2 4.71238898038f
#define _PI_6 0.52359877559f
#define _RPM_TO_RADS 0.10471975512f

#define NOT_SET -12345.0f
#define _HIGH_IMPEDANCE 0
#define _HIGH_Z _HIGH_IMPEDANCE
#define _ACTIVE 1
#define _NC ((int)NOT_SET)

#define MIN_ANGLE_DETECT_MOVEMENT (_2PI / 101.0f)

// Safe, type-specific function for integers
static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

static inline int max(int a, int b) {
    return (a < b) ? a : b;
}

// dq variables
typedef struct {
    float d;
    float q;
} DQ_s;

// dq voltage structs
typedef DQ_s DQVoltage_s;
// dq current structs
typedef DQ_s DQCurrent_s;

// alpha-beta variables
typedef struct {
    float alpha;
    float beta;
} AB_s;

typedef AB_s ABVoltage_s; // NOT USED
typedef AB_s ABCurrent_s;

// phase structs
typedef struct {
    float a;
    float b;
    float c;
} Phase_s;
typedef Phase_s PhaseVoltage_s; // NOT USED
typedef Phase_s PhaseCurrent_s;

/**
 *  Function approximating the sine calculation by using fixed size array
 * - execution time ~40us (Arduino UNO)
 *
 * @param a angle in between 0 and 2PI
 */

// function approximating the sine calculation by using fixed size array
// uses a 65 element lookup table and interpolation
// thanks to @dekutree for his work on optimizing this

// function approximating cosine calculation by using fixed size array
// ~55us (float array)
// ~56us (int array)
// precision +-0.005
// it has to receive an angle in between 0 and 2PI

// normalizing radian angle to [0,2PI]

__attribute__((weak)) float _sin(float a);

/**
 * Function approximating cosine calculation by using fixed size array
 * - execution time ~50us (Arduino UNO)
 *
 * @param a angle in between 0 and 2PI
 */
__attribute__((weak)) float _cos(float a);

/**
 * Function returning both sine and cosine of the angle in one call.
 * Internally it uses the _sin and _cos functions, but you may wish to
 * provide your own implementation which is more optimized.
 */
__attribute__((weak)) void _sincos(float a, float *s, float *c);

/**
 * Function approximating atan2
 *
 */
__attribute__((weak)) float _atan2(float y, float x);

/**
 * normalizing radian angle to [0,2PI]
 * @param angle - angle to be normalized
 */
__attribute__((weak)) float _normalizeAngle(float angle);

/**
 * Electrical angle calculation
 *
 * @param shaft_angle - shaft angle of the motor
 * @param pole_pairs - number of pole pairs
 */
float _electricalAngle(float shaft_angle, int pole_pairs);

/**
 * Function approximating square root function
 *  - using fast inverse square root
 *
 * @param value - number
 */
// square root approximation function using
// https://reprap.org/forum/read.php?147,219210
// https://en.wikipedia.org/wiki/Fast_inverse_square_root
__attribute__((weak)) float _sqrtApprox(float number);

// Electrical angle calculation
float _electricalAngle(float shaft_angle, int pole_pairs);

#endif
