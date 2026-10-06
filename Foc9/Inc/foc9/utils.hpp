#ifndef FOC9_TYPES_HPP
#define FOC9_TYPES_HPP

#include "foc9/constants.h"

template <typename T> constexpr T clamp(const T value, const T minimum, const T maximum) {
  return (value < minimum) ? minimum : (value > maximum) ? maximum : value;
};

inline float wrap_0_2pi(float angle) {
  while (angle >= TWO_PI) {
    angle -= TWO_PI;
  }

  while (angle < 0.0f) {
    angle += TWO_PI;
  }

  return angle;
}

inline float wrap_minus_pi_pi(float angle) {
  while (angle >= PI) {
    angle -= TWO_PI;
  }

  while (angle < -PI) {
    angle += TWO_PI;
  }

  return angle;
}

#endif
