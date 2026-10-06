#ifndef FOC9_FOC_HPP
#define FOC9_FOC_HPP

#include "foc9/math.h"
#include "foc9/types.hpp"
#include <stdint.h>

void forward_clarke_transform(const PhaseVector_t& phase_currents, AlphaBetaFrame_t& I_ab);

void forward_park_transform(const AlphaBetaFrame_t& I_ab, const float32_t& theta_el,
                            DirectQuadratureFrame_t& I_dq);

void inverse_park_transform(const DirectQuadratureFrame_t& V_dq, const float32_t& theta_el,
                            AlphaBetaFrame_t& V_ab);

void inverse_clarke_transform(AlphaBetaFrame_t const& V_ab, PhaseVector_t& V_uvw);

void svpwm_generator(AlphaBetaFrame_t const& V_ab, const float& vbus, PhaseVector_t& duty);

void forward(PhaseVector_t& I_uvw, float const& theta_el, DirectQuadratureFrame_t& I_dq);

void backward(const DirectQuadratureFrame_t& V_dq, float32_t& theta_el, PhaseVector_t& duty,
              float vbus);

#endif // FOC9_HPP
// TODO: Feed VBUS to Controller, for SVPWM
//  - Pass timestamp to measurement callbacks, and pick measurements with same
//  callback
//  - Depending on encoder update rate, interpolate/extarpolate θ_el
//  - Predictive PWM delay compensation
//  - Duty Cycle Normalization
//  - Plot d_pi_loop with target and measured
//  - Plot q_pi_loop with target and measured
