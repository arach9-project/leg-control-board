

#include "foc9/pi-controller.hpp"

PIController::PIController(float vbus, float Kp_0, float Ki_0) {
    Kp = Kp_0;
    Ki = Ki_0;
    out_max = vbus / sqrtf(3.0f);
    out_min = -vbus / sqrtf(3.0f);
    integrator = 0.0f;
}
void PIController::update(float target, float measured, float dt) {
    float error = target - measured;

    float p_term = Kp * error;

    integrator += Ki * error * dt;

    if (integrator > out_max) {
        integrator = out_max;
    } else if (integrator < out_min) {
        integrator = out_min;
    }

    _output = p_term + integrator;

    if (_output > out_max) {
        _output = out_max;
    } else if (_output < out_min) {
        _output = out_min;
    }
}

float PIController::output() {
    return _output;
}
