#include "drv8316_adapter.h"
#include "drv8316/drv8316.hpp"
#include "drv8316_adapter.hpp"

#include "main.h"
#include "spi.h"
#include "stdio.h"
#include "stm32g4xx_hal_def.h"
#include "tim.h"

/*
 * Board-specific wiring belongs here, not in Components/DRV8316.
 * Rename the DRV1/DRV2/DRV3 GPIO labels below to your CubeMX labels.
 * The examples assume three separate PWM timers and a shared SPI bus.
 */

Drv8316::Config drv1_cfg{&hspi1,
                         &htim1,
                         {M0_nSCS_GPIO_Port, M0_nSCS_Pin},
                         {M0_nFAULT_GPIO_Port, M0_nFAULT_Pin},
                         {M0_nSLEEP_GPIO_Port, M0_nSLEEP_Pin},
                         100U};

Drv8316::Config drv2_cfg{&hspi1,
                         &htim2,
                         {M1_nSCS_GPIO_Port, M1_nSCS_Pin},
                         {M1_nFAULT_GPIO_Port, M1_nFAULT_Pin},
                         {M1_nSLEEP_GPIO_Port, M1_nSLEEP_Pin},
                         100U};

Drv8316::Config drv3_cfg{&hspi1,
                         &htim8,
                         {M2_nSCS_GPIO_Port, M2_nSCS_Pin},
                         {M2_nFAULT_GPIO_Port, M2_nFAULT_Pin},
                         {M2_nSLEEP_GPIO_Port, M2_nSLEEP_Pin},
                         100U};

Drv8316 drv1{drv1_cfg};
Drv8316 drv2{drv2_cfg};
Drv8316 drv3{drv3_cfg};

namespace {
Drv8316 *driverFor(const Drv8316_Instance_t instance) noexcept {
    switch (instance) {
        case DRV8316_INSTANCE_1:
            return &drv1;
        case DRV8316_INSTANCE_2:
            return &drv2;
        case DRV8316_INSTANCE_3:
            return &drv3;
        default:
            return nullptr;
    }
}
} // namespace

extern "C" HAL_StatusTypeDef drv8316_init(void) {
    printf("DRV8316 Init\r\n");
    HAL_StatusTypeDef status = drv1.initialize();
    if (status != HAL_OK) {
        return status;
    }
    return HAL_OK;

    // status = drv2.initialize();
    // if (status != HAL_OK) {
    //     return status;
    // }
    //
    // return drv3.initialize();
}

extern "C" HAL_StatusTypeDef
drv8316_init_instance(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->initialize() : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_clear_faults(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->clearFaults() : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_refresh_diagnostics(const Drv8316_Instance_t instance,
                            DRV8316_Diagnostics_t *const diagnostics) {
    if (diagnostics == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    if (driver == nullptr) {
        return HAL_ERROR;
    }

    const HAL_StatusTypeDef status = driver->refreshDiagnostics();
    if (status == HAL_OK) {
        *diagnostics = driver->diagnostics();
    }
    return status;
}

extern "C" HAL_StatusTypeDef drv8316_set_pwm(const Drv8316_Instance_t instance,
                                             const uint16_t phase_a,
                                             const uint16_t phase_b,
                                             const uint16_t phase_c) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setPwm(phase_a, phase_b, phase_c)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_pwm_mode(const Drv8316_Instance_t instance,
                     const DRV8316_PWM_Mode_t mode) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setPwmMode(mode) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_current_sense_gain(const Drv8316_Instance_t instance,
                               const DRV8316_CSA_Gain_t gain) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setCurrentSenseGain(gain) : HAL_ERROR;
}

extern "C" void drv8316_wake(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);
    if (driver != nullptr) {
        driver->wake();
    }
}

extern "C" void drv8316_sleep(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);
    if (driver != nullptr) {
        driver->sleep();
    }
}
