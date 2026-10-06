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

typedef struct {
  PhaseVector_t duty;
} FocOutput;

typedef struct {
  uint16_t u;
  uint16_t v;
  uint16_t w;
} PhaseIntVector_t;
#endif
