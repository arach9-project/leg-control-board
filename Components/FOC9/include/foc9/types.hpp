#ifndef FOC9_TYPES_HPP
#define FOC9_TYPES_HPP

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
#endif
