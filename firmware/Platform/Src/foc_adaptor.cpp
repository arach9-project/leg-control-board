#include "as5048a/as5048a.hpp"
#include "current_sense.h"
#include "drv8316/drv8316.hpp"
#include "foc9/foc-controller.hpp"
#include "foc9/types.hpp"
#include "foc_adapter.h"
#include "main.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include <stdio.h>

// TODO: Move to own driver
struct PositionEstimate {
    float theta_el;
    bool valid;
};

extern Drv8316 drv1;

extern As5048a encoder1;

constexpr float kCurrentLoopPeriod = 0.00005f; // 20 kHz

constexpr uint8_t pole_pairs = 7;
constexpr uint8_t electrical_offset = 0;
#define ELECTRICAL_OFFSET 0
FocController foc1 = FocController();

// TODO: Add state machine that i can use to debug the foc loop, either error or
// the current step
// TODO: implement sensorless FOC
// TODO: Function to calibrate motor and get electrical offset, set motor to 0
// rad, read encoder value, calculate offset

// float calibrateElectricalOffset()
// {
//     constexpr float alignment_angle = 0.0f;
//     constexpr float alignment_voltage = 1.0f;
//
//     // Produce a stationary rotor field:
//     // vd = alignment_voltage
//     // vq = 0
//     // theta_el = alignment_angle
//     foc1.applyVoltageVector(
//         alignment_voltage,
//         0.0f,
//         alignment_angle,
//         measured_vbus);
//
//     HAL_Delay(1000);
//
//     const float theta_mech =
//         encoder.getMechanicalAngleRad();
//
//     disableMotorPwm();
//
//     const float offset = wrap_0_2pi(
//         alignment_angle -
//         motor_direction * 7.0f * theta_mech);
//
//     return offset;
// }
float wrap_0_2pi(float angle) {
    while (angle >= TWO_PI) {
        angle -= TWO_PI;
    }

    while (angle < 0.0f) {
        angle += TWO_PI;
    }

    return angle;
}

extern "C" void MotorControl_CurrentLoopStepISR() {
    PhaseCurrents_t currents{};
    if (CurrentSense_GetCurrents(0, &currents) != HAL_OK) {
        printf("Error with current sense\r\n");
        // axis0->disable();
        // drv1.sleep();
        return;
    }
    // const As5048a::Sample position = encoder1.sample();
    // if (!position.valid) {
    // }

    float theta_el = 0;
    float omega_el = 2.0f;
    // theta_el = wrap_0_2pi(pole_pairs * position.angle_rad *
    // electrical_offset);

    theta_el += omega_el * kCurrentLoopPeriod;
    theta_el = wrap_0_2pi(theta_el);

    float target_iq = 1.0f;
    PhaseVector_t I = {.u = currents.u, .v = currents.v, .w = currents.w};
    FocInput input{.I = I,
                   .theta_el = theta_el,
                   .id_ref = 0.0f,
                   .iq_ref = target_iq,
                   .vbus = 12.0f};

    const FocOutput output = foc1.step(&input, kCurrentLoopPeriod);
    // printf("%0.2f, %0.2f, %0.2f\r\n", output.duty.u, output.duty.v,
    //        output.duty.w);

    drv1.setPwm(output.duty.u, output.duty.v, output.duty.w);
}
