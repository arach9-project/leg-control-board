#include "as5048a/as5048a.hpp"

As5048a::As5048a(const Config &config) noexcept : config_{config} {
}

HAL_StatusTypeDef As5048a::initialize() {
    if ((config_.spi == nullptr) || !config_.chipSelect.isValid()) {
        return HAL_ERROR;
    }

    enableCycleCounter();

    HAL_GPIO_WritePin(config_.chipSelect.port, config_.chipSelect.number,
                      GPIO_PIN_SET);
    HAL_Delay(10U);

    uint16_t ignoredResponse = 0U;
    return transferFrame(AS5048A_SPI_NOP_COMMAND, ignoredResponse);
}

void As5048a::enableCycleCounter() noexcept {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

#if defined(DWT_LAR)
    DWT->LAR = 0xC5ACCE55UL;
#endif

    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void As5048a::delayMicroseconds(const uint32_t microseconds) noexcept {
    const uint32_t cyclesPerMicrosecond = SystemCoreClock / 1'000'000UL;
    const uint32_t requiredCycles = cyclesPerMicrosecond * microseconds;
    const uint32_t start = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < requiredCycles) {
    }
}

uint8_t As5048a::countSetBits(uint16_t word) noexcept {
    uint8_t count = 0U;

    while (word != 0U) {
        count = static_cast<uint8_t>(count + (word & 1U));
        word >>= 1U;
    }

    return count;
}

bool As5048a::requiresParityBit(const uint16_t frame) noexcept {
    return (countSetBits(frame) & 1U) != 0U;
}

bool As5048a::isValueInRange(const uint16_t value,
                             const uint16_t maximum) noexcept {
    return value <= maximum;
}

HAL_StatusTypeDef As5048a::transferFrame(const uint16_t transmitFrame,
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

    delayMicroseconds(1U);

    const HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        config_.spi, transmitBytes, receiveBytes, 2U, config_.spiTimeoutMs);

    if (status == HAL_OK) {
        while (__HAL_SPI_GET_FLAG(config_.spi, SPI_FLAG_BSY) != RESET) {
        }
    }

    delayMicroseconds(1U);

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

HAL_StatusTypeDef As5048a::readRegister(const uint16_t address,
                                        uint16_t &value) {
    value = 0U;

    if (!isValueInRange(address, AS5048A_SPI_ADDRESS_MASK)) {
        return HAL_ERROR;
    }

    uint16_t readCommand = static_cast<uint16_t>(
        AS5048A_SPI_READ | (address & AS5048A_SPI_ADDRESS_MASK));

    if (requiresParityBit(readCommand)) {
        readCommand |= AS5048A_SPI_PARITY_MASK;
    }

    uint16_t ignoredResponse = 0U;
    HAL_StatusTypeDef status = transferFrame(readCommand, ignoredResponse);
    if (status != HAL_OK) {
        return status;
    }

    uint16_t registerResponse = 0U;
    status = transferFrame(AS5048A_SPI_NOP_COMMAND, registerResponse);
    if (status != HAL_OK) {
        return status;
    }

    if ((countSetBits(registerResponse) & 1U) != 0U) {
        return HAL_ERROR;
    }

    if ((registerResponse & AS5048A_SPI_ERROR_MASK) != 0U) {
        return HAL_ERROR;
    }

    value = static_cast<uint16_t>(registerResponse & AS5048A_SPI_DATA_MASK);
    return HAL_OK;
}

HAL_StatusTypeDef As5048a::writeRegister(const uint16_t address,
                                         const uint16_t value,
                                         uint16_t *const receiveFrame) {
    if (!isValueInRange(address, AS5048A_SPI_ADDRESS_MASK)) {
        return HAL_ERROR;
    }

    if (!isValueInRange(value, AS5048A_SPI_DATA_MASK)) {
        return HAL_ERROR;
    }

    // Write-address command: bit15 parity, bit14 = 0 (write), bits13:0 address.
    uint16_t addressFrame =
        static_cast<uint16_t>(address & AS5048A_SPI_ADDRESS_MASK);

    if (requiresParityBit(addressFrame)) {
        addressFrame |= AS5048A_SPI_PARITY_MASK;
    }

    uint16_t addressResponse = 0U;
    HAL_StatusTypeDef status = transferFrame(addressFrame, addressResponse);
    if (status != HAL_OK) {
        return status;
    }

    // Write-data frame: bit15 parity, bit14 must stay 0, bits13:0 data.
    uint16_t dataFrame = static_cast<uint16_t>(value & AS5048A_SPI_DATA_MASK);

    if (requiresParityBit(dataFrame)) {
        dataFrame |= AS5048A_SPI_PARITY_MASK;
    }

    uint16_t dataResponse = 0U;
    status = transferFrame(dataFrame, dataResponse);
    if (status != HAL_OK) {
        return status;
    }

    if (receiveFrame != nullptr) {
        *receiveFrame = dataResponse;
    }

    // dataResponse echoes the *previous* (address) command's response, per
    // the AS5048A's one-frame-delayed reply. At minimum, confirm its parity.
    if ((countSetBits(dataResponse) & 1U) != 0U) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef As5048a::getField(const uint16_t registerAddress,
                                    const uint16_t mask, const uint8_t offset,
                                    uint16_t &value) {
    value = 0U;

    if (offset >= 16U) {
        return HAL_ERROR;
    }

    uint16_t registerValue = 0U;
    const HAL_StatusTypeDef status =
        readRegister(registerAddress, registerValue);
    if (status != HAL_OK) {
        return status;
    }

    value = static_cast<uint16_t>((registerValue & mask) >> offset);
    return HAL_OK;
}

HAL_StatusTypeDef As5048a::setField(const uint16_t registerAddress,
                                    const uint16_t mask, const uint8_t offset,
                                    const uint16_t value) {
    if (offset >= 16U) {
        return HAL_ERROR;
    }

    const uint32_t shiftedValue = static_cast<uint32_t>(value) << offset;
    if ((shiftedValue & static_cast<uint32_t>(~mask)) != 0U) {
        return HAL_ERROR;
    }

    uint16_t registerValue = 0U;
    HAL_StatusTypeDef status = readRegister(registerAddress, registerValue);
    if (status != HAL_OK) {
        return status;
    }

    registerValue =
        static_cast<uint16_t>(registerValue & static_cast<uint16_t>(~mask));
    registerValue = static_cast<uint16_t>(
        registerValue | static_cast<uint16_t>(shiftedValue & mask));

    return writeRegister(registerAddress, registerValue);
}

/* -------------------------------------------------------------------------- */
/* Complete sensor sample                                                     */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef As5048a::sample(AS5048A_Sample_t &sample) {
    sample = AS5048A_Sample_t{};

    uint16_t rawAngle = 0U;
    HAL_StatusTypeDef status = readRegister(AS5048A_REG_ANGLE, rawAngle);
    if (status != HAL_OK) {
        return status;
    }

    rawAngle &= AS5048A_ANGLE_MASK;
    sample.angle.raw = rawAngle;
    sample.angle.degrees =
        static_cast<float>(rawAngle) * AS5048A_DEGREES_PER_COUNT;
    sample.angle.radians =
        static_cast<float>(rawAngle) * AS5048A_RADIANS_PER_COUNT;

    uint16_t magnitude = 0U;
    status = readRegister(AS5048A_REG_MAGNITUDE, magnitude);
    if (status != HAL_OK) {
        return status;
    }
    sample.magnitude.raw =
        static_cast<uint16_t>(magnitude & AS5048A_MAGNITUDE_MASK);

    uint16_t diagnosticRegister = 0U;
    status = readRegister(AS5048A_REG_DIAGNOSTICS_AGC, diagnosticRegister);
    if (status != HAL_OK) {
        return status;
    }

    AS5048A_Diagnostics_t &diagnostics = sample.diagnostics;
    diagnostics.raw = diagnosticRegister;
    diagnostics.agc =
        static_cast<uint8_t>((diagnosticRegister & AS5048A_DIAG_AGC_MASK) >>
                             AS5048A_DIAG_AGC_OFFSET);
    diagnostics.offset_compensation_finished =
        (diagnosticRegister & AS5048A_DIAG_OCF_MASK) != 0U;
    diagnostics.cordic_overflow =
        (diagnosticRegister & AS5048A_DIAG_COF_MASK) != 0U;
    diagnostics.magnet_too_strong =
        (diagnosticRegister & AS5048A_DIAG_COMP_LOW_MASK) != 0U;
    diagnostics.magnet_too_weak =
        (diagnosticRegister & AS5048A_DIAG_COMP_HIGH_MASK) != 0U;

    if (!diagnostics.offset_compensation_finished ||
        diagnostics.cordic_overflow || diagnostics.magnet_too_strong ||
        diagnostics.magnet_too_weak) {
        return HAL_ERROR;
    }

    sample.valid = true;
    return HAL_OK;
}

/* -------------------------------------------------------------------------- */
/* Continuous fast angle reads                                                */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef As5048a::beginContinuousAngleRead() {
    continuousAngleReadPrimed_ = false;

    uint16_t command = static_cast<uint16_t>(
        AS5048A_SPI_READ | (AS5048A_REG_ANGLE & AS5048A_SPI_ADDRESS_MASK));

    if (requiresParityBit(command)) {
        command |= AS5048A_SPI_PARITY_MASK;
    }

    uint16_t ignoredResponse = 0U;
    const HAL_StatusTypeDef status = transferFrame(command, ignoredResponse);
    if (status != HAL_OK) {
        return status;
    }

    continuousAngleReadPrimed_ = true;
    return HAL_OK;
}

HAL_StatusTypeDef As5048a::readNextAngle(AS5048A_Angle_t &angle) {
    angle = AS5048A_Angle_t{};

    if (!continuousAngleReadPrimed_) {
        return HAL_ERROR;
    }

    uint16_t command = static_cast<uint16_t>(
        AS5048A_SPI_READ | (AS5048A_REG_ANGLE & AS5048A_SPI_ADDRESS_MASK));

    if (requiresParityBit(command)) {
        command |= AS5048A_SPI_PARITY_MASK;
    }

    uint16_t response = 0U;
    const HAL_StatusTypeDef status = transferFrame(command, response);
    if (status != HAL_OK) {
        continuousAngleReadPrimed_ = false;
        return status;
    }

    if ((countSetBits(response) & 1U) != 0U) {
        continuousAngleReadPrimed_ = false;
        return HAL_ERROR;
    }

    if ((response & AS5048A_SPI_ERROR_MASK) != 0U) {
        continuousAngleReadPrimed_ = false;
        return HAL_ERROR;
    }

    const uint16_t raw = static_cast<uint16_t>(response & AS5048A_ANGLE_MASK);
    angle.raw = raw;
    angle.degrees = static_cast<float>(raw) * AS5048A_DEGREES_PER_COUNT;
    angle.radians = static_cast<float>(raw) * AS5048A_RADIANS_PER_COUNT;

    return HAL_OK;
}

/* -------------------------------------------------------------------------- */
/* Zero position / errors                                                    */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef As5048a::readZeroPosition(uint16_t &zeroPosition) {
    zeroPosition = 0U;

    uint16_t high = 0U;
    HAL_StatusTypeDef status =
        readRegister(AS5048A_REG_ZERO_POSITION_HIGH, high);
    if (status != HAL_OK) {
        return status;
    }

    uint16_t low = 0U;
    status = readRegister(AS5048A_REG_ZERO_POSITION_LOW, low);
    if (status != HAL_OK) {
        return status;
    }

    zeroPosition = AS5048A_ZERO_POSITION_COMBINE(high & AS5048A_ZERO_HIGH_MASK,
                                                 low & AS5048A_ZERO_LOW_MASK);
    return HAL_OK;
}

HAL_StatusTypeDef As5048a::writeZeroPosition(const uint16_t zeroPosition) {
    if (!isValueInRange(zeroPosition, AS5048A_ZERO_POSITION_MASK)) {
        return HAL_ERROR;
    }

    const uint16_t high = AS5048A_ZERO_POSITION_HIGH_VALUE(zeroPosition);
    const uint16_t low = AS5048A_ZERO_POSITION_LOW_VALUE(zeroPosition);

    HAL_StatusTypeDef status =
        writeRegister(AS5048A_REG_ZERO_POSITION_HIGH, high);
    if (status != HAL_OK) {
        return status;
    }

    return writeRegister(AS5048A_REG_ZERO_POSITION_LOW, low);
}

HAL_StatusTypeDef As5048a::setCurrentPositionAsZero() {
    // Per the datasheet, the zero-position registers must be cleared before
    // reading the current angle, or the read reflects the old offset.
    HAL_StatusTypeDef status =
        writeRegister(AS5048A_REG_ZERO_POSITION_HIGH, 0U);
    if (status != HAL_OK) {
        return status;
    }

    status = writeRegister(AS5048A_REG_ZERO_POSITION_LOW, 0U);
    if (status != HAL_OK) {
        return status;
    }

    uint16_t currentAngle = 0U;
    status = readRegister(AS5048A_REG_ANGLE, currentAngle);
    if (status != HAL_OK) {
        return status;
    }

    return writeZeroPosition(currentAngle & AS5048A_ZERO_POSITION_MASK);
}

HAL_StatusTypeDef As5048a::clearCommunicationErrors(uint16_t &rawErrors) {
    return readRegister(AS5048A_REG_CLEAR_ERROR, rawErrors);
}
