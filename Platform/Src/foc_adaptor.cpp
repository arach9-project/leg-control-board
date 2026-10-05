#include "adc.h"
#include "arm_math.h"
#include "as5048a_adapter.h"
#include "current_sense.h"
#include "drv8316/drv8316.hpp"
#include "drv8316_adapter.h"
#include "foc9/foc-controller.hpp"
#include "foc9/types.hpp"
#include "foc_adapter.h"
#include "foc_adapter.hpp"
#include "main.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include <stdio.h>

// TODO: set this in axis
constexpr float kCurrentLoopPeriod = 0.00005f; // 20 kHz
extern "C" {
volatile uint8_t position_loop_on = false; // 0x20007C00 (1 byte)
volatile uint8_t current_loop_on = false;  // 0x20007C00 (1 byte)
volatile uint8_t drv8316_on = false;       // 0x20007C00 (1 byte)
volatile float theta_mech;                 // 0x20007C0C (4 bytes)
volatile float omega_raw;                  // 0x20007C10 (4 bytes)
volatile float omega_mech;                 // 0x20007C14 (4 bytes)
volatile float theta_target;               // 0x20007C18 (4 bytes)
volatile float prev_theta_mech;            // 0x20007C20 (4 bytes)
// volatile PhaseVector_t monitor_duty;
volatile float kPositionKp = 2.0f;
volatile float kPositionKd = 0.03f;
constexpr float kVelocityAlpha = 1.0f;
FocInput foc_input;
FocOutput foc_output;
}
constexpr float kIqLimit = 0.5f; // A
constexpr float kIdLimit = 0.2f; // A
constexpr float kPositionLoopDt = 0.001f;
constexpr float kPositionDeadband = 0.005f; // ~0.29 deg
constexpr float kVelocityDeadband = 0.05f;  // rad/s
constexpr uint8_t pole_pairs = 7;

constexpr float encoder_direction = 1.0f;
volatile float electrical_offset = 0;
volatile uint8_t axis_calibrated = false;
volatile uint8_t motor_headless_rotating = false;

#define MIN_DUTY 0.02f
#define MAX_DUTY 0.98f
#define ARR 4250

struct PhaseIntVector_t {
  uint16_t u;
  uint16_t v;
  uint16_t w;
};

template <typename T>
constexpr T clamp(const T value, const T minimum, const T maximum) {
  return (value < minimum) ? minimum : (value > maximum) ? maximum : value;
};

float wrap_0_2pi(float angle) {
  while (angle >= TWO_PI) {
    angle -= TWO_PI;
  }

  while (angle < 0.0f) {
    angle += TWO_PI;
  }

  return angle;
}

float wrap_minus_pi_pi(float angle) {
  while (angle >= PI) {
    angle -= TWO_PI;
  }

  while (angle < -PI) {
    angle += TWO_PI;
  }

  return angle;
}

// encapsulate this in axis, pole pair count and electircal offset

FocController foc1 = FocController();

extern "C" void axis_test_angle(float theta_el) {
  DirectQuadratureFrame_t vdq = {
      .d = 1.5f,
      .q = 0.0f,
  };

  foc1.backward(vdq, theta_el, foc_output.duty);
}
extern "C" HAL_StatusTypeDef axis_init() {
  foc1.setVBus(12.0f);
  return HAL_OK;
}

void duty_to_ccr(PhaseVector_t &duty, PhaseIntVector_t &ccr) {
  float du = clamp(duty.u, MIN_DUTY, MAX_DUTY);
  float dv = clamp(duty.v, MIN_DUTY, MAX_DUTY);
  float dw = clamp(duty.w, MIN_DUTY, MAX_DUTY);

  ccr.u = static_cast<uint16_t>(du * ARR + 0.5f);
  ccr.v = static_cast<uint16_t>(dv * ARR + 0.5f);
  ccr.w = static_cast<uint16_t>(dw * ARR + 0.5f);
}

extern "C" HAL_StatusTypeDef axis_full_rotation(float rotations,
                                                uint32_t delay_ms) {

  constexpr float voltage = 1.5f;
  constexpr float step = 0.01f;

  const float theta_end = rotations * TWO_PI * static_cast<float>(pole_pairs);

  for (float theta = 0.0f; theta < theta_end; theta += step) {

    DirectQuadratureFrame_t V_dq = {
        .d = voltage,
        .q = 0.0f,
    };

    foc_input.theta_el = wrap_0_2pi(PI / 2);

    foc1.backward(V_dq, foc_input.theta_el, foc_output.duty);

    HAL_Delay(delay_ms);
  }

  foc_output.duty = {
      .u = 0.5f,
      .v = 0.5f,
      .w = 0.5f,
  };

  return HAL_OK;
}

extern "C" HAL_StatusTypeDef axis_calibrate_electrical_offset() {
  printf("Calibrating electrical offset for Axis %d\r\n", 1);
  PhaseIntVector_t ccr;
  DirectQuadratureFrame_t V_dq = {0, 0};
  float32_t theta_align = 0;
  AS5048A_Sample_t sample{};

  for (size_t i = 0; i < 100; ++i) {
    V_dq.d += 1.0f / 100.0f;
    foc1.backward(V_dq, theta_align, foc_output.duty);
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

  electrical_offset =
      wrap_0_2pi(encoder_direction * pole_pairs * theta_mech - theta_align);

  V_dq = {0, 0};
  foc1.backward(V_dq, theta_align, foc_output.duty);
  axis_calibrated = true;
  return HAL_OK;
}

extern "C" HAL_StatusTypeDef axis_position_loop_step() {

  static bool was_position_loop_on = false;

  if (!position_loop_on) {
    if (was_position_loop_on) {
      foc1.reset();

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

extern "C" HAL_StatusTypeDef axis_current_loop_step() {
  AS5048A_Angle_t angle = {0, 0, 0};

  if (!axis_calibrated) {
    return HAL_OK;
  }
  if (motor_headless_rotating) {
    return HAL_OK;
  }
  return HAL_OK;

  if (as5048a_read_next_angle(AS5048A_INSTANCE_1, &angle))
    return HAL_ERROR;

  if (CurrentSense_GetCurrents(0, &foc_input.I) != HAL_OK) {
    return HAL_ERROR;
  }

  theta_mech = wrap_0_2pi(encoder_direction * angle.radians);
  foc_input.theta_el = wrap_0_2pi(
      encoder_direction * angle.radians * pole_pairs - electrical_offset);

  foc_input.vel_el = 0.0f;
  foc_input.id_ref = clamp(0.0f, -kIdLimit, kIdLimit),
  foc_input.iq_ref = clamp(foc_input.iq_ref, -kIqLimit, kIqLimit),
  foc_input.vbus = 12.0f;

  if (!current_loop_on) {
    foc_output.duty = {0.5, 0.5, 0.5};
    return HAL_OK;
  }
  foc_output = foc1.step(foc_input, kCurrentLoopPeriod);
  return HAL_OK;
}

// command motor loop
HAL_StatusTypeDef axis_pwm_step() {
  PhaseIntVector_t ccr;

  duty_to_ccr(foc_output.duty, ccr);
  if (drv8316_set_pwm(DRV8316_INSTANCE_1, ccr.u, ccr.v, ccr.w) != HAL_OK)
    return HAL_ERROR;
  return HAL_OK;
}

// TODO:
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
