#ifndef AS5048A_ADAPTER_H
#define AS5048A_ADAPTER_H

#include "as5048a/types.h"
#include "stm32g4xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AS5048A_INSTANCE_1 = 0,
  AS5048A_INSTANCE_2 = 1,
  AS5048A_INSTANCE_3 = 2
} As5048a_Instance_t;

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef as5048a_init(void);

HAL_StatusTypeDef as5048a_init_instance(As5048a_Instance_t instance);

/* -------------------------------------------------------------------------- */
/* Sampling                                                                   */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef as5048a_sample(As5048a_Instance_t instance, AS5048A_Sample_t* sample);

/* -------------------------------------------------------------------------- */
/* Continuous angle acquisition                                               */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef as5048a_begin_continuous_angle_read(As5048a_Instance_t instance);

HAL_StatusTypeDef as5048a_read_next_angle(As5048a_Instance_t instance, AS5048A_Angle_t* angle);

HAL_StatusTypeDef as5048a_is_continuous_angle_read_primed(As5048a_Instance_t instance,
                                                          bool* primed);

/* -------------------------------------------------------------------------- */
/* Zero position                                                              */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef as5048a_read_zero_position(As5048a_Instance_t instance, uint16_t* zero_position);

HAL_StatusTypeDef as5048a_write_zero_position(As5048a_Instance_t instance, uint16_t zero_position);

HAL_StatusTypeDef as5048a_set_current_position_as_zero(As5048a_Instance_t instance);

/* -------------------------------------------------------------------------- */
/* Communication errors                                                       */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef as5048a_clear_communication_errors(As5048a_Instance_t instance,
                                                     uint16_t* raw_errors);

/* -------------------------------------------------------------------------- */
/* Diagnostics                                                                */
/* -------------------------------------------------------------------------- */

/*
 * Return the latest AS5048A error code.
 *
 * The returned value corresponds to as5048a::Error
 * in the C++ driver.
 */
uint32_t as5048a_get_last_error(As5048a_Instance_t instance);

/*
 * Return the latest operation code.
 *
 * The returned value corresponds to as5048a::Operation
 * in the C++ driver.
 */
uint32_t as5048a_get_last_operation(As5048a_Instance_t instance);

/*
 * Return the last STM32 SPI error bitmask.
 */
uint32_t as5048a_get_last_spi_error(As5048a_Instance_t instance);

/*
 * Return the last transmitted SPI frame.
 */
uint16_t as5048a_get_last_tx_frame(As5048a_Instance_t instance);

/*
 * Return the last received SPI frame.
 */
uint16_t as5048a_get_last_rx_frame(As5048a_Instance_t instance);

/*
 * Print pending encoder errors.
 *
 * Call from the main loop, never from the ADC ISR.
 */
void as5048a_print_pending_errors(void);

#ifdef __cplusplus
}
#endif

#endif /* AS5048A_ADAPTER_H */
