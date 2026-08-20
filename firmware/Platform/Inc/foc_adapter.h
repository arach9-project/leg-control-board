#ifndef FOC_ADAPTER_HPP
#define FOC_ADAPTER_HPP

#include "stm32g4xx_hal.h"
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

HAL_StatusTypeDef axis_current_loop_step(void);

HAL_StatusTypeDef axis_calibrate_electrical_offset();

HAL_StatusTypeDef axis_full_rotation();

HAL_StatusTypeDef axis_init();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !FOC_ADAPTER_HPP
