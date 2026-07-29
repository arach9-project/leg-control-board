
#include "current_sense.h"

#include "adc.h"
#include "main.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"

#include <stddef.h>
#include <stdio.h>

#ifndef AMPS_PER_COUNT
#define AMPS_PER_COUNT 0.01f
#endif

typedef struct {
    uint64_t sum_u;
    uint64_t sum_v;
} CalibrationAccumulator_t;

static volatile CurrentSenseState_t current_sense_state =
    CURRENT_SENSE_NOT_CALIBRATED;

static uint32_t calibration_sample_count = 0U;

static CalibrationAccumulator_t calibration_accumulators[MOTOR_COUNT];
static CurrentOffset_t current_offsets[MOTOR_COUNT];

/*
 * Written by the ADC ISR and potentially read outside the ISR.
 * Volatile prevents the compiler from caching these values.
 */
static volatile PhaseCurrents_t motor_currents[MOTOR_COUNT];

static uint16_t read_raw_u(uint32_t motor_index) {
    static const uint32_t injected_ranks[MOTOR_COUNT] = {
        ADC_INJECTED_RANK_1, ADC_INJECTED_RANK_2, ADC_INJECTED_RANK_3};

    return (uint16_t)HAL_ADCEx_InjectedGetValue(&hadc1,
                                                injected_ranks[motor_index]);
}

static uint16_t read_raw_v(uint32_t motor_index) {
    static const uint32_t injected_ranks[MOTOR_COUNT] = {
        ADC_INJECTED_RANK_1, ADC_INJECTED_RANK_2, ADC_INJECTED_RANK_3};

    return (uint16_t)HAL_ADCEx_InjectedGetValue(&hadc2,
                                                injected_ranks[motor_index]);
}

static void accumulate_calibration_sample(void) {
    for (uint32_t motor = 0U; motor < MOTOR_COUNT; ++motor) {
        calibration_accumulators[motor].sum_u += read_raw_u(motor);
        calibration_accumulators[motor].sum_v += read_raw_v(motor);
    }

    ++calibration_sample_count;

    if (calibration_sample_count < CALIBRATION_SAMPLES) {
        return;
    }

    const float inverse_sample_count = 1.0f / (float)calibration_sample_count;

    for (uint32_t motor = 0U; motor < MOTOR_COUNT; ++motor) {
        current_offsets[motor].u =
            (float)calibration_accumulators[motor].sum_u * inverse_sample_count;

        current_offsets[motor].v =
            (float)calibration_accumulators[motor].sum_v * inverse_sample_count;
    }

    current_sense_state = CURRENT_SENSE_READY;
}

static void update_phase_currents(void) {
    for (uint32_t motor = 0U; motor < MOTOR_COUNT; ++motor) {
        const uint16_t raw_u = read_raw_u(motor);
        const uint16_t raw_v = read_raw_v(motor);

        const float phase_u =
            ((float)raw_u - current_offsets[motor].u) * AMPS_PER_COUNT;

        const float phase_v =
            ((float)raw_v - current_offsets[motor].v) * AMPS_PER_COUNT;

        motor_currents[motor].u = phase_u;
        motor_currents[motor].v = phase_v;
        motor_currents[motor].w = -(phase_u + phase_v);
    }
}

void CurrentSense_Init(void) {
    CurrentSense_StartCalibration();

    for (uint32_t motor = 0U; motor < MOTOR_COUNT; ++motor) {
        motor_currents[motor].u = 0.0f;
        motor_currents[motor].v = 0.0f;
        motor_currents[motor].w = 0.0f;
    }
}

void CurrentSense_StartCalibration(void) {
    printf("Starting ADC Calibration\r\n");
    calibration_sample_count = 0U;

    for (uint32_t motor = 0U; motor < MOTOR_COUNT; ++motor) {
        calibration_accumulators[motor].sum_u = 0U;
        calibration_accumulators[motor].sum_v = 0U;

        current_offsets[motor].u = 0.0f;
        current_offsets[motor].v = 0.0f;
    }

    current_sense_state = CURRENT_SENSE_CALIBRATING;
    printf("Finished ADC Calibration\r\n");
}

bool CurrentSense_IsReady(void) {
    return current_sense_state == CURRENT_SENSE_READY;
}

HAL_StatusTypeDef CurrentSense_ProcessAdcISR(void) {
    switch (current_sense_state) {
        case CURRENT_SENSE_CALIBRATING:
            accumulate_calibration_sample();
            return HAL_OK;

        case CURRENT_SENSE_READY:
            update_phase_currents();
            return HAL_OK;

        case CURRENT_SENSE_NOT_CALIBRATED:
        case CURRENT_SENSE_ERROR:
        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef CurrentSense_GetCurrents(uint32_t motor_index,
                                           PhaseCurrents_t *currents) {
    if (currents == NULL) {
        return HAL_ERROR;
    }

    if (motor_index >= MOTOR_COUNT) {
        return HAL_ERROR;
    }

    if (current_sense_state != CURRENT_SENSE_READY) {
        return HAL_ERROR;
    }

    /*
     * These values are written in the ISR. If the foreground code can
     * preempt the write, protect this copy with a short critical section.
     */
    currents->u = motor_currents[motor_index].u;
    currents->v = motor_currents[motor_index].v;
    currents->w = motor_currents[motor_index].w;

    return HAL_OK;
}
