#include "foc9/pi_controller.hpp"
#include "math.h"

#include "foc9/pi_controller.hpp"
#include <stdint.h>

PIController::PIController(float vbus, float Kp_0, float Ki_0)
    : Kp(Kp_0), Ki(Ki_0), integrator(0.0f), out_max(vbus / sqrtf(3.0f)),
      out_min(-vbus / sqrtf(3.0f)), _output(0.0f) {}

void PIController::reset() {
  integrator = 0.0f;
  _output = 0.0f;
}

void PIController::update(float target, float measured, float dt) {
  if (!isfinite(target) || !isfinite(measured) || !isfinite(dt) || dt <= 0.0f) {
    reset();
    return;
  }

  const float error = target - measured;
  const float p_term = Kp * error;

  const float candidate_integrator = integrator + Ki * error * dt;
  const float candidate_output = p_term + candidate_integrator;

  // Integrate only when not driving further into saturation.
  if ((candidate_output <= out_max && candidate_output >= out_min) ||
      (candidate_output > out_max && error < 0.0f) ||
      (candidate_output < out_min && error > 0.0f)) {
    integrator = candidate_integrator;
  }

  integrator = fmax(out_min, fmin(integrator, out_max));

  _output = fmax(out_min, fmin(p_term + integrator, out_max));
}

float PIController::output() {
  return _output;
}
