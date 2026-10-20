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

#define IQ_LIMIT 2.0f
#define ID_LIMIT 2.0f
#define CALIBRATION_VOLTAGE 2.0f
#define VBUS 12.0f
#define RAMP_CYCLES 1000
#define SETTLE_CYCLES 2000
#define SAMPLE_CYCLES 100

Axis_Config_t axis1_cfg{
    .kIqLimit = IQ_LIMIT,
    .kIdLimit = ID_LIMIT,
    .kPositionLoopDt = 0.001f,
    .kPositionDeadband = 0.005f,
    .kVelocityDeadband = 0.05f,
    .kVelocityAlpha = 1.0f,
    .kCurrentKp = 0.5f,
    .kCurrentKi = 0.0f,
    .pole_pairs = 7,
    .kPositionKp = 2.0f,
    .kPositionKd = 0.03f,
    .vbus = VBUS,
    .kCalibrationVoltage = CALIBRATION_VOLTAGE,
    .kThetaAlign = 0.0f,
    .kRampCycles = RAMP_CYCLES,
    .kSettleCycles = SETTLE_CYCLES,
    .kSampleCycles = SAMPLE_CYCLES,
};

Axis_Config_t axis2_cfg{
    .kIqLimit = IQ_LIMIT,
    .kIdLimit = ID_LIMIT,
    .kPositionLoopDt = 0.001f,
    .kPositionDeadband = 0.005f,
    .kVelocityDeadband = 0.05f,
    .kVelocityAlpha = 1.0f,
    .kCurrentKp = 0.5f,
    .kCurrentKi = 0.0f,
    .pole_pairs = 7,
    .kPositionKp = 2.0f,
    .kPositionKd = 0.03f,
    .vbus = VBUS,
    .kCalibrationVoltage = CALIBRATION_VOLTAGE,
    .kThetaAlign = 0.0f,
    .kRampCycles = RAMP_CYCLES,
    .kSettleCycles = SETTLE_CYCLES,
    .kSampleCycles = SAMPLE_CYCLES,

};

Axis_Config_t axis3_cfg{
    .kIqLimit = IQ_LIMIT,
    .kIdLimit = ID_LIMIT,
    .kPositionLoopDt = 0.001f,
    .kPositionDeadband = 0.005f,
    .kVelocityDeadband = 0.05f,
    .kVelocityAlpha = 1.0f,
    .kCurrentKp = 0.5f,
    .kCurrentKi = 0.0f,
    .pole_pairs = 7,
    .kPositionKp = 2.0f,
    .kPositionKd = 0.03f,
    .vbus = VBUS,
    .kCalibrationVoltage = CALIBRATION_VOLTAGE,
    .kThetaAlign = 0.0f,
    .kRampCycles = RAMP_CYCLES,
    .kSettleCycles = SETTLE_CYCLES,
    .kSampleCycles = SAMPLE_CYCLES,

};

Axis axis1{axis1_cfg};
Axis axis2{axis2_cfg};
Axis axis3{axis3_cfg};

AS5048A_Angle_t axis1_angle = {0, 0, 0};
AS5048A_Angle_t axis2_angle = {0, 0, 0};
AS5048A_Angle_t axis3_angle = {0, 0, 0};

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

extern "C" HAL_StatusTypeDef axis_init_instance(const Axis_Instance_t instance) {
  Axis* const axis = axisFor(instance);
  axis->init();
  return HAL_OK;
}

extern "C" HAL_StatusTypeDef axis_step(const Axis_Instance_t instance, PhaseVector_t currents,
                                       float theta, float dt, FocOutput* output) {
  Axis* const axis = axisFor(instance);
  *output = axis->step(currents, theta, dt);
  return HAL_OK;
}

extern "C" AS5048A_Angle_t* angleFor(const Axis_Instance_t instance) noexcept {
  switch (instance) {
  case AXIS_INSTANCE_1:
    return &axis1_angle;
  case AXIS_INSTANCE_2:
    return &axis2_angle;
  case AXIS_INSTANCE_3:
    return &axis3_angle;
  default:
    return nullptr;
  }
}
