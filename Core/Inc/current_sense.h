#ifndef CURRENT_SENSE_H
#define CURRENT_SENSE_H

#include "foc9/types.hpp"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define MOTOR_COUNT 3U
#define CALIBRATION_SAMPLES 1000U

typedef struct {
    float u;
    float v;
} CurrentOffset_t;

typedef enum {
    CURRENT_SENSE_NOT_CALIBRATED = 0,
    CURRENT_SENSE_CALIBRATING,
    CURRENT_SENSE_READY,
    CURRENT_SENSE_ERROR
} CurrentSenseState_t;

void CurrentSense_Init(void);

void CurrentSense_StartCalibration(void);

bool CurrentSense_IsReady(void);

HAL_StatusTypeDef CurrentSense_ProcessAdcISR(void);

HAL_StatusTypeDef CurrentSense_GetCurrents(uint32_t motor_index,
                                           PhaseVector_t *currents);

#ifdef __cplusplus
}
#endif

#endif
