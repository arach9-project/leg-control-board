
#ifndef Drv8316_H
#define Drv8316_H

#include "drv8316/drv8316_types.h"
#include "stm32g4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DRV8316_INSTANCE_1 = 0,
    DRV8316_INSTANCE_2 = 1,
    DRV8316_INSTANCE_3 = 2
} Drv8316_Instance_t;

/* Initializes all board-level Drv8316 objects. */
HAL_StatusTypeDef drv8316_init(void);

/* Narrow C facade. Add wrappers only when C code actually needs them. */
HAL_StatusTypeDef drv8316_init_instance(Drv8316_Instance_t instance);
HAL_StatusTypeDef drv8316_clear_faults(Drv8316_Instance_t instance);
HAL_StatusTypeDef
drv8316_refresh_diagnostics(Drv8316_Instance_t instance,
                            DRV8316_Diagnostics_t *diagnostics);
HAL_StatusTypeDef drv8316_set_pwm(Drv8316_Instance_t instance, uint16_t phase_a,
                                  uint16_t phase_b, uint16_t phase_c);
HAL_StatusTypeDef drv8316_set_pwm_mode(Drv8316_Instance_t instance,
                                       DRV8316_PWM_Mode_t mode);
HAL_StatusTypeDef drv8316_set_current_sense_gain(Drv8316_Instance_t instance,
                                                 DRV8316_CSA_Gain_t gain);
void drv8316_wake(Drv8316_Instance_t instance);
void drv8316_sleep(Drv8316_Instance_t instance);

#ifdef __cplusplus
}
#endif

#endif /* Drv8316_H */
