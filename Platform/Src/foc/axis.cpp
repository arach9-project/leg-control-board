#include "adc.h"
#include "arm_math.h"
#include "as5048a_adapter.h"
#include "current_sense.h"
#include "drv8316/drv8316.hpp"
#include "drv8316_adapter.h"
#include "foc/pi_controller.hpp"
#include "foc/types.hpp"
#include "main.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include <stdio.h>

// TODO: set this in axis

// encapsulate this in axis, pole pair count and electircal offset

struct Axis_Config_t {
  float kIqLimit = 0.5f; // A
  float kIdLimit = 0.2f; // A
  float kPositionLoopDt = 0.001f;
  float kPositionDeadband = 0.005f; // ~0.29 deg
  float kVelocityDeadband = 0.05f;  // rad/s
  float kVelocityAlpha = 1.0f;
  uint8_t pole_pairs = 7;
  volatile float kPositionKp = 2.0f;
  volatile float kPositionKd = 0.03f;
  float vbus = 12.0f;
};

class Axis {
private:
  PIController d_axis_loop_, q_axis_loop_;

  volatile uint8_t position_loop_on = false;
  volatile uint8_t current_loop_on = false;
  volatile uint8_t axis_calibrated = false;
  volatile float theta_target, theta_el, theta_mech, omega_mech, omega_raw;
  volatile float prev_theta_mech;

  float encoder_direction = 1.0f;
  volatile float electrical_offset = 0;
  float _current_loop_period;

public:
  Axis_Config_t cfg;
  Axis()
      : d_axis_loop_(PIController(cfg.vbus, 0.5f, 100.0f)),
        q_axis_loop_(PIController(cfg.vbus, 0.5f, 100.0f)) {
    _current_loop_period = calculate_loop_freq();
  }

  void test_angle(float theta_el) {
    DirectQuadratureFrame_t vdq = {
        .d = 1.5f,
        .q = 0.0f,
    };

    backward(vdq, theta_el, foc_output.duty);
  }

  float calculate_loop_freq() {
    const uint32_t timer_clock = HAL_RCC_GetPCLK2Freq();
    const uint32_t psc = TIM1->PSC;
    const uint32_t arr = TIM1->ARR;

    const float pwm_frequency = static_cast<float>(timer_clock) /
                                (2.0f * static_cast<float>(psc + 1) * static_cast<float>(arr));

    return 1.0f / pwm_frequency;
  }
  void open_loop(float rotations, uint32_t delay_ms) {

    constexpr float voltage = 1.5f;
    constexpr float step = 0.01f;

    const float theta_end = rotations * TWO_PI * static_cast<float>(pole_pairs);

    for (float theta = 0.0f; theta < theta_end; theta += step) {

      DirectQuadratureFrame_t V_dq = {
          .d = voltage,
          .q = 0.0f,
      };

      theta_el = wrap_0_2pi(theta);

      foc.backward(V_dq, foc_input.theta_el, foc_output.duty);

      HAL_Delay(delay_ms);
    }

    foc_output.duty = {
        .u = 0.5f,
        .v = 0.5f,
        .w = 0.5f,
    };
  }
  HAL_StatusTypeDef axis_calibrate_electrical_offset() {
    DirectQuadratureFrame_t V_dq = {0, 0};
    float32_t theta_align = 0;
    AS5048A_Sample_t sample{};

    for (size_t i = 0; i < 100; ++i) {
      V_dq.d += 1.0f / 100.0f;
      foc.backward(V_dq, theta_align, foc_output.duty);
      HAL_Delay(10);
    }

    HAL_Delay(800);

    // Averaging sum of encoder sample angles
    float sum_sin = 0.0f;
    float sum_cos = 0.0f;
    for (int i = 0; i < 100; ++i) {
      if (as5048a_sample(AS5048A_INSTANCE_1, &sample) != HAL_OK) {
        return HAL_ERROR;
      };
      theta_mech = sample.angle.radians;
      sum_sin += sinf(theta_mech);
      sum_cos += cosf(theta_mech);
      HAL_Delay(2);
    }

    theta_mech = atan2f(sum_sin, sum_cos);
    if (theta_mech < 0.0f)
      theta_mech += TWO_PI;

    electrical_offset = wrap_0_2pi(encoder_direction * pole_pairs * theta_mech - theta_align);

    V_dq = {0, 0};
    foc.backward(V_dq, theta_align, foc_output.duty);
    axis_calibrated = true;
    return HAL_OK;
  }
  HAL_StatusTypeDef axis_position_loop_step() {

    static bool was_position_loop_on = false;

    if (!position_loop_on) {
      if (was_position_loop_on) {
        foc.reset();

        // Optional: zero command once when leaving position mode
        foc_input.iq_ref = 0.0f;
      }

      was_position_loop_on = false;

      prev_theta_mech = theta_mech;
      omega_raw = 0.0f;
      omega_mech = 0.0f;

      return HAL_OK;
    }

    was_position_loop_on = true;

    float position_error = wrap_minus_pi_pi(theta_target - theta_mech);

    if (fabsf(position_error) < kPositionDeadband) {
      position_error = 0.0f;
    }

    float delta_theta = wrap_minus_pi_pi(theta_mech - prev_theta_mech);

    omega_raw = delta_theta / kPositionLoopDt;
    omega_mech += kVelocityAlpha * (omega_raw - omega_mech);

    if (fabsf(omega_mech) < kVelocityDeadband) {
      omega_mech = 0.0f;
    }

    float iq_command = kPositionKp * position_error - kPositionKd * omega_mech;

    foc_input.iq_ref = clamp(iq_command, -kIqLimit, kIqLimit);

    prev_theta_mech = theta_mech;

    return HAL_OK;
  }
  HAL_StatusTypeDef axis_current_loop_step() {
    AS5048A_Angle_t angle = {0, 0, 0};

    if (!axis_calibrated) {
      return HAL_OK;
    }
    if (motor_headless_rotating) {
      return HAL_OK;
    }
    return HAL_OK;

    theta_mech = wrap_0_2pi(encoder_direction * angle.radians);
    foc_input.theta_el =
        wrap_0_2pi(encoder_direction * angle.radians * pole_pairs - electrical_offset);

    foc_input.vel_el = 0.0f;
    foc_input.id_ref = clamp(0.0f, -kIdLimit, kIdLimit),
    foc_input.iq_ref = clamp(foc_input.iq_ref, -kIqLimit, kIqLimit), foc_input.vbus = 12.0f;

    if (!current_loop_on) {
      foc_output.duty = {0.5, 0.5, 0.5};
      return HAL_OK;
    }
    foc_output = foc.step(foc_input, kCurrentLoopPeriod);
    return HAL_OK;
  }

  const FocOutput step(FocInput& input, float dt) {

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

  HAL_StatusTypeDef axis_pwm_step() {
    PhaseIntVector_t ccr;

    duty_to_ccr(foc_output.duty, ccr);
    if (drv8316_set_pwm(DRV8316_INSTANCE_1, ccr.u, ccr.v, ccr.w) != HAL_OK)
      return HAL_ERROR;
    return HAL_OK;
  }

  void reset() {
    d_axis_loop_.reset();
    q_axis_loop_.reset();
  }
};

// TODO:
// - [ ] Simplify FOC code and remove adapter convention, make foc math one file
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
