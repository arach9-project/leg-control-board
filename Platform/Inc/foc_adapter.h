#ifndef FOC_ADAPTER_HPP
#define FOC_ADAPTER_HPP

#include "foc9/types.hpp"
#include "stm32g4xx_hal.h"

#ifdef __cplusplus

extern "C" {
#endif // __cplusplus

extern volatile uint8_t position_loop_on;
extern volatile uint8_t current_loop_on;
extern volatile uint8_t axis_calibrated;
extern volatile uint8_t motor_headless_rotating;
extern volatile float theta_target;
extern volatile uint8_t drv8316_on; // 0x20007C00 (1 byte)

HAL_StatusTypeDef axis_current_loop_step(void);

HAL_StatusTypeDef axis_position_loop_step(void);

HAL_StatusTypeDef axis_calibrate_electrical_offset();

HAL_StatusTypeDef axis_full_rotation(float rotation, uint32_t delay);

HAL_StatusTypeDef axis_init();

HAL_StatusTypeDef axis_pwm_step();

void axis_test_angle(float theta_el);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !FOC_ADAPTER_HPP
