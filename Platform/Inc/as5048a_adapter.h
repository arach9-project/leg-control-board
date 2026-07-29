#ifndef As5048a_H
#define As5048a_H
#include "as5048a/types.h"
#include "stm32g4xx_hal.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    AS5048A_INSTANCE_1 = 0,
    AS5048A_INSTANCE_2 = 1,
    AS5048A_INSTANCE_3 = 2
} As5048a_Instance_t;

/* Initializes all board-level As5048a objects. */
HAL_StatusTypeDef as5048a_init(void);

/* Narrow C facade. Add wrappers only when C code actually needs them. */
HAL_StatusTypeDef as5048a_init_instance(As5048a_Instance_t instance);

HAL_StatusTypeDef as5048a_sample(As5048a_Instance_t instance,
                                 AS5048A_Sample_t *sample);

HAL_StatusTypeDef
as5048a_begin_continuous_angle_read(As5048a_Instance_t instance);
HAL_StatusTypeDef as5048a_read_next_angle(As5048a_Instance_t instance,
                                          AS5048A_Angle_t *angle);

HAL_StatusTypeDef as5048a_read_zero_position(As5048a_Instance_t instance,
                                             uint16_t *zero_position);
HAL_StatusTypeDef as5048a_write_zero_position(As5048a_Instance_t instance,
                                              uint16_t zero_position);
HAL_StatusTypeDef
as5048a_set_current_position_as_zero(As5048a_Instance_t instance);

HAL_StatusTypeDef
as5048a_clear_communication_errors(As5048a_Instance_t instance,
                                   uint16_t *raw_errors);
#ifdef __cplusplus
}
#endif
#endif /* As5048a_H */
