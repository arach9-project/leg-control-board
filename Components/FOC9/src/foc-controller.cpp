
#include "arm_math.h"
#include "foc9/types.hpp"
#include "math.c"
// #include <algorithm>
#include "foc9/foc-controller.hpp"
#include <stdint.h>

#define TWO_PI 6.28318530718f

template <typename T>
constexpr const T &clamp(const T &val, const T &low, const T &high) {
  return (val < low) ? low : (high < val) ? high : val;
}

FocController::FocController()
    : d_axis_loop_(PIController(NOMINAL_VBUS_VOLTAGE, 0.5f, 100.0f)),
      q_axis_loop_(PIController(NOMINAL_VBUS_VOLTAGE, 0.5f, 100.0f)) {}

void FocController::init() {
  d_axis_loop_ = PIController(NOMINAL_VBUS_VOLTAGE, 0.5f, 100.0f);
  q_axis_loop_ = PIController(NOMINAL_VBUS_VOLTAGE, 0.5f, 100.0f);
}

void FocController::forward_clarke_transform(
    const PhaseVector_t &phase_currents, AlphaBetaFrame_t &I_ab) {
  I_ab.alpha = phase_currents.u; // I_alpha = I_a
  I_ab.beta =
      one_by_sqrt3 *
      (phase_currents.v - phase_currents.w); // I_beta = 1/sqrt(3)(I_b - I_c)
}

void FocController::forward_park_transform(const AlphaBetaFrame_t &I_ab,
                                           const float32_t &theta_el,
                                           DirectQuadratureFrame_t &I_dq) {
  float c_I = our_arm_cos_f32(theta_el);
  float s_I = our_arm_sin_f32(theta_el);
  I_dq.d =
      I_ab.alpha * c_I +
      I_ab.beta * s_I; // I_d =  I_alpha * cos(theta_e) + I_beta * sin(theta_e)
  I_dq.q =
      -I_ab.alpha * s_I +
      I_ab.beta * c_I; // I_q = -I_alpha * sin(theta_e) + I_beta * cos(theta_e)
}

void FocController::inverse_park_transform(const DirectQuadratureFrame_t &V_dq,
                                           const float32_t &theta_el,
                                           AlphaBetaFrame_t &V_ab) {
  float c_I = our_arm_cos_f32(theta_el);
  float s_I = our_arm_sin_f32(theta_el);
  V_ab.alpha =
      c_I * V_dq.d - s_I * V_dq.q; //  V_α = cos(θ_e) * V_d - sin(θ_e) * V_q

  V_ab.beta =
      s_I * V_dq.d + c_I * V_dq.q; //  V_β = sin(θ_e) * V_d + cos(θ_e) * V_q
}

void FocController::inverse_clarke_transform(AlphaBetaFrame_t const &V_ab,
                                             PhaseVector_t &V_uvw) {
  V_uvw.u = V_ab.alpha;
  V_uvw.v =
      -0.5f * V_ab.alpha + 0.8660254f * V_ab.beta; // 0.8660254 = sqrt(3)/2
  V_uvw.w = -0.5f * V_ab.alpha - 0.8660254f * V_ab.beta;
}

void FocController::svpwm_generator(AlphaBetaFrame_t const &V_ab,
                                    const float &vbus, PhaseVector_t &duty) {

  PhaseVector_t V_uvw = {0, 0, 0};

  inverse_clarke_transform(V_ab, V_uvw);

  float32_t v_phases[3] = {V_uvw.u, V_uvw.v, V_uvw.w};
  float32_t v_max, v_min;
  uint32_t idx; // Index of the highest phase (0, 1, or 2)
  arm_max_f32(v_phases, 3, &v_max, &idx);
  arm_min_f32(v_phases, 3, &v_min, &idx);
  float v_offset = (v_max + v_min) * 0.5f;

  V_uvw.u -= v_offset;
  V_uvw.v -= v_offset;
  V_uvw.w -= v_offset;

  duty.u = (V_uvw.u / vbus) + 0.5f;
  duty.v = (V_uvw.v / vbus) + 0.5f;
  duty.w = (V_uvw.w / vbus) + 0.5f;

  duty.u = clamp(duty.u, 0.0f, 1.0f);
  duty.v = clamp(duty.v, 0.0f, 1.0f);
  duty.w = clamp(duty.w, 0.0f, 1.0f);
}

void FocController::forward(PhaseVector_t &I_uvw, float const &theta_el,
                            DirectQuadratureFrame_t &I_dq) {

  AlphaBetaFrame_t I_ab = {0, 0};

  I_uvw.w = -I_uvw.u - I_uvw.v; // KCL: I_w = -I_u - I_v
  forward_clarke_transform(I_uvw, I_ab);
  forward_park_transform(I_ab, theta_el, I_dq);
}

void FocController::backward(const DirectQuadratureFrame_t &V_dq,
                             float32_t &theta_el, PhaseVector_t &duty) {
  AlphaBetaFrame_t V_ab = {0, 0};
  inverse_park_transform(V_dq, theta_el, V_ab);
  svpwm_generator(V_ab, vbus_, duty);
}

const FocOutput FocController::step(FocInput &input, float dt) {

  DirectQuadratureFrame_t I_dq = {0, 0};
  DirectQuadratureFrame_t V_dq = {0, 0};
  PhaseVector_t duty = {0, 0, 0};

  forward(input.I, input.theta_el, I_dq);

  d_axis_loop_.update(input.id_ref, I_dq.d, dt);
  q_axis_loop_.update(input.iq_ref, I_dq.q, dt);

  V_dq.d = d_axis_loop_.output();
  V_dq.q = q_axis_loop_.output();

  backward(V_dq, input.theta_el, duty);
  return {.duty = duty};
}

void FocController::reset() {
  d_axis_loop_.reset();
  q_axis_loop_.reset();
}
// TODO: Feed VBUS to Controller, for SVPWM
//  - Pass timestamp to measurement callbacks, and pick measurements with same
//  callback
//  - Depending on encoder update rate, interpolate/extarpolate θ_el
//  - Predictive PWM delay compensation
//  - Duty Cycle Normalization
//  - Plot d_pi_loop with target and measured
//  - Plot q_pi_loop with target and measured
