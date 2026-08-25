#include "drv8316/drv8316.hpp"

Drv8316::Drv8316(const Config &config) noexcept : config_{config} {
}

HAL_StatusTypeDef Drv8316::initialize() {
    if ((config_.spi == nullptr) || !config_.chipSelect.isValid() ||
        !config_.sleep.isValid()) {
        return HAL_ERROR;
    }

    enableCycleCounter();

    HAL_GPIO_WritePin(config_.chipSelect.port, config_.chipSelect.number,
                      GPIO_PIN_SET);
    wake();
    HAL_Delay(2U);

    HAL_StatusTypeDef status = unlockRegisters();
    if (status != HAL_OK) {
        return status;
    }

    status = setDriverState(DRV8316_DRIVER_ACTIVE);
    if (status != HAL_OK) {
        return status;
    }

    status = clearFaults();
    if (status != HAL_OK) {
        return status;
    }

    status = refreshDiagnostics();
    if (status != HAL_OK) {
        return status;
    }

    return setCurrentSenseGain(DRV8316_CSA_GAIN_0_15V_A);
}

HAL_StatusTypeDef Drv8316::refreshDiagnostics() {
    HAL_StatusTypeDef status =
        readRegister(DRV8316_REG_IC_STATUS, diagnostics_.ic_status);
    if (status != HAL_OK) {
        return status;
    }

    if ((diagnostics_.ic_status & DRV8316_IC_STATUS_DETAIL_REQUIRED_MASK) !=
        0U) {
        status = readRegister(DRV8316_REG_STATUS_1, diagnostics_.status_1);
        if (status != HAL_OK) {
            return status;
        }

        status = readRegister(DRV8316_REG_STATUS_2, diagnostics_.status_2);
        if (status != HAL_OK) {
            return status;
        }
    } else {
        diagnostics_.status_1 = 0U;
        diagnostics_.status_2 = 0U;
    }

    return HAL_OK;
}

const DRV8316_Diagnostics_t &Drv8316::diagnostics() const noexcept {
    return diagnostics_;
}

bool Drv8316::isFaultPinAsserted() const noexcept {
    if (!config_.fault.isValid()) {
        return false;
    }

    return HAL_GPIO_ReadPin(config_.fault.port, config_.fault.number) ==
           GPIO_PIN_RESET;
}

void Drv8316::wake() noexcept {
    if (config_.sleep.isValid()) {
        HAL_GPIO_WritePin(config_.sleep.port, config_.sleep.number,
                          GPIO_PIN_SET);
    }
}

void Drv8316::sleep() noexcept {
    if (config_.sleep.isValid()) {
        HAL_GPIO_WritePin(config_.sleep.port, config_.sleep.number,
                          GPIO_PIN_RESET);
    }
}

void Drv8316::enableCycleCounter() noexcept {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void Drv8316::delayMicroseconds(const uint32_t microseconds) noexcept {
    const uint32_t cyclesPerMicrosecond = SystemCoreClock / 1'000'000UL;
    const uint32_t requiredCycles = cyclesPerMicrosecond * microseconds;
    const uint32_t start = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < requiredCycles) {
    }
}

uint8_t Drv8316::countSetBits(uint16_t word) noexcept {
    uint8_t count = 0U;

    while (word != 0U) {
        count = static_cast<uint8_t>(count + (word & 1U));
        word >>= 1U;
    }

    return count;
}

bool Drv8316::requiresParityBit(const uint16_t frame) noexcept {
    return (countSetBits(frame) & 1U) != 0U;
}

bool Drv8316::isValueInRange(const uint8_t value,
                             const uint8_t maximum) noexcept {
    return value <= maximum;
}

HAL_StatusTypeDef Drv8316::transferFrame(const uint16_t transmitFrame,
                                         uint16_t &receiveFrame) {
    if ((config_.spi == nullptr) || !config_.chipSelect.isValid()) {
        receiveFrame = 0U;
        return HAL_ERROR;
    }

    if (HAL_SPI_GetState(config_.spi) != HAL_SPI_STATE_READY) {
        receiveFrame = 0U;
        return HAL_BUSY;
    }

    uint8_t transmitBytes[2]{
        static_cast<uint8_t>((transmitFrame >> 8U) & 0xFFU),
        static_cast<uint8_t>(transmitFrame & 0xFFU)};
    uint8_t receiveBytes[2]{0U, 0U};

    HAL_GPIO_WritePin(config_.chipSelect.port, config_.chipSelect.number,
                      GPIO_PIN_RESET);

    const HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        config_.spi, transmitBytes, receiveBytes, 2U, config_.spiTimeoutMs);

    if (status == HAL_OK) {
        while (__HAL_SPI_GET_FLAG(config_.spi, SPI_FLAG_BSY) != RESET) {
        }
    }

    HAL_GPIO_WritePin(config_.chipSelect.port, config_.chipSelect.number,
                      GPIO_PIN_SET);

    delayMicroseconds(1U);

    if (status != HAL_OK) {
        receiveFrame = 0U;
        return status;
    }

    receiveFrame =
        static_cast<uint16_t>((static_cast<uint16_t>(receiveBytes[0]) << 8U) |
                              static_cast<uint16_t>(receiveBytes[1]));

    return HAL_OK;
}

HAL_StatusTypeDef Drv8316::readRegister(const uint8_t address, uint8_t &value) {
    uint16_t transmitFrame =
        static_cast<uint16_t>((1U << 15U) | ((address & 0x3FU) << 9U));

    if (requiresParityBit(transmitFrame)) {
        transmitFrame |= (1U << 8U);
    }

    uint16_t receiveFrame = 0U;
    const HAL_StatusTypeDef status = transferFrame(transmitFrame, receiveFrame);
    if (status != HAL_OK) {
        return status;
    }

    value = static_cast<uint8_t>(receiveFrame & 0xFFU);
    return HAL_OK;
}

HAL_StatusTypeDef Drv8316::writeRegister(const uint8_t address,
                                         const uint8_t value,
                                         uint16_t *const receiveFrame) {
    uint16_t transmitFrame =
        static_cast<uint16_t>(((address & 0x3FU) << 9U) | value);

    if (requiresParityBit(transmitFrame)) {
        transmitFrame |= (1U << 8U);
    }

    uint16_t localReceiveFrame = 0U;
    const HAL_StatusTypeDef status =
        transferFrame(transmitFrame, localReceiveFrame);
    if (status != HAL_OK) {
        return status;
    }

    if (receiveFrame != nullptr) {
        *receiveFrame = localReceiveFrame;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Drv8316::getField(const uint8_t registerAddress,
                                    const uint8_t mask, const uint8_t offset,
                                    uint8_t &value) {
    uint8_t registerValue = 0U;
    const HAL_StatusTypeDef status =
        readRegister(registerAddress, registerValue);
    if (status != HAL_OK) {
        return status;
    }

    value = static_cast<uint8_t>((registerValue & mask) >> offset);
    return HAL_OK;
}

HAL_StatusTypeDef Drv8316::setField(const uint8_t registerAddress,
                                    const uint8_t mask, const uint8_t offset,
                                    const uint8_t value) {
    uint8_t registerValue = 0U;
    HAL_StatusTypeDef status = readRegister(registerAddress, registerValue);
    if (status != HAL_OK) {
        return status;
    }

    registerValue =
        static_cast<uint8_t>(registerValue & static_cast<uint8_t>(~mask));
    registerValue =
        static_cast<uint8_t>(registerValue | ((value << offset) & mask));

    return writeRegister(registerAddress, registerValue);
}

HAL_StatusTypeDef
Drv8316::getOvertemperatureReporting(DRV8316_OTW_Report_t &reporting) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_3, DRV8316_CTRL3_OTW_REP_MASK,
                 DRV8316_CTRL3_OTW_REP_OFFSET, value);
    if (status == HAL_OK) {
        reporting = static_cast<DRV8316_OTW_Report_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setOvertemperatureReporting(const DRV8316_OTW_Report_t reporting) {
    if (!isValueInRange(static_cast<uint8_t>(reporting), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_3, DRV8316_CTRL3_OTW_REP_MASK,
                    DRV8316_CTRL3_OTW_REP_OFFSET,
                    static_cast<uint8_t>(reporting));
}

HAL_StatusTypeDef
Drv8316::getOvervoltageProtection(DRV8316_OVP_Enable_t &enabled) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_3, DRV8316_CTRL3_OVP_EN_MASK,
                 DRV8316_CTRL3_OVP_EN_OFFSET, value);
    if (status == HAL_OK) {
        enabled = static_cast<DRV8316_OVP_Enable_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setOvervoltageProtection(const DRV8316_OVP_Enable_t enabled) {
    if (!isValueInRange(static_cast<uint8_t>(enabled), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_3, DRV8316_CTRL3_OVP_EN_MASK,
                    DRV8316_CTRL3_OVP_EN_OFFSET, static_cast<uint8_t>(enabled));
}

HAL_StatusTypeDef Drv8316::getOvervoltageLevel(DRV8316_OVP_Level_t &level) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_3, DRV8316_CTRL3_OVP_SEL_MASK,
                 DRV8316_CTRL3_OVP_SEL_OFFSET, value);
    if (status == HAL_OK) {
        level = static_cast<DRV8316_OVP_Level_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setOvervoltageLevel(const DRV8316_OVP_Level_t level) {
    if (!isValueInRange(static_cast<uint8_t>(level), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_3, DRV8316_CTRL3_OVP_SEL_MASK,
                    DRV8316_CTRL3_OVP_SEL_OFFSET, static_cast<uint8_t>(level));
}

HAL_StatusTypeDef
Drv8316::getPwm100Frequency(DRV8316_PWM100DutyFrequency_t &frequency) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_3, DRV8316_CTRL3_PWM_100_DUTY_MASK,
                 DRV8316_CTRL3_PWM_100_DUTY_OFFSET, value);
    if (status == HAL_OK) {
        frequency = static_cast<DRV8316_PWM100DutyFrequency_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setPwm100Frequency(const DRV8316_PWM100DutyFrequency_t frequency) {
    if (!isValueInRange(static_cast<uint8_t>(frequency), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_3, DRV8316_CTRL3_PWM_100_DUTY_MASK,
                    DRV8316_CTRL3_PWM_100_DUTY_OFFSET,
                    static_cast<uint8_t>(frequency));
}

HAL_StatusTypeDef Drv8316::getOcpMode(DRV8316_OCP_Mode_t &mode) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_MODE_MASK,
                 DRV8316_CTRL4_OCP_MODE_OFFSET, value);
    if (status == HAL_OK) {
        mode = static_cast<DRV8316_OCP_Mode_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setOcpMode(const DRV8316_OCP_Mode_t mode) {
    if (!isValueInRange(static_cast<uint8_t>(mode), 3U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_MODE_MASK,
                    DRV8316_CTRL4_OCP_MODE_OFFSET, static_cast<uint8_t>(mode));
}

HAL_StatusTypeDef Drv8316::getOcpLevel(DRV8316_OCP_Level_t &level) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_LVL_MASK,
                 DRV8316_CTRL4_OCP_LVL_OFFSET, value);
    if (status == HAL_OK) {
        level = static_cast<DRV8316_OCP_Level_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setOcpLevel(const DRV8316_OCP_Level_t level) {
    if (!isValueInRange(static_cast<uint8_t>(level), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_LVL_MASK,
                    DRV8316_CTRL4_OCP_LVL_OFFSET, static_cast<uint8_t>(level));
}

HAL_StatusTypeDef Drv8316::getOcpRetryTime(DRV8316_OCP_RetryTime_t &retryTime) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_RETRY_MASK,
                 DRV8316_CTRL4_OCP_RETRY_OFFSET, value);
    if (status == HAL_OK) {
        retryTime = static_cast<DRV8316_OCP_RetryTime_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setOcpRetryTime(const DRV8316_OCP_RetryTime_t retryTime) {
    if (!isValueInRange(static_cast<uint8_t>(retryTime), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_RETRY_MASK,
                    DRV8316_CTRL4_OCP_RETRY_OFFSET,
                    static_cast<uint8_t>(retryTime));
}

HAL_StatusTypeDef
Drv8316::getOcpDeglitchTime(DRV8316_OCP_DeglitchTime_t &deglitchTime) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_DEG_MASK,
                 DRV8316_CTRL4_OCP_DEG_OFFSET, value);
    if (status == HAL_OK) {
        deglitchTime = static_cast<DRV8316_OCP_DeglitchTime_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setOcpDeglitchTime(const DRV8316_OCP_DeglitchTime_t deglitchTime) {
    if (!isValueInRange(static_cast<uint8_t>(deglitchTime), 3U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_DEG_MASK,
                    DRV8316_CTRL4_OCP_DEG_OFFSET,
                    static_cast<uint8_t>(deglitchTime));
}

HAL_StatusTypeDef Drv8316::getOcpCycleByCycle(DRV8316_OCP_CBC_t &enabled) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_CBC_MASK,
                 DRV8316_CTRL4_OCP_CBC_OFFSET, value);
    if (status == HAL_OK) {
        enabled = static_cast<DRV8316_OCP_CBC_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setOcpCycleByCycle(const DRV8316_OCP_CBC_t enabled) {
    if (!isValueInRange(static_cast<uint8_t>(enabled), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_CBC_MASK,
                    DRV8316_CTRL4_OCP_CBC_OFFSET,
                    static_cast<uint8_t>(enabled));
}

HAL_StatusTypeDef Drv8316::getDriverState(DRV8316_DriverState_t &state) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_DRV_OFF_MASK,
                 DRV8316_CTRL4_DRV_OFF_OFFSET, value);
    if (status == HAL_OK) {
        state = static_cast<DRV8316_DriverState_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setDriverState(const DRV8316_DriverState_t state) {
    if (!isValueInRange(static_cast<uint8_t>(state), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_4, DRV8316_CTRL4_DRV_OFF_MASK,
                    DRV8316_CTRL4_DRV_OFF_OFFSET, static_cast<uint8_t>(state));
}

HAL_StatusTypeDef Drv8316::getCurrentSenseGain(DRV8316_CSA_Gain_t &gain) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_5, DRV8316_CTRL5_CSA_GAIN_MASK,
                 DRV8316_CTRL5_CSA_GAIN_OFFSET, value);
    if (status == HAL_OK) {
        gain = static_cast<DRV8316_CSA_Gain_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setCurrentSenseGain(const DRV8316_CSA_Gain_t gain) {
    if (!isValueInRange(static_cast<uint8_t>(gain), 3U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_5, DRV8316_CTRL5_CSA_GAIN_MASK,
                    DRV8316_CTRL5_CSA_GAIN_OFFSET, static_cast<uint8_t>(gain));
}

HAL_StatusTypeDef Drv8316::getAsr(DRV8316_ASR_Enable_t &enabled) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_5, DRV8316_CTRL5_EN_ASR_MASK,
                 DRV8316_CTRL5_EN_ASR_OFFSET, value);
    if (status == HAL_OK) {
        enabled = static_cast<DRV8316_ASR_Enable_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setAsr(const DRV8316_ASR_Enable_t enabled) {
    if (!isValueInRange(static_cast<uint8_t>(enabled), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_5, DRV8316_CTRL5_EN_ASR_MASK,
                    DRV8316_CTRL5_EN_ASR_OFFSET, static_cast<uint8_t>(enabled));
}

HAL_StatusTypeDef Drv8316::getAar(DRV8316_AAR_Enable_t &enabled) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_5, DRV8316_CTRL5_EN_AAR_MASK,
                 DRV8316_CTRL5_EN_AAR_OFFSET, value);
    if (status == HAL_OK) {
        enabled = static_cast<DRV8316_AAR_Enable_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setAar(const DRV8316_AAR_Enable_t enabled) {
    if (!isValueInRange(static_cast<uint8_t>(enabled), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_5, DRV8316_CTRL5_EN_AAR_MASK,
                    DRV8316_CTRL5_EN_AAR_OFFSET, static_cast<uint8_t>(enabled));
}

HAL_StatusTypeDef
Drv8316::getIlimRecirculation(DRV8316_ILIM_Recirculation_t &mode) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_5, DRV8316_CTRL5_ILIM_RECIR_MASK,
                 DRV8316_CTRL5_ILIM_RECIR_OFFSET, value);
    if (status == HAL_OK) {
        mode = static_cast<DRV8316_ILIM_Recirculation_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setIlimRecirculation(const DRV8316_ILIM_Recirculation_t mode) {
    if (!isValueInRange(static_cast<uint8_t>(mode), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_5, DRV8316_CTRL5_ILIM_RECIR_MASK,
                    DRV8316_CTRL5_ILIM_RECIR_OFFSET,
                    static_cast<uint8_t>(mode));
}

HAL_StatusTypeDef Drv8316::getBuckState(DRV8316_BuckDisable_t &state) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_DIS_MASK,
                 DRV8316_CTRL6_BUCK_DIS_OFFSET, value);
    if (status == HAL_OK) {
        state = static_cast<DRV8316_BuckDisable_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setBuckState(const DRV8316_BuckDisable_t state) {
    if (!isValueInRange(static_cast<uint8_t>(state), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_DIS_MASK,
                    DRV8316_CTRL6_BUCK_DIS_OFFSET, static_cast<uint8_t>(state));
}

HAL_StatusTypeDef Drv8316::getBuckVoltage(DRV8316_BuckVoltage_t &voltage) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_SEL_MASK,
                 DRV8316_CTRL6_BUCK_SEL_OFFSET, value);
    if (status == HAL_OK) {
        voltage = static_cast<DRV8316_BuckVoltage_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setBuckVoltage(const DRV8316_BuckVoltage_t voltage) {
    if (!isValueInRange(static_cast<uint8_t>(voltage), 3U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_SEL_MASK,
                    DRV8316_CTRL6_BUCK_SEL_OFFSET,
                    static_cast<uint8_t>(voltage));
}

HAL_StatusTypeDef
Drv8316::getBuckCurrentLimit(DRV8316_BuckCurrentLimit_t &limit) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_CL_MASK,
                 DRV8316_CTRL6_BUCK_CL_OFFSET, value);
    if (status == HAL_OK) {
        limit = static_cast<DRV8316_BuckCurrentLimit_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setBuckCurrentLimit(const DRV8316_BuckCurrentLimit_t limit) {
    if (!isValueInRange(static_cast<uint8_t>(limit), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_CL_MASK,
                    DRV8316_CTRL6_BUCK_CL_OFFSET, static_cast<uint8_t>(limit));
}

HAL_StatusTypeDef
Drv8316::getBuckPowerSequencing(DRV8316_BuckPowerSequence_t &state) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_PS_DIS_MASK,
                 DRV8316_CTRL6_BUCK_PS_DIS_OFFSET, value);
    if (status == HAL_OK) {
        state = static_cast<DRV8316_BuckPowerSequence_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setBuckPowerSequencing(const DRV8316_BuckPowerSequence_t state) {
    if (!isValueInRange(static_cast<uint8_t>(state), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_PS_DIS_MASK,
                    DRV8316_CTRL6_BUCK_PS_DIS_OFFSET,
                    static_cast<uint8_t>(state));
}

HAL_StatusTypeDef Drv8316::getDelayTarget(DRV8316_DelayTarget_t &target) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_10, DRV8316_CTRL10_DLY_TARGET_MASK,
                 DRV8316_CTRL10_DLY_TARGET_OFFSET, value);
    if (status == HAL_OK) {
        target = static_cast<DRV8316_DelayTarget_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setDelayTarget(const DRV8316_DelayTarget_t target) {
    if (!isValueInRange(static_cast<uint8_t>(target), 0x0FU)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_10, DRV8316_CTRL10_DLY_TARGET_MASK,
                    DRV8316_CTRL10_DLY_TARGET_OFFSET,
                    static_cast<uint8_t>(target));
}

HAL_StatusTypeDef
Drv8316::getDelayCompensation(DRV8316_DelayCompensation_t &enabled) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_10, DRV8316_CTRL10_DLYCMP_EN_MASK,
                 DRV8316_CTRL10_DLYCMP_EN_OFFSET, value);
    if (status == HAL_OK) {
        enabled = static_cast<DRV8316_DelayCompensation_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setDelayCompensation(const DRV8316_DelayCompensation_t enabled) {
    if (!isValueInRange(static_cast<uint8_t>(enabled), 1U)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_10, DRV8316_CTRL10_DLYCMP_EN_MASK,
                    DRV8316_CTRL10_DLYCMP_EN_OFFSET,
                    static_cast<uint8_t>(enabled));
}

HAL_StatusTypeDef Drv8316::getSdoMode(DRV8316_SDO_Mode_t &mode) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_2, DRV8316_CTRL2_SDO_MODE_MASK,
                 DRV8316_CTRL2_SDO_MODE_OFFSET, value);
    if (status == HAL_OK) {
        mode = static_cast<DRV8316_SDO_Mode_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setSdoMode(const DRV8316_SDO_Mode_t mode) {
    if (static_cast<uint8_t>(mode) >
        static_cast<uint8_t>(DRV8316_SDO_MODE_PUSH_PULL)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_2, DRV8316_CTRL2_SDO_MODE_MASK,
                    DRV8316_CTRL2_SDO_MODE_OFFSET, static_cast<uint8_t>(mode));
}

HAL_StatusTypeDef Drv8316::getSlewRate(DRV8316_SlewRate_t &slewRate) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_2, DRV8316_CTRL2_SLEW_MASK,
                 DRV8316_CTRL2_SLEW_OFFSET, value);
    if (status == HAL_OK) {
        slewRate = static_cast<DRV8316_SlewRate_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setSlewRate(const DRV8316_SlewRate_t slewRate) {
    if (static_cast<uint8_t>(slewRate) >
        static_cast<uint8_t>(DRV8316_SLEW_RATE_200V_US)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_2, DRV8316_CTRL2_SLEW_MASK,
                    DRV8316_CTRL2_SLEW_OFFSET, static_cast<uint8_t>(slewRate));
}

HAL_StatusTypeDef Drv8316::getPwmMode(DRV8316_PWM_Mode_t &mode) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_2, DRV8316_CTRL2_PWM_MODE_MASK,
                 DRV8316_CTRL2_PWM_MODE_OFFSET, value);
    if (status == HAL_OK) {
        mode = static_cast<DRV8316_PWM_Mode_t>(value);
    }
    return status;
}

HAL_StatusTypeDef Drv8316::setPwmMode(const DRV8316_PWM_Mode_t mode) {
    if (static_cast<uint8_t>(mode) >
        static_cast<uint8_t>(DRV8316_PWM_MODE_3X_CL)) {
        return HAL_ERROR;
    }
    return setField(DRV8316_REG_CTRL_2, DRV8316_CTRL2_PWM_MODE_MASK,
                    DRV8316_CTRL2_PWM_MODE_OFFSET, static_cast<uint8_t>(mode));
}

HAL_StatusTypeDef Drv8316::getRegisterLock(DRV8316_RegisterLock_t &lockStatus) {
    uint8_t value = 0U;
    const HAL_StatusTypeDef status =
        getField(DRV8316_REG_CTRL_1, DRV8316_CTRL1_REG_LOCK_MASK,
                 DRV8316_CTRL1_REG_LOCK_OFFSET, value);
    if (status == HAL_OK) {
        lockStatus = static_cast<DRV8316_RegisterLock_t>(value);
    }
    return status;
}

HAL_StatusTypeDef
Drv8316::setRegisterLock(const DRV8316_RegisterLock_t lockStatus) {
    if ((lockStatus != DRV8316_REG_LOCK_UNLOCK) &&
        (lockStatus != DRV8316_REG_LOCK_LOCK)) {
        return HAL_ERROR;
    }

    return setField(DRV8316_REG_CTRL_1, DRV8316_CTRL1_REG_LOCK_MASK,
                    DRV8316_CTRL1_REG_LOCK_OFFSET,
                    static_cast<uint8_t>(lockStatus));
}

HAL_StatusTypeDef Drv8316::lockRegisters() {
    return setRegisterLock(DRV8316_REG_LOCK_LOCK);
}

HAL_StatusTypeDef Drv8316::unlockRegisters() {
    return setRegisterLock(DRV8316_REG_LOCK_UNLOCK);
}

HAL_StatusTypeDef Drv8316::clearFaults() {
    uint8_t control2 = 0U;
    HAL_StatusTypeDef status = readRegister(DRV8316_REG_CTRL_2, control2);
    if (status != HAL_OK) {
        return status;
    }

    control2 = static_cast<uint8_t>(control2 | DRV8316_CTRL2_CLR_FLT_MASK);
    return writeRegister(DRV8316_REG_CTRL_2, control2);
}

HAL_StatusTypeDef Drv8316::setPwm(const uint16_t phaseA, const uint16_t phaseB,
                                  const uint16_t phaseC) {
    if ((config_.timer == nullptr) || (config_.timer->Instance == nullptr)) {
        return HAL_ERROR;
    }

    TIM_TypeDef *const timer = config_.timer->Instance;
    const uint32_t maximum = timer->ARR;

    if ((phaseA > maximum) || (phaseB > maximum) || (phaseC > maximum)) {
        return HAL_ERROR;
    }

    timer->CCR1 = phaseA;
    timer->CCR2 = phaseB;
    timer->CCR3 = phaseC;

    return HAL_OK;
}
