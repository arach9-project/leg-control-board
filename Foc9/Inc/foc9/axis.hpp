#ifndef FOC9_AXIS_HPP
#define FOC9_AXIS_HPP

#include <foc9/pi_controller.hpp>
#include <foc9/types.hpp>
#include <stdint.h>

struct Axis_Config_t {
  float kIqLimit;
  float kIdLimit;
  float kPositionLoopDt;
  float kPositionDeadband;
  float kVelocityDeadband;
  float kVelocityAlpha;

  uint8_t pole_pairs;

  float kPositionKp;
  float kPositionKd;

  float vbus;

  float kCalibrationVoltage;
  float kThetaAlign;

  uint32_t kRampCycles;
  uint32_t kSettleCycles;
  uint32_t kSampleCycles;
};

enum class CalibrationState : uint8_t {
  Idle,
  RampAlignment,
  Settle,
  Sample,
  Complete,
};

enum class AxisMode : uint8_t {
  Disabled,
  Calibration,
  OpenLoop,
  Current,
  Position,
};

//
// Debugger / external control interface
//
struct AxisControl {
  float theta_target;
  float id_ref;
  float iq_ref;
  float open_loop_speed;

  float position_kp;
  float position_kd;

  AxisMode axis_mode;
};

//
// Read-only debugger telemetry
//
struct AxisTelemetry {
  uint8_t axis_calibrated;

  float theta_mech;
  float omega_raw;
  float omega_mech;

  float electrical_offset;
  float open_loop_theta_el;

  DirectQuadratureFrame_t current_dq;
  DirectQuadratureFrame_t voltage_dq;

  CalibrationState calibration_state;
};

class Axis {
public:
  // Shared with debugger / SWD.
  AxisControl control;

private:
  Axis_Config_t config;
  PIController d_axis_loop_;
  PIController q_axis_loop_;

  float id_error_{0.0f};
  float iq_error_{0.0f};

  AxisMode previous_mode_{AxisMode::Disabled};

  //
  // Internal computational state.
  // These are deliberately NOT volatile.
  //
  float prev_theta_mech_ = 0.0f;

  float electrical_offset_ = 0.0f;
  float encoder_direction_ = 1.0f;
  float theta_mech_{0.0f};
  float theta_el_{0.0f};

  float omega_raw_ = 0.0f;
  float omega_mech_ = 0.0f;

  float open_loop_theta_el_ = 0.0f;

  // PhaseVector_t current_ = {0.0f, 0.0f, 0.0f};

  DirectQuadratureFrame_t current_dq_ = {
      .d = 0.0f,
      .q = 0.0f,
  };

  DirectQuadratureFrame_t voltage_dq_ = {
      .d = 0.0f,
      .q = 0.0f,
  };

  FocOutput output_{};

  CalibrationState calibration_state_ = CalibrationState::Idle;

  uint32_t calibration_counter_ = 0;

  float calibration_sum_sin_ = 0.0f;
  float calibration_sum_cos_ = 0.0f;

  AxisTelemetry telemetry();

  void update_velocity(float theta_mech, float dt);

public:
  explicit Axis(Axis_Config_t cfg);

  void init();

  void test_angle(float theta);

  void start_electrical_offset_calibration();
  void calibration_step(float theta_mech);

  void position_loop_step();
  void enter_position_mode(float theta_mech);

  void current_loop_step(PhaseVector_t currents, float theta_el, float dt);

  void open_loop_step(float mechanical_speed_rad_s, float dt);

  FocOutput step(PhaseVector_t currents, float theta, float dt);

  void reset();
};

#endif
