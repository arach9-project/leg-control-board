#ifndef AS5048A_TYPES_H
#define AS5048A_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t raw;
    float degrees;
    float radians;
} AS5048A_Angle_t;

typedef struct {
    uint16_t raw;
} AS5048A_Magnitude_t;

typedef struct {
    uint16_t raw;
    uint8_t agc;
    bool offset_compensation_finished;
    bool cordic_overflow;
    bool magnet_too_strong;
    bool magnet_too_weak;
} AS5048A_Diagnostics_t;

typedef struct {
    AS5048A_Angle_t angle;
    AS5048A_Magnitude_t magnitude;
    AS5048A_Diagnostics_t diagnostics;
    bool valid;
} AS5048A_Sample_t;

#ifdef __cplusplus
}
#endif
#endif /* AS5048A_TYPES_H */
