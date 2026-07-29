#ifndef FOC9_HPP
#define FOC9_HPP

#include "constants.h"
#include "pi-controller.hpp"
#include "types.hpp"
#include <math.h>
#include <stdint.h>

// Usage in your FOC loop:

class FocController {
  private:
    PhaseVector_t I_uvw = {0, 0, 0}; // phase currents
    float theta_el = 0;
    PIController d_axis_loop;
    PIController q_axis_loop;

    float target_Id = 0.0f;
    float target_Iq = 0.0f; // Torque command
    PhaseVector_t duty = {0, 0, 0};

    AlphaBetaFrame_t I_ab = {0, 0};
    AlphaBetaFrame_t V_ab = {0, 0};
    DirectQuadratureFrame_t I_dq = {0, 0};
    DirectQuadratureFrame_t V_dq = {0, 0};

    void forward_clarke_transform(PhaseVector_t *const phase_currents,
                                  AlphaBetaFrame_t *I_ab);

    void forward_park_transform(AlphaBetaFrame_t *const I_ab,
                                DirectQuadratureFrame_t *I_dq);

    void inverse_park_transform(DirectQuadratureFrame_t *const V_dq,
                                AlphaBetaFrame_t *V_ab);

    void inverse_clarke_transform(AlphaBetaFrame_t const *V_ab,
                                  PhaseVector_t *V_uvw);

    void svpwm_generator(AlphaBetaFrame_t *V_ab, float vbus,
                         PhaseVector_t *duty);

  public:
    FocController();
    void init();

    void set_target_currents(float I_d, float I_q);

    void get_duty(PhaseVector_t *duty);

    const FocOutput step(const FocInput *input, float dt);
};

#endif // FOC9_HPP
