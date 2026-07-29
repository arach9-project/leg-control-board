
#include "arm_math.h"
#include "foc9/types.hpp"
#include "math.c"
// #include <algorithm>
#include "foc9/foc-controller.hpp"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define TWO_PI 6.28318530718f
#define NOMINAL_VBUS_VOLTAGE                                                   \
    12.0f // Set this to your power supply voltage (Volts)

template <typename T>
constexpr const T &clamp(const T &val, const T &low, const T &high) {
    return (val < low) ? low : (high < val) ? high : val;
}

FocController::FocController()
    : d_axis_loop(PIController(NOMINAL_VBUS_VOLTAGE, 0.5f, 100.0f)),
      q_axis_loop(PIController(NOMINAL_VBUS_VOLTAGE, 0.5f, 100.0f)) {
}

void FocController::init() {
    d_axis_loop = PIController(NOMINAL_VBUS_VOLTAGE, 0.5f, 100.0f);
    q_axis_loop = PIController(NOMINAL_VBUS_VOLTAGE, 0.5f, 100.0f);
}

void FocController::set_target_currents(float I_d, float I_q) {
    target_Id = I_d;
    target_Iq = I_q;
}

void FocController::forward_clarke_transform(
    PhaseVector_t *const phase_currents, AlphaBetaFrame_t *I_ab) {
    I_ab->alpha = phase_currents->u; // I_alpha = I_a
    I_ab->beta =
        one_by_sqrt3 * (phase_currents->v -
                        phase_currents->w); // I_beta = 1/sqrt(3)(I_b - I_c)
}

void FocController::forward_park_transform(AlphaBetaFrame_t *const I_ab,
                                           DirectQuadratureFrame_t *I_dq) {
    float c_I = our_arm_cos_f32(theta_el);
    float s_I = our_arm_sin_f32(theta_el);
    I_dq->d = I_ab->alpha * c_I +
              I_ab->beta *
                  s_I; // I_d =  I_alpha * cos(theta_e) + I_beta * sin(theta_e)
    I_dq->q = -I_ab->alpha * s_I +
              I_ab->beta *
                  c_I; // I_q = -I_alpha * sin(theta_e) + I_beta * cos(theta_e)
}

void FocController::inverse_park_transform(DirectQuadratureFrame_t *const V_dq,
                                           AlphaBetaFrame_t *V_ab) {
    float c_I = our_arm_cos_f32(theta_el);
    float s_I = our_arm_sin_f32(theta_el);
    V_ab->alpha =
        c_I * V_dq->d - s_I * V_dq->q; //  V_α = cos(θ_e) * V_d - sin(θ_e) * V_q

    V_ab->beta =
        s_I * V_dq->d + c_I * V_dq->q; //  V_β = sin(θ_e) * V_d + cos(θ_e) * V_q
}

void FocController::inverse_clarke_transform(AlphaBetaFrame_t const *V_ab,
                                             PhaseVector_t *V_uvw) {
    V_uvw->u = V_ab->alpha;
    V_uvw->v =
        -0.5f * V_ab->alpha + 0.8660254f * V_ab->beta; // 0.8660254 = sqrt(3)/2
    V_uvw->w = -0.5f * V_ab->alpha - 0.8660254f * V_ab->beta;
}

void FocController::svpwm_generator(AlphaBetaFrame_t *V_ab, float vbus,
                                    PhaseVector_t *duty) {

    PhaseVector_t V_uvw = {0, 0, 0};

    inverse_clarke_transform(V_ab, &V_uvw);

    float32_t v_phases[3] = {V_uvw.u, V_uvw.v, V_uvw.w};
    float32_t v_max, v_min;
    uint32_t idx; // Index of the highest phase (0, 1, or 2)
    arm_max_f32(v_phases, 3, &v_max, &idx);
    arm_max_f32(v_phases, 3, &v_min, &idx);
    float v_offset = (v_max + v_min) * 0.5f;

    V_uvw.u -= v_offset;
    V_uvw.v -= v_offset;
    V_uvw.w -= v_offset;

    duty->u = (V_uvw.u / vbus) + 0.5f;
    duty->v = (V_uvw.v / vbus) + 0.5f;
    duty->w = (V_uvw.w / vbus) + 0.5f;

    duty->u = clamp(duty->u, 0.0f, 1.0f);
    duty->v = clamp(duty->v, 0.0f, 1.0f);
    duty->w = clamp(duty->w, 0.0f, 1.0f);
}

const FocOutput FocController::step(const FocInput *input, float dt) {

    I_uvw = input->I;
    I_uvw.w = -I_uvw.u - I_uvw.v; // KCL: I_w = -I_u - I_v
                                  //

    forward_clarke_transform(&I_uvw, &I_ab);
    forward_park_transform(&I_ab, &I_dq);

    d_axis_loop.update(target_Id, I_dq.d, dt);
    q_axis_loop.update(target_Iq, I_dq.q, dt);

    V_dq.d = d_axis_loop.output();
    V_dq.q = q_axis_loop.output();

    inverse_park_transform(&V_dq, &V_ab);
    svpwm_generator(&V_ab, NOMINAL_VBUS_VOLTAGE, &duty);
    return {.duty = duty};
}
// TODO: Feed VBUS to Controller, for SVPWM
//  - Pass timestamp to measurement callbacks, and pick measurements with same
//  callback
//  - Depending on encoder update rate, interpolate/extarpolate θ_el
//  - Predictive PWM delay compensation
//  - Duty Cycle Normalization
//  - Plot d_pi_loop with target and measured
//  - Plot q_pi_loop with target and measured
