#include "foc9/axis.hpp"

#include "foc9/constants.h"
#include "foc9/foc.hpp"
#include "foc9/types.hpp"
#include "foc9/utils.hpp"

constexpr float voltage = 3.0f;

Axis::Axis(Axis_Config_t config)
    : config(config), d_axis_loop_(PIController(config.vbus, config.kCurrentKp, config.kCurrentKi)),
      q_axis_loop_(PIController(config.vbus, config.kCurrentKp, config.kCurrentKi)) {

  //
  // Control defaults
  //
  control.theta_target = 0.0f;
  control.id_ref = 0.0f;
  control.iq_ref = 0.0f;
  control.open_loop_speed = 0.75f;

  control.position_kp = config.kPositionKp;
  control.position_kd = config.kPositionKd;

  control.axis_mode = AxisMode::Disabled;

  //
  // Telemetry defaults
  //
}

void Axis::init() {
  reset();
}

void Axis::test_angle(float theta) {
  DirectQuadratureFrame_t voltage_dq = {
      .d = 1.5f,
      .q = 0.0f,
  };

  backward(voltage_dq, theta, output_.duty, config.vbus);
}

void Axis::start_electrical_offset_calibration() {
  calibration_state_ = CalibrationState::RampAlignment;
  calibration_counter_ = 0;

  calibration_sum_sin_ = 0.0f;
  calibration_sum_cos_ = 0.0f;
}

void Axis::calibration_step(float theta_mech) {
  voltage_dq_.d = 0.0f;
  voltage_dq_.q = 0.0f;

  switch (calibration_state_) {
  case CalibrationState::Idle:
    return;

  case CalibrationState::RampAlignment: {
    float alpha = static_cast<float>(calibration_counter_) / static_cast<float>(config.kRampCycles);

    if (alpha > 1.0f) {
      alpha = 1.0f;
    }

    voltage_dq_.d = config.kCalibrationVoltage;

    voltage_dq_.d *= alpha;

    backward(voltage_dq_, config.kThetaAlign, output_.duty, config.vbus);

    calibration_counter_++;

    if (calibration_counter_ >= config.kRampCycles) {
      calibration_counter_ = 0;
      calibration_state_ = CalibrationState::Settle;
    }

    break;
  }

  case CalibrationState::Settle:
    voltage_dq_.d = config.kCalibrationVoltage;

    backward(voltage_dq_, config.kThetaAlign, output_.duty, config.vbus);

    calibration_counter_++;

    if (calibration_counter_ >= config.kSettleCycles) {
      calibration_counter_ = 0;

      calibration_sum_sin_ = 0.0f;
      calibration_sum_cos_ = 0.0f;

      calibration_state_ = CalibrationState::Sample;
    }

    break;

  case CalibrationState::Sample:
    voltage_dq_.d = config.kCalibrationVoltage;

    backward(voltage_dq_, config.kThetaAlign, output_.duty, config.vbus);

    calibration_sum_sin_ += sinf(theta_mech);
    calibration_sum_cos_ += cosf(theta_mech);

    calibration_counter_++;

    if (calibration_counter_ >= config.kSampleCycles) {
      float averaged_theta = atan2f(calibration_sum_sin_, calibration_sum_cos_);

      if (averaged_theta < 0.0f) {
        averaged_theta += TWO_PI;
      }

      electrical_offset_ =
          wrap_0_2pi(static_cast<float>(config.pole_pairs) * averaged_theta - config.kThetaAlign);

      calibration_state_ = CalibrationState::Complete;
    }

    break;

  case CalibrationState::Complete:
    output_.duty.u = 0.5f;
    output_.duty.v = 0.5f;
    output_.duty.w = 0.5f;

    reset();

    control.id_ref = 0.0f;
    control.iq_ref = 0.0f;

    control.axis_mode = AxisMode::Current;

    previous_mode_ = AxisMode::Current;

    break;
  }
}

void Axis::position_loop_step() {
  const float theta_target = control.theta_target;

  float position_error = wrap_minus_pi_pi(theta_target - theta_mech_);

  if (fabsf(position_error) < config.kPositionDeadband) {
    position_error = 0.0f;
  }

  const float position_kp = control.position_kp;
  const float position_kd = control.position_kd;

  const float iq_command = position_kp * position_error - position_kd * omega_mech_;

  control.iq_ref = clamp(iq_command, -config.kIqLimit, config.kIqLimit);
}

void Axis::enter_position_mode(float theta_mech) {
  prev_theta_mech_ = theta_mech;

  omega_raw_ = 0.0f;
  omega_mech_ = 0.0f;

  control.iq_ref = 0.0f;

  reset();
}

void Axis::current_loop_step(PhaseVector_t currents, float theta_el, float dt) {
  float id_ref = clamp(static_cast<float>(control.id_ref), -config.kIdLimit, config.kIdLimit);
  float iq_ref = clamp(static_cast<float>(control.iq_ref), -config.kIqLimit, config.kIqLimit);

  // Write clamped values back to externally visible controls.
  control.id_ref = id_ref;
  control.iq_ref = iq_ref;

  //
  // Phase currents -> dq
  //
  forward(currents, theta_el, current_dq_);

  id_error_ = id_ref - current_dq_.d;
  iq_error_ = iq_ref - current_dq_.q;

  //
  // PI controllers
  //
  d_axis_loop_.update(id_ref, current_dq_.d, dt);
  q_axis_loop_.update(iq_ref, current_dq_.q, dt);

  voltage_dq_.d = d_axis_loop_.output();
  voltage_dq_.q = q_axis_loop_.output();

  const float v_limit = config.vbus / sqrtf(3.0f);

  const float magnitude = sqrtf(voltage_dq_.d * voltage_dq_.d + voltage_dq_.q * voltage_dq_.q);

  if (magnitude > v_limit) {
    const float scale = v_limit / magnitude;

    voltage_dq_.d *= scale;
    voltage_dq_.q *= scale;
  }

  //
  // dq voltage -> PWM duty
  //
  backward(voltage_dq_, theta_el, output_.duty, config.vbus);
}
void Axis::update_velocity(float theta_mech, float dt) {
  if (!isfinite(theta_mech) || !isfinite(dt) || dt <= 0.0f) {
    return;
  }

  if (!velocity_initialized_) {
    prev_theta_mech_ = theta_mech;
    omega_raw_ = 0.0f;
    omega_mech_ = 0.0f;
    velocity_elapsed_ = 0.0f;
    velocity_initialized_ = true;
    return;
  }

  velocity_elapsed_ += dt;

  constexpr float velocity_period = 0.001f; // 1 kHz

  if (velocity_elapsed_ < velocity_period) {
    return;
  }

  const float delta_theta = wrap_minus_pi_pi(theta_mech - prev_theta_mech_);

  omega_raw_ = delta_theta / velocity_elapsed_;

  constexpr float tau = 0.02f; // 20 ms filter
  const float alpha = velocity_elapsed_ / (tau + velocity_elapsed_);

  omega_mech_ += alpha * (omega_raw_ - omega_mech_);

  prev_theta_mech_ = theta_mech;
  velocity_elapsed_ = 0.0f;
}

void Axis::open_loop_step(float mechanical_speed_rad_s, float dt) {

  const float electrical_speed_rad_s =
      mechanical_speed_rad_s * static_cast<float>(config.pole_pairs);

  open_loop_theta_el_ = wrap_0_2pi(open_loop_theta_el_ + electrical_speed_rad_s * dt);

  voltage_dq_.d = voltage;
  voltage_dq_.q = 0.0f;

  backward(voltage_dq_, open_loop_theta_el_, output_.duty, config.vbus);
}

FocOutput Axis::step(PhaseVector_t currents, float theta, float dt) {
  theta_mech_ = wrap_0_2pi(encoder_direction_ * theta);

  theta_el_ = wrap_0_2pi(encoder_direction_ * theta * static_cast<float>(config.pole_pairs) -
                         electrical_offset_);

  update_velocity(theta_mech_, dt);
  const AxisMode mode = control.axis_mode;

  // Automatically start calibration when entering calibration mode.
  if (mode == AxisMode::Calibration && previous_mode_ != AxisMode::Calibration) {
    reset();
    start_electrical_offset_calibration();
  }

  previous_mode_ = mode;

  switch (mode) {
  case AxisMode::Calibration:
    calibration_step(theta_mech_);
    break;

  case AxisMode::OpenLoop:
    open_loop_step(static_cast<float>(control.open_loop_speed), dt);
    break;

  case AxisMode::Current:
    current_loop_step(currents, theta_el_, dt);
    break;

  case AxisMode::Position:
    position_loop_step();
    current_loop_step(currents, theta_el_, dt);
    break;

  case AxisMode::Disabled:
  default:
    output_.duty.u = 0.5f;
    output_.duty.v = 0.5f;
    output_.duty.w = 0.5f;
    break;
  }

  return output_;
}

void Axis::reset() {
  d_axis_loop_.reset();
  q_axis_loop_.reset();

  voltage_dq_.d = 0.0f;
  voltage_dq_.q = 0.0f;

  output_.duty.u = 0.5f;
  output_.duty.v = 0.5f;
  output_.duty.w = 0.5f;
}

AxisTelemetry Axis::telemetry() {
  //
  // Member-by-member assignment is intentional.
  // telemetry is volatile, while the FOC math structures are not.
  //

  AxisTelemetry telemetry;
  telemetry.omega_raw = omega_raw_;
  telemetry.omega_mech = omega_mech_;

  telemetry.electrical_offset = electrical_offset_;
  telemetry.open_loop_theta_el = open_loop_theta_el_;

  telemetry.current_dq.d = current_dq_.d;
  telemetry.current_dq.q = current_dq_.q;

  telemetry.voltage_dq.d = voltage_dq_.d;
  telemetry.voltage_dq.q = voltage_dq_.q;

  telemetry.calibration_state = calibration_state_;
  return telemetry;
}
