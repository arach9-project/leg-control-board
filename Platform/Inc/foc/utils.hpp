#ifndef FOC9_TYPES_HPP
#define FOC9_TYPES_HPP

#include "foc/constants.h"
#include "foc/types.hpp"

template <typename T> constexpr T clamp(const T value, const T minimum, const T maximum) {
  return (value < minimum) ? minimum : (value > maximum) ? maximum : value;
};

void duty_to_ccr(PhaseVector_t& duty, PhaseIntVector_t& ccr, float arr, float min_duty,
                 float max_duty) {
  float du = clamp(duty.u, min_duty, max_duty);
  float dv = clamp(duty.v, min_duty, max_duty);
  float dw = clamp(duty.w, min_duty, max_duty);

  ccr.u = static_cast<uint16_t>(du * arr + 0.5f);
  ccr.v = static_cast<uint16_t>(dv * arr + 0.5f);
  ccr.w = static_cast<uint16_t>(dw * arr + 0.5f);
}

float wrap_0_2pi(float angle) {
  while (angle >= TWO_PI) {
    angle -= TWO_PI;
  }

  while (angle < 0.0f) {
    angle += TWO_PI;
  }

  return angle;
}

float wrap_minus_pi_pi(float angle) {
  while (angle >= PI) {
    angle -= TWO_PI;
  }

  while (angle < -PI) {
    angle += TWO_PI;
  }

  return angle;
}

#endif
