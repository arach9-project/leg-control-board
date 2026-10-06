#ifndef FOC9_AXIS_HPP
#define FOC9_AXIS_HPP

#include <foc9/pi_controller.hpp>
#include <foc9/types.hpp>
#include <stdint.h>

// TODO: set this in axis

// encapsulate this in axis, pole pair count and electircal offset

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

enum class CalibrationState {
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

class Axis {
private:
  Axis_Config_t cfg;
  PIController d_axis_loop_, q_axis_loop_;
  volatile AxisMode axis_mode = AxisMode::Disabled;

  uint8_t axis_calibrated = false;
  float theta_target, id_ref, iq_ref, omega_raw, omega_mech;
  float prev_theta_mech;

  float encoder_direction = 1.0f;
  volatile float electrical_offset = 0;
  FocOutput _output;

  CalibrationState calibration_state = CalibrationState::Idle;

  uint32_t calibration_counter = 0;

  float calibration_sum_sin = 0.0f;
  float calibration_sum_cos = 0.0f;
  float open_loop_speed = 10.0f;
  PhaseVector_t I;
  DirectQuadratureFrame_t I_dq = {0, 0};
  DirectQuadratureFrame_t V_dq = {0, 0};
  PhaseVector_t duty = {0, 0, 0};

public:
  Axis(Axis_Config_t cfg);

  void init();
  void test_angle(float theta);
  void start_electrical_offset_calibration();
  void calibration_step(float theta_mech);
  void position_loop_step(float theta_mech, float dt);
  void enter_position_mode(float theta_mech);
  void current_loop_step(float theta_el, float dt);
  float open_loop_theta_el = 0.0f;
  void open_loop_step(float mechanical_speed_rad_s, float dt);
  const FocOutput step(float theta, float dt);

  void reset();
};

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

#endif // !FOC9_AXIS_HPP
