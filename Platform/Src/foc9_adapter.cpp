#include "foc9_adapter.h"
#include "foc9/axis.hpp"
#include "foc9/types.hpp"
#include "foc9_adapter.hpp"

#include "stdio.h"
#include "stm32g4xx_hal_def.h"

/*
 * Board-specific wiring belongs here, not in Components/axis.
 * Rename the DRV1/DRV2/DRV3 GPIO labels below to your CubeMX labels.
 * The examples assume three separate PWM timers and a shared SPI bus.
 */

Axis_Config_t axis1_cfg{.kIqLimit = 0.5f,
                        .kIdLimit = 0.2f,
                        .kPositionLoopDt = 0.001f,
                        .kPositionDeadband = 0.005f,
                        .kVelocityDeadband = 0.05f,
                        .kVelocityAlpha = 1.0f,
                        .pole_pairs = 7,
                        .kPositionKp = 2.0f,
                        .kPositionKd = 0.03f,
                        .vbus = 12.0f,
                        .kCalibrationVoltage = 1.0f,
                        .kThetaAlign = 0.0f,
                        .kRampCycles = 2000,
                        .kSettleCycles = 16000,
                        .kSampleCycles = 100};

Axis_Config_t axis2_cfg{.kIqLimit = 0.5f,
                        .kIdLimit = 0.2f,
                        .kPositionLoopDt = 0.001f,
                        .kPositionDeadband = 0.005f,
                        .kVelocityDeadband = 0.05f,
                        .kVelocityAlpha = 1.0f,
                        .pole_pairs = 7,
                        .kPositionKp = 2.0f,
                        .kPositionKd = 0.03f,
                        .vbus = 12.0f,
                        .kCalibrationVoltage = 1.0f,
                        .kThetaAlign = 0.0f,
                        .kRampCycles = 2000,
                        .kSettleCycles = 16000,
                        .kSampleCycles = 100};

Axis_Config_t axis3_cfg{.kIqLimit = 0.5f,
                        .kIdLimit = 0.2f,
                        .kPositionLoopDt = 0.001f,
                        .kPositionDeadband = 0.005f,
                        .kVelocityDeadband = 0.05f,
                        .kVelocityAlpha = 1.0f,
                        .pole_pairs = 7,
                        .kPositionKp = 2.0f,
                        .kPositionKd = 0.03f,
                        .vbus = 12.0f,
                        .kCalibrationVoltage = 1.0f,
                        .kThetaAlign = 0.0f,
                        .kRampCycles = 2000,
                        .kSettleCycles = 16000,
                        .kSampleCycles = 100};

Axis axis1{axis1_cfg};
Axis axis2{axis2_cfg};
Axis axis3{axis3_cfg};

namespace {
Axis* axisFor(const Axis_Instance_t instance) noexcept {
  switch (instance) {
  case AXIS_INSTANCE_1:
    return &axis1;
  case AXIS_INSTANCE_2:
    return &axis2;
  case AXIS_INSTANCE_3:
    return &axis3;
  default:
    return nullptr;
  }
}
} // namespace

extern "C" HAL_StatusTypeDef axis_init(void) {
  printf("axis Init\r\n");
  axis1.init();
  return HAL_OK;
}

extern "C" HAL_StatusTypeDef axis_init_instance(const Axis_Instance_t instance) {
  Axis* const axis = axisFor(instance);
  axis->init();
  return HAL_OK;
}

extern "C" HAL_StatusTypeDef axis_step(const Axis_Instance_t instance, float theta, float dt,
                                       FocOutput* output) {
  Axis* const axis = axisFor(instance);
  *output = axis->step(theta, dt);
  return HAL_OK;
}
