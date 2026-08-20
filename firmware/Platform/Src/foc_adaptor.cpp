#include "arm_math.h"
#include "as5048a_adapter.h"
#include "current_sense.h"
#include "drv8316/drv8316.hpp"
#include "drv8316_adapter.h"
#include "foc9/foc-controller.hpp"
#include "foc9/types.hpp"
#include "foc_adapter.h"
#include "main.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include <stdio.h>

#define MONITOR 1

// TODO: set this in axis
constexpr float kCurrentLoopPeriod = 0.00005f; // 20 kHz

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
constexpr uint8_t pole_pairs = 7;
volatile float electrical_offset = 0;
volatile bool calibrated = false;

// TODO: Each axis has a contorl loop
FocController foc1 = FocController();

// TODO: Add state machine that i can use to debug the foc loop, either error or
// the current step
// TODO: implement sensorless FOC
// TODO: Function to calibrate motor and get electrical offset, set motor to 0
// rad, read encoder value, calculate offset

extern "C" HAL_StatusTypeDef axis_init() {
    foc1.setVBus(12.0f);
    return HAL_OK;
}

struct PhaseIntVector_t {
    uint16_t u;
    uint16_t v;
    uint16_t w;
};

void duty_to_ccr(PhaseVector_t &duty, PhaseIntVector_t &ccr) {
    uint32_t arr = 4250;
    ccr.u = static_cast<uint16_t>(duty.u * arr + 0.5f);
    ccr.v = static_cast<uint16_t>(duty.v * arr + 0.5f);
    ccr.w = static_cast<uint16_t>(duty.w * arr + 0.5f);
}

#ifdef MONITOR
extern "C" {
volatile float monitor_theta_electrical_offset = 0;
volatile float monitor_theta_mechanical = 0;
volatile uint16_t monitor_u = 0;
volatile uint16_t monitor_v = 0;
volatile uint16_t monitor_w = 0;
};
#endif // MONITOR

extern "C" HAL_StatusTypeDef axis_full_rotation() {
    PhaseVector_t duty;
    PhaseIntVector_t ccr;
    DirectQuadratureFrame_t V_dq = {0, 0};
    float32_t theta_align = 0;
    AS5048A_Sample_t sample{};
    float measured_theta_el;
    float encoder_direction = -1.0f;

    for (float theta_el = 0.0f; theta_el < 2.0f * M_PI; theta_el += 0.01f) {
        DirectQuadratureFrame_t V_dq = {.d = 1.5f, .q = 0.0f};

        foc1.backward(V_dq, theta_el, duty);
        duty_to_ccr(duty, ccr);

        drv8316_set_pwm(DRV8316_INSTANCE_1, ccr.u, ccr.v, ccr.w);

        if (as5048a_sample(AS5048A_INSTANCE_1, &sample) != HAL_OK) {
            return HAL_ERROR;
        };

        measured_theta_el =
            wrap_0_2pi(encoder_direction * pole_pairs * sample.angle.radians -
                       electrical_offset);
        // printf("%0.4f, %0.4f\r\n", measured_theta_el, sample.angle.radians);

        HAL_Delay(5);
    }

    V_dq = {0, 0};
    foc1.backward(V_dq, theta_align, duty);
    duty_to_ccr(duty, ccr);

    if (drv8316_set_pwm(DRV8316_INSTANCE_1, ccr.u, ccr.v, ccr.w) != HAL_OK) {
        return HAL_ERROR;
    };
    return HAL_OK;
}
extern "C" HAL_StatusTypeDef axis_calibrate_electrical_offset() {
    printf("Calibrating electrical offset for Axis %d\r\n", 1);
    PhaseVector_t duty;
    PhaseIntVector_t ccr;
    DirectQuadratureFrame_t V_dq = {0, 0};
    float32_t theta_align = 0;
    AS5048A_Sample_t sample{};
    float encoder_direction = -1.0f;

    for (size_t i = 0; i < 100; ++i) {
        V_dq.d += 1.0f / 100.0f;
        foc1.backward(V_dq, theta_align, duty);
        duty_to_ccr(duty, ccr);
        if (drv8316_set_pwm(DRV8316_INSTANCE_1, ccr.u, ccr.v, ccr.w) !=
            HAL_OK) {
            return HAL_ERROR;
        };
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
        sum_sin += sinf(sample.angle.radians);
        sum_cos += cosf(sample.angle.radians);
        HAL_Delay(2);
    }

    float mean_sin = sum_sin / 100.0f;
    float mean_cos = sum_cos / 100.0f;

    float resultant = sqrtf(mean_sin * mean_sin + mean_cos * mean_cos);
    float theta_mech = atan2f(sum_sin, sum_cos);
    if (theta_mech < 0.0f)
        theta_mech += TWO_PI;

    electrical_offset =
        wrap_0_2pi(encoder_direction * pole_pairs * theta_mech - theta_align);

    printf("%0.4f,%.4f,%.4f\r\n", resultant, theta_mech, electrical_offset);

    V_dq = {0, 0};
    foc1.backward(V_dq, theta_align, duty);
    duty_to_ccr(duty, ccr);

    if (drv8316_set_pwm(DRV8316_INSTANCE_1, ccr.u, ccr.v, ccr.w) != HAL_OK) {
        return HAL_ERROR;
    };

    calibrated = true;
    return HAL_OK;
}

#define MIN_DUTY 0.02f
#define MAX_DUTY 0.98f

extern "C" HAL_StatusTypeDef axis_current_loop_step() {
    if (!calibrated) {
        return HAL_OK;
    }

    float theta_target = 0.5;
    PhaseCurrents_t currents{};

    if (CurrentSense_GetCurrents(0, &currents) != HAL_OK) {
        printf("Error with current sense\r\n");
        // axis0->disable();
        // drv1.sleep();
        return HAL_ERROR;
    }
    // TODO: Check if encoder object exists
    // TODO: Check if encoder continous mode initiated
    // TODO: Make seperate loop for each axis, make this encapsulated in axis
    AS5048A_Angle_t angle = {0, 0, 0};
    // TODO: Measure how much angle measurement over SPI takes

    if (as5048a_read_next_angle(AS5048A_INSTANCE_1, &angle)) {
        Error_Handler();
    };

    float encoder_direction = -1.0f;
    float theta_el = wrap_0_2pi(encoder_direction * angle.radians * pole_pairs -
                                electrical_offset);

    float position_error = wrap_minus_pi_pi(theta_target - angle.radians);

    // float velocity_error = target_velocity - measured_velocity;

    float iq_ref = kp * position_error - kd * angular_velocity;

    iq_ref = clamp(iq_ref, -iq_limit, iq_limit);

    // float iq_ref = 1.0f;
    PhaseVector_t I = {.u = currents.u, .v = currents.v, .w = currents.w};
    FocInput input{.I = I,
                   .theta_el = theta_el,
                   .id_ref = 0.0f,
                   .iq_ref = iq_ref,
                   .vbus = 12.0f};

    FocOutput output = foc1.step(&input, kCurrentLoopPeriod);
    output.duty.u = clamp(output.duty.u, MIN_DUTY, MAX_DUTY);
    output.duty.v = clamp(output.duty.v, MIN_DUTY, MAX_DUTY);
    output.duty.w = clamp(output.duty.w, MIN_DUTY, MAX_DUTY);
    // CCR_x = d_x * ARR
    // TODO: Get period programatically and set in Axis
    uint32_t arr = 4250;
    uint16_t ccr_u = output.duty.u * arr, ccr_v = output.duty.v * arr,
             ccr_w = output.duty.w * arr;

    printf("%d, %d, %d\r\n", ccr_u, ccr_v, ccr_w);

    return HAL_OK;
    // TODO: CHange set_pwm to set_ccr
    drv8316_set_pwm(DRV8316_INSTANCE_1, ccr_u, ccr_w, ccr_v);
}
