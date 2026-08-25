#include "current_sense.h"

#include "adc.h"
#include "foc9/types.hpp"
#include "main.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define ADC_VREF 3.3f
#define ADC_RESOLUTION 4095.0f

// DRV8316 CSA gain
// You configured DRV8316_CSA_GAIN_0_15V_A
#define CSA_GAIN 0.15f

#define ADC_TO_VOLTS (ADC_VREF / ADC_RESOLUTION)
#define AMPS_PER_COUNT (ADC_TO_VOLTS / CSA_GAIN)

#define CALIBRATION_SAMPLES 2048U

// Low pass filter coefficient
// 0 = no filtering, closer to 1 = heavier filtering
#define CURRENT_FILTER_ALPHA 0.15f

typedef struct {
    uint64_t sum_u;
    uint64_t sum_v;

    float mean_u;
    float mean_v;

    float variance_u;
    float variance_v;

    uint32_t count;
} CalibrationAccumulator_t;

static volatile CurrentSenseState_t current_sense_state =
    CURRENT_SENSE_NOT_CALIBRATED;

static CalibrationAccumulator_t calibration_accumulators[MOTOR_COUNT];

static CurrentOffset_t current_offsets[MOTOR_COUNT];

static volatile PhaseVector_t motor_currents[MOTOR_COUNT];

static PhaseVector_t filtered_currents[MOTOR_COUNT];

static uint32_t calibration_sample_count = 0;

/*
 * Read ADC injected channels.
 * ADC1 = phase U
 * ADC2 = phase V
 */
static uint16_t read_raw_u(uint32_t motor_index) {
    static const uint32_t ranks[MOTOR_COUNT] = {
        ADC_INJECTED_RANK_1, ADC_INJECTED_RANK_2, ADC_INJECTED_RANK_3};

    return HAL_ADCEx_InjectedGetValue(&hadc1, ranks[motor_index]);
}

static uint16_t read_raw_v(uint32_t motor_index) {
    static const uint32_t ranks[MOTOR_COUNT] = {
        ADC_INJECTED_RANK_1, ADC_INJECTED_RANK_2, ADC_INJECTED_RANK_3};

    return HAL_ADCEx_InjectedGetValue(&hadc2, ranks[motor_index]);
}

/*
 * DRV8316 current sense correction.
 *
 * TI recommends compensating cross coupling between
 * phase current measurements.
 */
static void drv8316_current_correction(float *u, float *v, float *w) {
    float iu = *u;
    float iv = *v;
    float iw = *w;

    *u = 0.995832f * iu - 0.028199f * iv - 0.014988f * iw;

    *v = 0.037737f * iu + 1.007723f * iv - 0.033757f * iw;

    *w = 0.009226f * iu + 0.029805f * iv + 1.003268f * iw;
}

static void accumulate_calibration_sample(void) {
    for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
        uint16_t u = read_raw_u(motor);
        uint16_t v = read_raw_v(motor);

        calibration_accumulators[motor].sum_u += u;
        calibration_accumulators[motor].sum_v += v;

        calibration_accumulators[motor].count++;
    }

    calibration_sample_count++;

    if (calibration_sample_count < CALIBRATION_SAMPLES)
        return;

    for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
        calibration_accumulators[motor].mean_u =
            (float)calibration_accumulators[motor].sum_u /
            (float)CALIBRATION_SAMPLES;

        calibration_accumulators[motor].mean_v =
            (float)calibration_accumulators[motor].sum_v /
            (float)CALIBRATION_SAMPLES;

        current_offsets[motor].u = calibration_accumulators[motor].mean_u;

        current_offsets[motor].v = calibration_accumulators[motor].mean_v;

        printf("Current offset motor %lu: U=%f V=%f\r\n", motor,
               current_offsets[motor].u, current_offsets[motor].v);
    }

    printf("ADC count -> amps = %f A/count\r\n", AMPS_PER_COUNT);

    current_sense_state = CURRENT_SENSE_READY;
}

static void update_phase_currents(void) {
    for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
        uint16_t raw_u = read_raw_u(motor);
        uint16_t raw_v = read_raw_v(motor);

        float phase_u =
            ((float)raw_u - current_offsets[motor].u) * AMPS_PER_COUNT;

        float phase_v =
            ((float)raw_v - current_offsets[motor].v) * AMPS_PER_COUNT;

        float phase_w = -(phase_u + phase_v);

        /*
         * Apply DRV8316 correction
         */
        drv8316_current_correction(&phase_u, &phase_v, &phase_w);

        /*
         * Simple IIR filter
         */
        filtered_currents[motor].u +=
            CURRENT_FILTER_ALPHA * (phase_u - filtered_currents[motor].u);

        filtered_currents[motor].v +=
            CURRENT_FILTER_ALPHA * (phase_v - filtered_currents[motor].v);

        filtered_currents[motor].w +=
            CURRENT_FILTER_ALPHA * (phase_w - filtered_currents[motor].w);

        motor_currents[motor] = filtered_currents[motor];
    }
}

void CurrentSense_Init(void) {
    CurrentSense_StartCalibration();

    for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
        motor_currents[motor].u = 0;
        motor_currents[motor].v = 0;
        motor_currents[motor].w = 0;

        filtered_currents[motor].u = 0;
        filtered_currents[motor].v = 0;
        filtered_currents[motor].w = 0;
    }
}

void CurrentSense_StartCalibration(void) {
    printf("Starting current sense calibration\r\n");

    calibration_sample_count = 0;

    for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
        calibration_accumulators[motor].sum_u = 0;
        calibration_accumulators[motor].sum_v = 0;

        current_offsets[motor].u = 0;
        current_offsets[motor].v = 0;
    }

    current_sense_state = CURRENT_SENSE_CALIBRATING;
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

        default:
            return HAL_ERROR;
    }
}

HAL_StatusTypeDef CurrentSense_GetCurrents(uint32_t motor_index,
                                           PhaseVector_t *currents) {
    if (currents == NULL)
        return HAL_ERROR;

    if (motor_index >= MOTOR_COUNT)
        return HAL_ERROR;

    if (current_sense_state != CURRENT_SENSE_READY)
        return HAL_ERROR;

    currents->u = motor_currents[motor_index].u;

    currents->v = motor_currents[motor_index].v;

    currents->w = motor_currents[motor_index].w;

    return HAL_OK;
}
