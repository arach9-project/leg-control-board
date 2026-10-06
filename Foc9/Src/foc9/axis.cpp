#include "foc9/axis.hpp"
#include "foc9/constants.h"
#include "foc9/foc.hpp"
#include "foc9/utils.hpp"
#include <foc9/pi_controller.hpp>
#include <foc9/types.hpp>
#include <stdint.h>

Axis::Axis(Axis_Config_t config)
    : d_axis_loop_(PIController(config.vbus, 0.5f, 100.0f)),
      q_axis_loop_(PIController(config.vbus, 0.5f, 100.0f)) {
  this->cfg = config;
};

void Axis::init() {

};
void Axis::test_angle(float theta) {
  DirectQuadratureFrame_t vdq = {
      .d = 1.5f,
      .q = 0.0f,
  };

  backward(vdq, theta, _output.duty, cfg.vbus);
}

void Axis::start_electrical_offset_calibration() {
  calibration_state = CalibrationState::RampAlignment;
  calibration_counter = 0;

  calibration_sum_sin = 0.0f;
  calibration_sum_cos = 0.0f;

  axis_calibrated = false;
}

void Axis::calibration_step(float theta_mech) {
  V_dq = {
      .d = 0.0f,
      .q = 0.0f,
  };

  switch (calibration_state) {

  case CalibrationState::Idle:
    return;

  case CalibrationState::RampAlignment: {
    float alpha = static_cast<float>(calibration_counter) / static_cast<float>(cfg.kRampCycles);

    if (alpha > 1.0f) {
      alpha = 1.0f;
    }

    V_dq.d = cfg.kCalibrationVoltage * alpha;

    backward(V_dq, cfg.kThetaAlign, _output.duty, cfg.vbus);

    calibration_counter++;

    if (calibration_counter >= cfg.kRampCycles) {
      calibration_counter = 0;
      calibration_state = CalibrationState::Settle;
    }

    break;
  }

  case CalibrationState::Settle:
    // Keep the rotor locked at the exact same electrical angle.
    V_dq.d = cfg.kCalibrationVoltage;

    backward(V_dq, cfg.kThetaAlign, _output.duty, cfg.vbus);

    calibration_counter++;

    if (calibration_counter >= cfg.kSettleCycles) {
      calibration_counter = 0;

      calibration_sum_sin = 0.0f;
      calibration_sum_cos = 0.0f;

      calibration_state = CalibrationState::Sample;
    }

    break;

  case CalibrationState::Sample:
    // IMPORTANT:
    // Continue holding the rotor at theta_align while sampling.
    V_dq.d = cfg.kCalibrationVoltage;

    backward(V_dq, cfg.kThetaAlign, _output.duty, cfg.vbus);

    calibration_sum_sin += sinf(theta_mech);
    calibration_sum_cos += cosf(theta_mech);

    calibration_counter++;

    if (calibration_counter >= cfg.kSampleCycles) {
      float averaged_theta = atan2f(calibration_sum_sin, calibration_sum_cos);

      if (averaged_theta < 0.0f) {
        averaged_theta += TWO_PI;
      }

      electrical_offset =
          wrap_0_2pi(encoder_direction * cfg.pole_pairs * averaged_theta - cfg.kThetaAlign);

      calibration_state = CalibrationState::Complete;
    }

    break;

  case CalibrationState::Complete:
    _output.duty = {
        .u = 0.5f,
        .v = 0.5f,
        .w = 0.5f,
    };

    axis_calibrated = true;
    calibration_state = CalibrationState::Idle;

    break;
  }
}

void Axis::position_loop_step(float theta_mech, float dt) {
  float position_error = wrap_minus_pi_pi(theta_target - theta_mech);

  if (fabsf(position_error) < cfg.kPositionDeadband) {
    position_error = 0.0f;
  }

  const float delta_theta = wrap_minus_pi_pi(theta_mech - prev_theta_mech);

  omega_raw = delta_theta / dt;

  omega_mech += cfg.kVelocityAlpha * (omega_raw - omega_mech);

  if (fabsf(omega_mech) < cfg.kVelocityDeadband) {
    omega_mech = 0.0f;
  }

  const float iq_command = cfg.kPositionKp * position_error - cfg.kPositionKd * omega_mech;

  iq_ref = clamp(iq_command, -cfg.kIqLimit, cfg.kIqLimit);

  prev_theta_mech = theta_mech;
}

void Axis::enter_position_mode(float theta_mech) {
  prev_theta_mech = theta_mech;
  omega_raw = 0.0f;
  omega_mech = 0.0f;
  iq_ref = 0.0f;

  reset();
}

void Axis::current_loop_step(float theta_el, float dt) {
  // Clamp current references.
  id_ref = clamp(id_ref, -cfg.kIdLimit, cfg.kIdLimit);
  iq_ref = clamp(iq_ref, -cfg.kIqLimit, cfg.kIqLimit);

  // Measured phase currents -> dq currents.
  forward(I, theta_el, I_dq);

  // Current PI controllers.
  d_axis_loop_.update(id_ref, I_dq.d, dt);

  q_axis_loop_.update(iq_ref, I_dq.q, dt);

  V_dq.d = d_axis_loop_.output();
  V_dq.q = q_axis_loop_.output();

  // dq voltages -> phase duty.
  backward(V_dq, theta_el, _output.duty, cfg.vbus);
}

float open_loop_theta_el = 0.0f;

void Axis::open_loop_step(float mechanical_speed_rad_s, float dt) {
  constexpr float voltage = 1.5f;

  const float electrical_speed_rad_s = mechanical_speed_rad_s * static_cast<float>(cfg.pole_pairs);

  open_loop_theta_el = wrap_0_2pi(open_loop_theta_el + electrical_speed_rad_s * dt);

  V_dq = {
      .d = voltage,
      .q = 0.0f,
  };

  backward(V_dq, open_loop_theta_el, _output.duty, cfg.vbus);
}

const FocOutput Axis::step(float theta, float dt) {
  float theta_mech = wrap_0_2pi(encoder_direction * theta);
  float theta_el = wrap_0_2pi(encoder_direction * theta * cfg.pole_pairs - electrical_offset);

  switch (axis_mode) {

  case AxisMode::Calibration:
    calibration_step(theta_mech);
    break;

  case AxisMode::OpenLoop:
    open_loop_step(open_loop_speed, dt);
    break;

  case AxisMode::Current:
    current_loop_step(theta_el, dt);
    break;

  case AxisMode::Position:
    position_loop_step(theta_mech, dt);
    break;

  default:
    break;
  }

  return {.duty = duty};
}

void Axis::reset() {
  d_axis_loop_.reset();
  q_axis_loop_.reset();
}

// TODO:
// - [x] Simplify FOC code and remove adapter convention, make foc math one file
// - [ ] Move period calc to adc.c
//- [ ] Check for AS5600 health and if not healthy magnet then dont set theta_el
//- [ ] only command drv in one loop
//- [ ] Command target position from cubemonitor
//- [ ] Enable and Disable FOC loop from cubemonitor
// TODO: Each axis has a contorl loop
// TODO: Add state machine that i can use to debug the foc loop, either error or
// the current step
// TODO: implement sensorless FOC
// TODO: Function to calibrate motor and get electrical offset, set motor to 0
// rad, read encoder value, calculate offset
