#ifndef FOC9_HPP
#define FOC9_HPP

#include "arm_math.h"
#include "constants.h"
#include "pi-controller.hpp"
#include "types.hpp"
#include <math.h>
#include <stdint.h>

// Usage in your FOC loop:

class FocController {
  private:
    PIController d_axis_loop_;
    PIController q_axis_loop_;

    float vbus_;
    void forward_clarke_transform(PhaseVector_t const &phase_currents,
                                  AlphaBetaFrame_t &I_ab);

    void forward_park_transform(AlphaBetaFrame_t const &I_ab,
                                const float32_t &theta_el,
                                DirectQuadratureFrame_t &I_dq);

    void inverse_park_transform(DirectQuadratureFrame_t const &V_dq,
                                const float32_t &theta_el,
                                AlphaBetaFrame_t &V_ab);

    void inverse_clarke_transform(AlphaBetaFrame_t const &V_ab,
                                  PhaseVector_t &V_uvw);

    void svpwm_generator(AlphaBetaFrame_t const &V_ab, float const &vbus,
                         PhaseVector_t &duty);

  public:
    FocController();
    void init();

    void get_duty(PhaseVector_t &duty);

    const FocOutput step(FocInput &input, float dt);

    void forward(PhaseVector_t &I_uvw, float const &theta_el,
                 DirectQuadratureFrame_t &I_dq);

    void backward(const DirectQuadratureFrame_t &V_dq, float32_t &theta_el,
                  PhaseVector_t &duty);
    void setVBus(float vbus) {
        vbus_ = vbus;
    }
};

#endif // FOC9_HPP
