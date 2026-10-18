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

// TODO: Change this when you add more motors
#define MOTOR_COUNT 3U
#define CALIBRATION_SAMPLES 1000U

typedef struct {
  float u;
  float v;
} CurrentOffset_t;

typedef enum {
  CURRENT_SENSE_IDLE = 0,
  CURRENT_SENSE_CALIBRATING,
  CURRENT_SENSE_READY,
  CURRENT_SENSE_FAILED,
} CurrentSenseState_t;

extern volatile CurrentSenseState_t current_sense_state;

void CurrentSense_Init(void);
void CurrentSense_StartCalibration(void);

bool CurrentSense_IsReady(void);

HAL_StatusTypeDef CurrentSense_ProcessAdcISR(void);
HAL_StatusTypeDef CurrentSense_GetCurrents(uint32_t motor_index, volatile PhaseVector_t* currents);

// Call from the main loop, never from an ISR.
void CurrentSense_PrintStatus(void);

#ifdef __cplusplus
}
#endif

#endif
