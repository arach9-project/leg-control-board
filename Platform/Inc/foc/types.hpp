#ifndef FOC_TYPES_HPP
#define FOC_TYPES_HPP

#include <stdint.h>

typedef struct {
  float u;
  float v;
  float w;
} PhaseVector_t;

typedef struct {
  float alpha;
  float beta;
} AlphaBetaFrame_t;

typedef struct {
  float d;
  float q;
} DirectQuadratureFrame_t;

struct FocInput {
  PhaseVector_t I;
  float theta_el;
  float vel_el;
  float id_ref;
  float iq_ref;
  float vbus;
};

struct FocOutput {
  PhaseVector_t duty;
};

struct PhaseIntVector_t {
  uint16_t u;
  uint16_t v;
  uint16_t w;
};
#endif
