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

extern "C" HAL_StatusTypeDef drv8316_wake(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);
    if (driver != nullptr) {
        driver->wake();
    }
    return HAL_OK;
}

extern "C" HAL_StatusTypeDef drv8316_sleep(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);
    if (driver != nullptr) {
        driver->sleep();
    }
    return HAL_OK;
}

extern "C" HAL_StatusTypeDef
drv8316_get_pwm_mode(const Drv8316_Instance_t instance,
                     DRV8316_PWM_Mode_t *const mode) {
    if (mode == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getPwmMode(*mode) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_slew_rate(const Drv8316_Instance_t instance,
                      const DRV8316_SlewRate_t slew_rate) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setSlewRate(slew_rate) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_slew_rate(const Drv8316_Instance_t instance,
                      DRV8316_SlewRate_t *const slew_rate) {
    if (slew_rate == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getSlewRate(*slew_rate) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_sdo_mode(const Drv8316_Instance_t instance,
                     const DRV8316_SDO_Mode_t mode) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setSdoMode(mode) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_sdo_mode(const Drv8316_Instance_t instance,
                     DRV8316_SDO_Mode_t *const mode) {
    if (mode == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getSdoMode(*mode) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_current_sense_gain(const Drv8316_Instance_t instance,
                               DRV8316_CSA_Gain_t *const gain) {
    if (gain == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getCurrentSenseGain(*gain) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_asr(const Drv8316_Instance_t instance,
                const DRV8316_ASR_Enable_t enabled) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setAsr(enabled) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_asr(const Drv8316_Instance_t instance,
                DRV8316_ASR_Enable_t *const enabled) {
    if (enabled == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getAsr(*enabled) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_aar(const Drv8316_Instance_t instance,
                const DRV8316_AAR_Enable_t enabled) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setAar(enabled) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_aar(const Drv8316_Instance_t instance,
                DRV8316_AAR_Enable_t *const enabled) {
    if (enabled == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getAar(*enabled) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_ilim_recirculation(const Drv8316_Instance_t instance,
                               const DRV8316_ILIM_Recirculation_t mode) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setIlimRecirculation(mode) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_ilim_recirculation(const Drv8316_Instance_t instance,
                               DRV8316_ILIM_Recirculation_t *const mode) {
    if (mode == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getIlimRecirculation(*mode)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_delay_compensation(const Drv8316_Instance_t instance,
                               const DRV8316_DelayCompensation_t enabled) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setDelayCompensation(enabled)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_delay_compensation(const Drv8316_Instance_t instance,
                               DRV8316_DelayCompensation_t *const enabled) {
    if (enabled == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getDelayCompensation(*enabled)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_delay_target(const Drv8316_Instance_t instance,
                         const DRV8316_DelayTarget_t target) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setDelayTarget(target) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_delay_target(const Drv8316_Instance_t instance,
                         DRV8316_DelayTarget_t *const target) {
    if (target == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getDelayTarget(*target) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_driver_state(const Drv8316_Instance_t instance,
                         const DRV8316_DriverState_t state) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setDriverState(state) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_driver_state(const Drv8316_Instance_t instance,
                         DRV8316_DriverState_t *const state) {
    if (state == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getDriverState(*state) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_ocp_mode(const Drv8316_Instance_t instance,
                     const DRV8316_OCP_Mode_t mode) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setOcpMode(mode) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_ocp_mode(const Drv8316_Instance_t instance,
                     DRV8316_OCP_Mode_t *const mode) {
    if (mode == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getOcpMode(*mode) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_ocp_level(const Drv8316_Instance_t instance,
                      const DRV8316_OCP_Level_t level) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setOcpLevel(level) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_ocp_level(const Drv8316_Instance_t instance,
                      DRV8316_OCP_Level_t *const level) {
    if (level == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getOcpLevel(*level) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_ocp_retry_time(const Drv8316_Instance_t instance,
                           const DRV8316_OCP_RetryTime_t retry_time) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setOcpRetryTime(retry_time)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_ocp_retry_time(const Drv8316_Instance_t instance,
                           DRV8316_OCP_RetryTime_t *const retry_time) {
    if (retry_time == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getOcpRetryTime(*retry_time)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_ocp_deglitch_time(const Drv8316_Instance_t instance,
                              const DRV8316_OCP_DeglitchTime_t deglitch_time) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setOcpDeglitchTime(deglitch_time)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_ocp_deglitch_time(const Drv8316_Instance_t instance,
                              DRV8316_OCP_DeglitchTime_t *const deglitch_time) {
    if (deglitch_time == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getOcpDeglitchTime(*deglitch_time)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_ocp_cycle_by_cycle(const Drv8316_Instance_t instance,
                               const DRV8316_OCP_CBC_t enabled) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setOcpCycleByCycle(enabled)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_ocp_cycle_by_cycle(const Drv8316_Instance_t instance,
                               DRV8316_OCP_CBC_t *const enabled) {
    if (enabled == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getOcpCycleByCycle(*enabled)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_overtemperature_reporting(const Drv8316_Instance_t instance,
                                      const DRV8316_OTW_Report_t reporting) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setOvertemperatureReporting(reporting)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_overtemperature_reporting(const Drv8316_Instance_t instance,
                                      DRV8316_OTW_Report_t *const reporting) {
    if (reporting == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getOvertemperatureReporting(*reporting)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_overvoltage_protection(const Drv8316_Instance_t instance,
                                   const DRV8316_OVP_Enable_t enabled) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setOvervoltageProtection(enabled)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_overvoltage_protection(const Drv8316_Instance_t instance,
                                   DRV8316_OVP_Enable_t *const enabled) {
    if (enabled == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getOvervoltageProtection(*enabled)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_overvoltage_level(const Drv8316_Instance_t instance,
                              const DRV8316_OVP_Level_t level) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setOvervoltageLevel(level) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_overvoltage_level(const Drv8316_Instance_t instance,
                              DRV8316_OVP_Level_t *const level) {
    if (level == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getOvervoltageLevel(*level)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_pwm100_frequency(const Drv8316_Instance_t instance,
                             const DRV8316_PWM100DutyFrequency_t frequency) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setPwm100Frequency(frequency)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_pwm100_frequency(const Drv8316_Instance_t instance,
                             DRV8316_PWM100DutyFrequency_t *const frequency) {
    if (frequency == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getPwm100Frequency(*frequency)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_buck_state(const Drv8316_Instance_t instance,
                       const DRV8316_BuckDisable_t state) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setBuckState(state) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_buck_state(const Drv8316_Instance_t instance,
                       DRV8316_BuckDisable_t *const state) {
    if (state == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getBuckState(*state) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_buck_voltage(const Drv8316_Instance_t instance,
                         const DRV8316_BuckVoltage_t voltage) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setBuckVoltage(voltage) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_buck_voltage(const Drv8316_Instance_t instance,
                         DRV8316_BuckVoltage_t *const voltage) {
    if (voltage == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getBuckVoltage(*voltage) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_buck_current_limit(const Drv8316_Instance_t instance,
                               const DRV8316_BuckCurrentLimit_t limit) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setBuckCurrentLimit(limit) : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_buck_current_limit(const Drv8316_Instance_t instance,
                               DRV8316_BuckCurrentLimit_t *const limit) {
    if (limit == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getBuckCurrentLimit(*limit)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_set_buck_power_sequencing(const Drv8316_Instance_t instance,
                                  const DRV8316_BuckPowerSequence_t state) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setBuckPowerSequencing(state)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_buck_power_sequencing(const Drv8316_Instance_t instance,
                                  DRV8316_BuckPowerSequence_t *const state) {
    if (state == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getBuckPowerSequencing(*state)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_lock_registers(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->lockRegisters() : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_unlock_registers(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->unlockRegisters() : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
drv8316_get_register_lock(const Drv8316_Instance_t instance,
                          DRV8316_RegisterLock_t *const lock_status) {
    if (lock_status == nullptr) {
        return HAL_ERROR;
    }

    Drv8316 *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->getRegisterLock(*lock_status)
                               : HAL_ERROR;
}

extern "C" uint8_t
drv8316_is_fault_asserted(const Drv8316_Instance_t instance) {
    Drv8316 *const driver = driverFor(instance);

    if (driver == nullptr) {
        return 0U;
    }

    return driver->isFaultPinAsserted() ? 1U : 0U;
}
