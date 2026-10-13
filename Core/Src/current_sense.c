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

#include "current_sense.h"

#include "adc.h"
#include "foc9/types.hpp"
#include "main.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

#define ADC_VREF 3.3f
#define ADC_RESOLUTION 4095.0f

#define CSA_GAIN 0.15f

#define ADC_TO_VOLTS (ADC_VREF / ADC_RESOLUTION)
#define AMPS_PER_COUNT (ADC_TO_VOLTS / CSA_GAIN)

#define CURRENT_FILTER_ALPHA 0.15f

// Conservative initial validation thresholds.
// Adjust based on measured ADC noise and the actual CSA configuration.
#define ADC_OFFSET_MIN 100.0f
#define ADC_OFFSET_MAX 3995.0f
#define ADC_MAX_STDDEV 12.0f

#define EVENT_STARTED (1UL << 0)
#define EVENT_COMPLETE (1UL << 1)
#define EVENT_FAILED (1UL << 2)

// -----------------------------------------------------------------------------
// Data structures
// -----------------------------------------------------------------------------

typedef struct {
  uint64_t sum_u;
  uint64_t sum_v;

  uint64_t sum_sq_u;
  uint64_t sum_sq_v;

  uint32_t count;

  float mean_u;
  float mean_v;

  float stddev_u;
  float stddev_v;

  bool valid;
} CalibrationAccumulator_t;

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

volatile CurrentSenseState_t current_sense_state = CURRENT_SENSE_IDLE;

static CalibrationAccumulator_t calibration[MOTOR_COUNT];

static CurrentOffset_t current_offsets[MOTOR_COUNT];

static volatile PhaseVector_t motor_currents[MOTOR_COUNT];

static PhaseVector_t filtered_currents[MOTOR_COUNT];

static uint32_t calibration_sample_count = 0;

static volatile uint32_t pending_events = 0;

// -----------------------------------------------------------------------------
// ADC acquisition
// -----------------------------------------------------------------------------

static uint16_t read_raw(ADC_HandleTypeDef* adc, uint32_t motor_index) {
  static const uint32_t ranks[MOTOR_COUNT] = {
      ADC_INJECTED_RANK_1,
      ADC_INJECTED_RANK_2,
      ADC_INJECTED_RANK_3,
  };

  return (uint16_t)HAL_ADCEx_InjectedGetValue(adc, ranks[motor_index]);
}

static uint16_t read_raw_u(uint32_t motor_index) {
  return read_raw(&hadc1, motor_index);
}

static uint16_t read_raw_v(uint32_t motor_index) {
  return read_raw(&hadc2, motor_index);
}

// -----------------------------------------------------------------------------
// DRV8316 current correction
// -----------------------------------------------------------------------------

static void drv8316_current_correction(float* u, float* v, float* w) {
  const float iu = *u;
  const float iv = *v;
  const float iw = *w;

  *u = 0.995832f * iu - 0.028199f * iv - 0.014988f * iw;

  *v = 0.037737f * iu + 1.007723f * iv - 0.033757f * iw;

  *w = 0.009226f * iu + 0.029805f * iv + 1.003268f * iw;
}

// -----------------------------------------------------------------------------
// Calibration
// -----------------------------------------------------------------------------

static void reset_calibration(void) {
  memset(calibration, 0, sizeof(calibration));
  memset(current_offsets, 0, sizeof(current_offsets));
  memset(filtered_currents, 0, sizeof(filtered_currents));

  calibration_sample_count = 0;

  for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
    motor_currents[motor].u = 0.0f;
    motor_currents[motor].v = 0.0f;
    motor_currents[motor].w = 0.0f;
  }
}

void CurrentSense_Init(void) {
  CurrentSense_StartCalibration();
}

void CurrentSense_StartCalibration(void) {
  // Prevent the ADC ISR from accessing partially reset accumulators.
  const uint32_t primask = __get_PRIMASK();

  __disable_irq();

  current_sense_state = CURRENT_SENSE_IDLE;

  reset_calibration();

  current_sense_state = CURRENT_SENSE_CALIBRATING;

  pending_events |= EVENT_STARTED;

  __set_PRIMASK(primask);
}

static float calculate_stddev(uint64_t sum, uint64_t sum_sq, uint32_t count) {
  if (count == 0U) {
    return 0.0f;
  }

  const double mean = (double)sum / (double)count;

  double variance = (double)sum_sq / (double)count - mean * mean;

  if (variance < 0.0) {
    variance = 0.0;
  }

  return (float)sqrt(variance);
}

static bool validate_calibration(const CalibrationAccumulator_t* result) {
  if (result->count != CALIBRATION_SAMPLES) {
    return false;
  }

  if (result->mean_u < ADC_OFFSET_MIN || result->mean_u > ADC_OFFSET_MAX) {
    return false;
  }

  if (result->mean_v < ADC_OFFSET_MIN || result->mean_v > ADC_OFFSET_MAX) {
    return false;
  }

  if (!isfinite(result->stddev_u) || !isfinite(result->stddev_v)) {
    return false;
  }

  if (result->stddev_u > ADC_MAX_STDDEV || result->stddev_v > ADC_MAX_STDDEV) {
    return false;
  }

  return true;
}

static void finish_calibration(void) {
  bool all_valid = true;

  for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
    CalibrationAccumulator_t* result = &calibration[motor];

    result->mean_u = (float)result->sum_u / (float)result->count;

    result->mean_v = (float)result->sum_v / (float)result->count;

    result->stddev_u = calculate_stddev(result->sum_u, result->sum_sq_u, result->count);

    result->stddev_v = calculate_stddev(result->sum_v, result->sum_sq_v, result->count);

    result->valid = validate_calibration(result);

    if (!result->valid) {
      all_valid = false;
      continue;
    }

    current_offsets[motor].u = result->mean_u;
    current_offsets[motor].v = result->mean_v;
  }

  if (all_valid) {
    current_sense_state = CURRENT_SENSE_READY;
    pending_events |= EVENT_COMPLETE;
  } else {
    current_sense_state = CURRENT_SENSE_FAILED;
    pending_events |= EVENT_FAILED;
  }
}

static void accumulate_calibration_sample(void) {
  for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
    const uint32_t u = read_raw_u(motor);
    const uint32_t v = read_raw_v(motor);

    CalibrationAccumulator_t* result = &calibration[motor];

    result->sum_u += u;
    result->sum_v += v;

    result->sum_sq_u += (uint64_t)u * u;
    result->sum_sq_v += (uint64_t)v * v;

    result->count++;
  }

  calibration_sample_count++;

  if (calibration_sample_count >= CALIBRATION_SAMPLES) {
    finish_calibration();
  }
}

// -----------------------------------------------------------------------------
// Current measurement
// -----------------------------------------------------------------------------

static float low_pass(float previous, float input) {
  return previous + CURRENT_FILTER_ALPHA * (input - previous);
}

static void update_phase_currents(void) {
  for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
    const uint16_t raw_u = read_raw_u(motor);
    const uint16_t raw_v = read_raw_v(motor);

    float phase_u = ((float)raw_u - current_offsets[motor].u) * AMPS_PER_COUNT;

    float phase_v = ((float)raw_v - current_offsets[motor].v) * AMPS_PER_COUNT;

    float phase_w = -(phase_u + phase_v);

    drv8316_current_correction(&phase_u, &phase_v, &phase_w);

    PhaseVector_t* filtered = &filtered_currents[motor];

    filtered->u = low_pass(filtered->u, phase_u);
    filtered->v = low_pass(filtered->v, phase_v);
    filtered->w = low_pass(filtered->w, phase_w);

    motor_currents[motor].u = filtered->u;
    motor_currents[motor].v = filtered->v;
    motor_currents[motor].w = filtered->w;
  }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

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

  case CURRENT_SENSE_IDLE:
  case CURRENT_SENSE_FAILED:
  default:
    return HAL_ERROR;
  }
}

HAL_StatusTypeDef CurrentSense_GetCurrents(uint32_t motor_index, PhaseVector_t* currents) {
  if (currents == NULL || motor_index >= MOTOR_COUNT) {
    return HAL_ERROR;
  }

  if (!CurrentSense_IsReady()) {
    return HAL_ERROR;
  }

  currents->u = motor_currents[motor_index].u;
  currents->v = motor_currents[motor_index].v;
  currents->w = motor_currents[motor_index].w;

  return HAL_OK;
}

// -----------------------------------------------------------------------------
// Diagnostics
// -----------------------------------------------------------------------------

void CurrentSense_PrintStatus(void) {
  CalibrationAccumulator_t results[MOTOR_COUNT];

  // Take a consistent snapshot and clear pending events atomically.
  const uint32_t primask = __get_PRIMASK();

  __disable_irq();

  const uint32_t events = pending_events;
  pending_events = 0;

  const CurrentSenseState_t state = current_sense_state;

  if (events & (EVENT_COMPLETE | EVENT_FAILED)) {
    memcpy(results, calibration, sizeof(results));
  }

  __set_PRIMASK(primask);

  if (events == 0U) {
    return;
  }

  if (events & EVENT_STARTED) {
    printf("[CURRENT SENSE] Calibration started (%lu samples per motor)\r\n",
           (unsigned long)CALIBRATION_SAMPLES);
  }

  if (events & (EVENT_COMPLETE | EVENT_FAILED)) {
    printf("\r\n");
    printf("========== CURRENT SENSE CALIBRATION ==========\r\n");

    for (uint32_t motor = 0; motor < MOTOR_COUNT; motor++) {
      const CalibrationAccumulator_t* result = &results[motor];

      printf("Motor %lu: %s\r\n", (unsigned long)(motor + 1U), result->valid ? "PASS" : "FAIL");

      printf("  U offset: %.2f ADC counts (%.4f V)\r\n", result->mean_u,
             result->mean_u * ADC_TO_VOLTS);

      printf("  V offset: %.2f ADC counts (%.4f V)\r\n", result->mean_v,
             result->mean_v * ADC_TO_VOLTS);

      printf("  U noise:  %.3f counts (%.4f A RMS)\r\n", result->stddev_u,
             result->stddev_u * AMPS_PER_COUNT);

      printf("  V noise:  %.3f counts (%.4f A RMS)\r\n", result->stddev_v,
             result->stddev_v * AMPS_PER_COUNT);

      printf("  Samples:  %lu\r\n", (unsigned long)result->count);

      printf("\r\n");
    }

    printf("Result: %s\r\n", (events & EVENT_FAILED) ? "FAILED" : "SUCCESS");

    printf("Current state: %s\r\n", state == CURRENT_SENSE_READY         ? "READY"
                                    : state == CURRENT_SENSE_FAILED      ? "FAILED"
                                    : state == CURRENT_SENSE_CALIBRATING ? "CALIBRATING"
                                                                         : "IDLE");

    printf("===============================================\r\n\r\n");
  }
}
