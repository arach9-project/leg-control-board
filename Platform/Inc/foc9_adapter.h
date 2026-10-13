#ifndef FOC9_ADAPTER_HPP
#define FOC9_ADAPTER_HPP

#include "as5048a/types.h"
#include "foc9/types.hpp"
#include "stm32g4xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { AXIS_INSTANCE_1 = 0, AXIS_INSTANCE_2 = 1, AXIS_INSTANCE_3 = 2 } Axis_Instance_t;

/* Initializes all board-level axis objects. */
HAL_StatusTypeDef axis_init(void);

/* Narrow C facade. Add wrappers only when C code actually needs them. */
HAL_StatusTypeDef axis_init_instance(const Axis_Instance_t instance);

HAL_StatusTypeDef axis_step(const Axis_Instance_t instance, PhaseVector_t currents, float theta,
                            float dt, FocOutput* output);
AS5048A_Angle_t* angleFor(const Axis_Instance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* FOC9_ADAPTER_HPP */
