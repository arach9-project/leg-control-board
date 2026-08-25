#ifndef DRV8316_HPP
#define DRV8316_HPP

#include "drv8316/drv8316_registers.h"
#include "drv8316/drv8316_types.h"
#include "stdint.h"
#include "stm32g4xx_hal.h"

class Drv8316 final {
  public:
    struct Pin {
        GPIO_TypeDef *port{nullptr};
        uint16_t number{0U};

        [[nodiscard]] bool isValid() const noexcept {
            return port != nullptr;
        }
    };

    struct Config {
        SPI_HandleTypeDef *spi{nullptr};
        TIM_HandleTypeDef *timer{nullptr};
        Pin chipSelect{};
        Pin fault{};
        Pin sleep{};
        uint32_t spiTimeoutMs{100U};
    };

    explicit Drv8316(const Config &config) noexcept;

    Drv8316(const Drv8316 &) = delete;
    Drv8316 &operator=(const Drv8316 &) = delete;
    Drv8316(Drv8316 &&) = delete;
    Drv8316 &operator=(Drv8316 &&) = delete;

    HAL_StatusTypeDef initialize();

    HAL_StatusTypeDef refreshDiagnostics();
    [[nodiscard]] const DRV8316_Diagnostics_t &diagnostics() const noexcept;
    [[nodiscard]] bool isFaultPinAsserted() const noexcept;

    void wake() noexcept;
    void sleep() noexcept;

    HAL_StatusTypeDef clearFaults();
    HAL_StatusTypeDef lockRegisters();
    HAL_StatusTypeDef unlockRegisters();

    HAL_StatusTypeDef getRegisterLock(DRV8316_RegisterLock_t &lockStatus);
    HAL_StatusTypeDef setRegisterLock(DRV8316_RegisterLock_t lockStatus);

    HAL_StatusTypeDef getSdoMode(DRV8316_SDO_Mode_t &mode);
    HAL_StatusTypeDef setSdoMode(DRV8316_SDO_Mode_t mode);

    HAL_StatusTypeDef getSlewRate(DRV8316_SlewRate_t &slewRate);
    HAL_StatusTypeDef setSlewRate(DRV8316_SlewRate_t slewRate);

    HAL_StatusTypeDef getPwmMode(DRV8316_PWM_Mode_t &mode);
    HAL_StatusTypeDef setPwmMode(DRV8316_PWM_Mode_t mode);

    HAL_StatusTypeDef
    getOvertemperatureReporting(DRV8316_OTW_Report_t &reporting);
    HAL_StatusTypeDef
    setOvertemperatureReporting(DRV8316_OTW_Report_t reporting);

    HAL_StatusTypeDef getOvervoltageProtection(DRV8316_OVP_Enable_t &enabled);
    HAL_StatusTypeDef setOvervoltageProtection(DRV8316_OVP_Enable_t enabled);

    HAL_StatusTypeDef getOvervoltageLevel(DRV8316_OVP_Level_t &level);
    HAL_StatusTypeDef setOvervoltageLevel(DRV8316_OVP_Level_t level);

    HAL_StatusTypeDef
    getPwm100Frequency(DRV8316_PWM100DutyFrequency_t &frequency);
    HAL_StatusTypeDef
    setPwm100Frequency(DRV8316_PWM100DutyFrequency_t frequency);

    HAL_StatusTypeDef getOcpMode(DRV8316_OCP_Mode_t &mode);
    HAL_StatusTypeDef setOcpMode(DRV8316_OCP_Mode_t mode);

    HAL_StatusTypeDef getOcpLevel(DRV8316_OCP_Level_t &level);
    HAL_StatusTypeDef setOcpLevel(DRV8316_OCP_Level_t level);

    HAL_StatusTypeDef getOcpRetryTime(DRV8316_OCP_RetryTime_t &retryTime);
    HAL_StatusTypeDef setOcpRetryTime(DRV8316_OCP_RetryTime_t retryTime);

    HAL_StatusTypeDef
    getOcpDeglitchTime(DRV8316_OCP_DeglitchTime_t &deglitchTime);
    HAL_StatusTypeDef
    setOcpDeglitchTime(DRV8316_OCP_DeglitchTime_t deglitchTime);

    HAL_StatusTypeDef getOcpCycleByCycle(DRV8316_OCP_CBC_t &enabled);
    HAL_StatusTypeDef setOcpCycleByCycle(DRV8316_OCP_CBC_t enabled);

    HAL_StatusTypeDef getDriverState(DRV8316_DriverState_t &state);
    HAL_StatusTypeDef setDriverState(DRV8316_DriverState_t state);

    HAL_StatusTypeDef getCurrentSenseGain(DRV8316_CSA_Gain_t &gain);
    HAL_StatusTypeDef setCurrentSenseGain(DRV8316_CSA_Gain_t gain);

    HAL_StatusTypeDef getAsr(DRV8316_ASR_Enable_t &enabled);
    HAL_StatusTypeDef setAsr(DRV8316_ASR_Enable_t enabled);

    HAL_StatusTypeDef getAar(DRV8316_AAR_Enable_t &enabled);
    HAL_StatusTypeDef setAar(DRV8316_AAR_Enable_t enabled);

    HAL_StatusTypeDef getIlimRecirculation(DRV8316_ILIM_Recirculation_t &mode);
    HAL_StatusTypeDef setIlimRecirculation(DRV8316_ILIM_Recirculation_t mode);

    HAL_StatusTypeDef getBuckState(DRV8316_BuckDisable_t &state);
    HAL_StatusTypeDef setBuckState(DRV8316_BuckDisable_t state);

    HAL_StatusTypeDef getBuckVoltage(DRV8316_BuckVoltage_t &voltage);
    HAL_StatusTypeDef setBuckVoltage(DRV8316_BuckVoltage_t voltage);

    HAL_StatusTypeDef getBuckCurrentLimit(DRV8316_BuckCurrentLimit_t &limit);
    HAL_StatusTypeDef setBuckCurrentLimit(DRV8316_BuckCurrentLimit_t limit);

    HAL_StatusTypeDef
    getBuckPowerSequencing(DRV8316_BuckPowerSequence_t &state);
    HAL_StatusTypeDef setBuckPowerSequencing(DRV8316_BuckPowerSequence_t state);

    HAL_StatusTypeDef getDelayTarget(DRV8316_DelayTarget_t &target);
    HAL_StatusTypeDef setDelayTarget(DRV8316_DelayTarget_t target);

    HAL_StatusTypeDef
    getDelayCompensation(DRV8316_DelayCompensation_t &enabled);
    HAL_StatusTypeDef setDelayCompensation(DRV8316_DelayCompensation_t enabled);

    HAL_StatusTypeDef setPwm(uint16_t phaseA, uint16_t phaseB, uint16_t phaseC);

  private:
    static void enableCycleCounter() noexcept;
    static void delayMicroseconds(uint32_t microseconds) noexcept;
    static uint8_t countSetBits(uint16_t word) noexcept;
    static bool requiresParityBit(uint16_t frame) noexcept;
    static bool isValueInRange(uint8_t value, uint8_t maximum) noexcept;

    HAL_StatusTypeDef transferFrame(uint16_t transmitFrame,
                                    uint16_t &receiveFrame);
    HAL_StatusTypeDef readRegister(uint8_t address, uint8_t &value);
    HAL_StatusTypeDef writeRegister(uint8_t address, uint8_t value,
                                    uint16_t *receiveFrame = nullptr);
    HAL_StatusTypeDef getField(uint8_t registerAddress, uint8_t mask,
                               uint8_t offset, uint8_t &value);
    HAL_StatusTypeDef setField(uint8_t registerAddress, uint8_t mask,
                               uint8_t offset, uint8_t value);

    Config config_{};
    DRV8316_Diagnostics_t diagnostics_{};
};
#endif // !DRV8316_HPP
