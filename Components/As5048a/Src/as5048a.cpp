#include "as5048a/as5048a.hpp"

namespace as5048a {

const char* errorName(Error error) noexcept {
  switch (error) {
  case Error::None:
    return "None";
  case Error::InvalidConfig:
    return "Invalid configuration";
  case Error::InvalidAddress:
    return "Invalid register address";
  case Error::InvalidValue:
    return "Invalid register value";
  case Error::InvalidField:
    return "Invalid bitfield";
  case Error::SpiBusy:
    return "SPI peripheral busy";
  case Error::SpiTimeout:
    return "SPI transfer timeout";
  case Error::SpiError:
    return "SPI transport failure";
  case Error::ParityError:
    return "SPI response parity error";
  case Error::SensorErrorFlag:
    return "AS5048A error flag set";
  case Error::PipelineNotPrimed:
    return "Angle pipeline not primed";
  case Error::OffsetCompensationIncomplete:
    return "Offset compensation incomplete";
  case Error::CordicOverflow:
    return "CORDIC overflow";
  case Error::MagnetTooStrong:
    return "Magnetic field too strong";
  case Error::MagnetTooWeak:
    return "Magnetic field too weak";
  }

  return "Unknown error";
}

const char* operationName(Operation operation) noexcept {
  switch (operation) {
  case Operation::None:
    return "None";
  case Operation::Initialize:
    return "Initialize";
  case Operation::Transfer:
    return "SPI transfer";
  case Operation::ReadRegister:
    return "Register read";
  case Operation::WriteRegister:
    return "Register write";
  case Operation::ReadAngle:
    return "Angle read";
  case Operation::ReadDiagnostics:
    return "Diagnostics read";
  case Operation::ReadMagnitude:
    return "Magnitude read";
  case Operation::BeginContinuous:
    return "Prime continuous reads";
  case Operation::ContinuousAngle:
    return "Continuous angle read";
  case Operation::ReadZero:
    return "Zero-position read";
  case Operation::WriteZero:
    return "Zero-position write";
  case Operation::ClearErrors:
    return "Clear communication errors";
  }

  return "Unknown operation";
}

} // namespace as5048a

As5048a::As5048a(const Config& config) noexcept : config_{config} {}

/* -------------------------------------------------------------------------- */
/* Status                                                                     */
/* -------------------------------------------------------------------------- */

As5048a::Status As5048a::record(Status status) noexcept {
  last_status_ = status;
  return status;
}

/* -------------------------------------------------------------------------- */
/* Timing                                                                     */
/* -------------------------------------------------------------------------- */

void As5048a::enableCycleCounter() noexcept {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

#if defined(DWT_LAR)
  DWT->LAR = 0xC5ACCE55UL;
#endif

  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void As5048a::delayMicroseconds(uint32_t microseconds) noexcept {
  const uint32_t cyclesPerMicrosecond = SystemCoreClock / 1000000UL;
  const uint32_t requiredCycles = cyclesPerMicrosecond * microseconds;
  const uint32_t start = DWT->CYCCNT;

  while ((DWT->CYCCNT - start) < requiredCycles) {
  }
}

/* -------------------------------------------------------------------------- */
/* SPI helpers                                                                */
/* -------------------------------------------------------------------------- */

uint8_t As5048a::countSetBits(uint16_t word) noexcept {
  uint8_t count = 0U;

  while (word != 0U) {
    count += static_cast<uint8_t>(word & 1U);
    word >>= 1U;
  }

  return count;
}

bool As5048a::requiresParityBit(uint16_t frame) noexcept {
  return (countSetBits(frame) & 1U) != 0U;
}

uint16_t As5048a::makeReadCommand(uint16_t address) noexcept {
  uint16_t command = static_cast<uint16_t>(AS5048A_SPI_READ | (address & AS5048A_SPI_ADDRESS_MASK));

  if (requiresParityBit(command)) {
    command |= AS5048A_SPI_PARITY_MASK;
  }

  return command;
}

/* -------------------------------------------------------------------------- */
/* SPI transport                                                              */
/* -------------------------------------------------------------------------- */

As5048a::Status As5048a::transferFrame(uint16_t transmitFrame, uint16_t& receiveFrame) {
  receiveFrame = 0U;

  Status status{};
  status.operation = Operation::Transfer;
  status.transmitted_frame = transmitFrame;

  if (config_.spi == nullptr || !config_.chipSelect.isValid()) {
    status.error = Error::InvalidConfig;
    status.hal_status = HAL_ERROR;
    return record(status);
  }

  SPI_HandleTypeDef* spi = config_.spi;

  if (HAL_SPI_GetState(spi) != HAL_SPI_STATE_READY) {
    status.error = Error::SpiBusy;
    status.hal_status = HAL_BUSY;
    status.spi_error = HAL_SPI_GetError(spi);

    return record(status);
  }

  uint8_t tx[2] = {
      static_cast<uint8_t>((transmitFrame >> 8U) & 0xFFU),
      static_cast<uint8_t>(transmitFrame & 0xFFU),
  };

  uint8_t rx[2] = {0U, 0U};

  HAL_GPIO_WritePin(config_.chipSelect.port, config_.chipSelect.number, GPIO_PIN_RESET);

  delayMicroseconds(1U);

  status.hal_status = HAL_SPI_TransmitReceive(spi, tx, rx, 2U, config_.spiTimeoutMs);

  status.spi_error = HAL_SPI_GetError(spi);

  if (status.hal_status == HAL_OK) {
    const uint32_t start = DWT->CYCCNT;
    const uint32_t timeoutCycles = SystemCoreClock / 100000U;

    while (__HAL_SPI_GET_FLAG(spi, SPI_FLAG_BSY) != RESET) {
      if ((DWT->CYCCNT - start) >= timeoutCycles) {
        status.error = Error::SpiTimeout;
        status.hal_status = HAL_TIMEOUT;
        break;
      }
    }
  }

  delayMicroseconds(1U);

  HAL_GPIO_WritePin(config_.chipSelect.port, config_.chipSelect.number, GPIO_PIN_SET);

  delayMicroseconds(1U);

  receiveFrame =
      static_cast<uint16_t>((static_cast<uint16_t>(rx[0]) << 8U) | static_cast<uint16_t>(rx[1]));

  status.received_frame = receiveFrame;

  if (status.error == Error::None) {
    switch (status.hal_status) {
    case HAL_OK:
      break;

    case HAL_BUSY:
      status.error = Error::SpiBusy;
      break;

    case HAL_TIMEOUT:
      status.error = Error::SpiTimeout;
      break;

    default:
      status.error = Error::SpiError;
      break;
    }
  }

  return record(status);
}

/* -------------------------------------------------------------------------- */
/* Response validation                                                        */
/* -------------------------------------------------------------------------- */

As5048a::Status As5048a::validateResponse(uint16_t frame, uint16_t address, Operation operation) {
  Status status{};

  status.operation = operation;
  status.register_address = address;
  status.received_frame = frame;

  if ((countSetBits(frame) & 1U) != 0U) {
    status.error = Error::ParityError;
    status.hal_status = HAL_ERROR;

    return record(status);
  }

  if ((frame & AS5048A_SPI_ERROR_MASK) != 0U) {
    status.error = Error::SensorErrorFlag;
    status.hal_status = HAL_ERROR;

    return record(status);
  }

  return record(status);
}

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

As5048a::Status As5048a::initialize() {
  continuousAngleReadPrimed_ = false;

  if (config_.spi == nullptr || !config_.chipSelect.isValid()) {
    Status status{};
    status.error = Error::InvalidConfig;
    status.operation = Operation::Initialize;
    status.hal_status = HAL_ERROR;

    return record(status);
  }

  enableCycleCounter();

  HAL_GPIO_WritePin(config_.chipSelect.port, config_.chipSelect.number, GPIO_PIN_SET);

  HAL_Delay(10U);

  uint16_t ignoredResponse = 0U;

  Status status = transferFrame(AS5048A_SPI_NOP_COMMAND, ignoredResponse);

  status.operation = Operation::Initialize;

  return record(status);
}

/* -------------------------------------------------------------------------- */
/* Register reading                                                           */
/* -------------------------------------------------------------------------- */

As5048a::Result<uint16_t> As5048a::readRegister(uint16_t address) {
  Result<uint16_t> result{};

  if (address > AS5048A_SPI_ADDRESS_MASK) {
    result.status.error = Error::InvalidAddress;
    result.status.operation = Operation::ReadRegister;
    result.status.register_address = address;
    result.status.hal_status = HAL_ERROR;

    result.status = record(result.status);
    return result;
  }

  // Arbitrary register reads interrupt the continuous angle pipeline.
  continuousAngleReadPrimed_ = false;

  uint16_t ignoredResponse = 0U;

  Status status = transferFrame(makeReadCommand(address), ignoredResponse);

  if (!status.ok()) {
    status.operation = Operation::ReadRegister;
    status.register_address = address;

    result.status = record(status);
    return result;
  }

  // AS5048A responses are delayed by one transaction.
  uint16_t registerResponse = 0U;

  status = transferFrame(AS5048A_SPI_NOP_COMMAND, registerResponse);

  if (!status.ok()) {
    status.operation = Operation::ReadRegister;
    status.register_address = address;

    result.status = record(status);
    return result;
  }

  status = validateResponse(registerResponse, address, Operation::ReadRegister);

  if (!status.ok()) {
    result.status = status;
    return result;
  }

  // Keep details from the transfer as well as the validation.
  status.transmitted_frame = AS5048A_SPI_NOP_COMMAND;
  status.received_frame = registerResponse;
  status.spi_error = HAL_SPI_GetError(config_.spi);

  result.value = static_cast<uint16_t>(registerResponse & AS5048A_SPI_DATA_MASK);

  result.status = record(status);

  return result;
}

/* -------------------------------------------------------------------------- */
/* Register writing                                                           */
/* -------------------------------------------------------------------------- */

As5048a::Status As5048a::writeRegister(uint16_t address, uint16_t value, uint16_t* receiveFrame) {
  continuousAngleReadPrimed_ = false;

  Status status{};
  status.operation = Operation::WriteRegister;
  status.register_address = address;

  if (address > AS5048A_SPI_ADDRESS_MASK) {
    status.error = Error::InvalidAddress;
    status.hal_status = HAL_ERROR;

    return record(status);
  }

  if (value > AS5048A_SPI_DATA_MASK) {
    status.error = Error::InvalidValue;
    status.hal_status = HAL_ERROR;

    return record(status);
  }

  // First transaction: write address.
  uint16_t addressFrame = static_cast<uint16_t>(address & AS5048A_SPI_ADDRESS_MASK);

  if (requiresParityBit(addressFrame)) {
    addressFrame |= AS5048A_SPI_PARITY_MASK;
  }

  uint16_t ignoredResponse = 0U;

  status = transferFrame(addressFrame, ignoredResponse);

  if (!status.ok()) {
    status.operation = Operation::WriteRegister;
    status.register_address = address;

    return record(status);
  }

  // Second transaction: write data.
  uint16_t dataFrame = static_cast<uint16_t>(value & AS5048A_SPI_DATA_MASK);

  if (requiresParityBit(dataFrame)) {
    dataFrame |= AS5048A_SPI_PARITY_MASK;
  }

  uint16_t response = 0U;

  status = transferFrame(dataFrame, response);

  if (!status.ok()) {
    status.operation = Operation::WriteRegister;
    status.register_address = address;

    return record(status);
  }

  if (receiveFrame != nullptr) {
    *receiveFrame = response;
  }

  status = validateResponse(response, address, Operation::WriteRegister);

  status.transmitted_frame = dataFrame;
  status.received_frame = response;

  return record(status);
}

/* -------------------------------------------------------------------------- */
/* Bitfield access                                                            */
/* -------------------------------------------------------------------------- */

As5048a::Result<uint16_t> As5048a::getField(uint16_t registerAddress, uint16_t mask,
                                            uint8_t offset) {
  Result<uint16_t> result{};

  if (offset >= 16U) {
    result.status.error = Error::InvalidField;
    result.status.hal_status = HAL_ERROR;

    result.status = record(result.status);
    return result;
  }

  auto reg = readRegister(registerAddress);

  result.status = reg.status;

  if (!reg.ok()) {
    return result;
  }

  result.value = static_cast<uint16_t>((reg.value & mask) >> offset);

  return result;
}

As5048a::Status As5048a::setField(uint16_t registerAddress, uint16_t mask, uint8_t offset,
                                  uint16_t value) {
  if (offset >= 16U) {
    Status status{};
    status.error = Error::InvalidField;
    status.hal_status = HAL_ERROR;

    return record(status);
  }

  const uint32_t shiftedValue = static_cast<uint32_t>(value) << offset;

  if ((shiftedValue & ~static_cast<uint32_t>(mask)) != 0U) {
    Status status{};
    status.error = Error::InvalidField;
    status.hal_status = HAL_ERROR;

    return record(status);
  }

  auto reg = readRegister(registerAddress);

  if (!reg.ok()) {
    return reg.status;
  }

  uint16_t updated =
      static_cast<uint16_t>((reg.value & static_cast<uint16_t>(~mask)) | (shiftedValue & mask));

  return writeRegister(registerAddress, updated);
}

/* -------------------------------------------------------------------------- */
/* Complete sensor sample                                                     */
/* -------------------------------------------------------------------------- */

As5048a::Result<AS5048A_Sample_t> As5048a::sample() {
  Result<AS5048A_Sample_t> result{};

  continuousAngleReadPrimed_ = false;

  /* ---------------------------------------------------------------------- */
  /* Read angle                                                             */
  /* ---------------------------------------------------------------------- */

  const auto angle = readRegister(AS5048A_REG_ANGLE);

  if (!angle.ok()) {
    result.status = angle.status;
    result.status.operation = Operation::ReadAngle;
    result.status = record(result.status);
    return result;
  }

  const uint16_t rawAngle = static_cast<uint16_t>(angle.value & AS5048A_ANGLE_MASK);

  result.value.angle.raw = rawAngle;

  result.value.angle.degrees = static_cast<float>(rawAngle) * AS5048A_DEGREES_PER_COUNT;

  result.value.angle.radians = static_cast<float>(rawAngle) * AS5048A_RADIANS_PER_COUNT;

  /* ---------------------------------------------------------------------- */
  /* Read magnetic magnitude                                                */
  /* ---------------------------------------------------------------------- */

  const auto magnitude = readRegister(AS5048A_REG_MAGNITUDE);

  if (!magnitude.ok()) {
    result.status = magnitude.status;
    result.status.operation = Operation::ReadMagnitude;
    result.status = record(result.status);
    return result;
  }

  result.value.magnitude.raw = static_cast<uint16_t>(magnitude.value & AS5048A_MAGNITUDE_MASK);

  /* ---------------------------------------------------------------------- */
  /* Read magnetic diagnostics                                              */
  /* ---------------------------------------------------------------------- */

  const auto diagnostics = readRegister(AS5048A_REG_DIAGNOSTICS_AGC);

  if (!diagnostics.ok()) {
    result.status = diagnostics.status;
    result.status.operation = Operation::ReadDiagnostics;
    result.status = record(result.status);
    return result;
  }

  AS5048A_Diagnostics_t& diag = result.value.diagnostics;

  diag.raw = diagnostics.value;

  diag.agc = static_cast<uint8_t>((diag.raw & AS5048A_DIAG_AGC_MASK) >> AS5048A_DIAG_AGC_OFFSET);

  diag.offset_compensation_finished = (diag.raw & AS5048A_DIAG_OCF_MASK) != 0U;

  diag.cordic_overflow = (diag.raw & AS5048A_DIAG_COF_MASK) != 0U;

  diag.magnet_too_strong = (diag.raw & AS5048A_DIAG_COMP_LOW_MASK) != 0U;

  diag.magnet_too_weak = (diag.raw & AS5048A_DIAG_COMP_HIGH_MASK) != 0U;

  /* ---------------------------------------------------------------------- */
  /* Preserve every diagnostic field                                        */
  /* ---------------------------------------------------------------------- */

  Status status = diagnostics.status;

  status.operation = Operation::ReadDiagnostics;
  status.register_address = AS5048A_REG_DIAGNOSTICS_AGC;

  status.diagnostics_raw = diag.raw;
  status.magnitude_raw = result.value.magnitude.raw;
  status.agc = diag.agc;

  status.diagnostics_valid = true;
  status.magnitude_valid = true;

  status.offset_compensation_finished = diag.offset_compensation_finished;

  status.cordic_overflow = diag.cordic_overflow;

  status.magnet_too_strong = diag.magnet_too_strong;

  status.magnet_too_weak = diag.magnet_too_weak;

  /* ---------------------------------------------------------------------- */
  /* Classify the error                                                     */
  /* ---------------------------------------------------------------------- */

  // Prioritize magnetic and processing faults.
  // OCF=0 can accompany a weak magnetic field.

  if (status.magnet_too_weak) {
    status.error = Error::MagnetTooWeak;
  } else if (status.magnet_too_strong) {
    status.error = Error::MagnetTooStrong;
  } else if (status.cordic_overflow) {
    status.error = Error::CordicOverflow;
  } else if (!status.offset_compensation_finished) {
    status.error = Error::OffsetCompensationIncomplete;
  }

  if (!status.ok()) {
    status.hal_status = HAL_ERROR;
  }

  result.value.valid = status.ok();
  result.status = record(status);

  return result;
}

/* -------------------------------------------------------------------------- */
/* Continuous angle acquisition                                               */
/* -------------------------------------------------------------------------- */

As5048a::Status As5048a::beginContinuousAngleRead() {
  continuousAngleReadPrimed_ = false;

  uint16_t ignoredResponse = 0U;

  Status status = transferFrame(makeReadCommand(AS5048A_REG_ANGLE), ignoredResponse);

  status.operation = Operation::BeginContinuous;
  status.register_address = AS5048A_REG_ANGLE;

  if (!status.ok()) {
    return record(status);
  }

  // The next response corresponds to the ANGLE command.
  continuousAngleReadPrimed_ = true;

  return record(status);
}

bool As5048a::continuousAngleReadPrimed() const noexcept {
  return continuousAngleReadPrimed_;
}

As5048a::Result<AS5048A_Angle_t> As5048a::readNextAngle() {
  Result<AS5048A_Angle_t> result{};

  if (!continuousAngleReadPrimed_) {
    result.status.error = Error::PipelineNotPrimed;
    result.status.operation = Operation::ContinuousAngle;
    result.status.register_address = AS5048A_REG_ANGLE;
    result.status.hal_status = HAL_ERROR;

    result.status = record(result.status);
    return result;
  }

  const uint16_t command = makeReadCommand(AS5048A_REG_ANGLE);

  uint16_t response = 0U;

  Status status = transferFrame(command, response);

  status.operation = Operation::ContinuousAngle;
  status.register_address = AS5048A_REG_ANGLE;

  if (!status.ok()) {
    continuousAngleReadPrimed_ = false;
    result.status = record(status);
    return result;
  }

  // Response to the previous ANGLE request.
  status = validateResponse(response, AS5048A_REG_ANGLE, Operation::ContinuousAngle);

  status.transmitted_frame = command;
  status.received_frame = response;
  status.spi_error = HAL_SPI_GetError(config_.spi);

  if (!status.ok()) {
    continuousAngleReadPrimed_ = false;
    result.status = record(status);
    return result;
  }

  const uint16_t raw = static_cast<uint16_t>(response & AS5048A_ANGLE_MASK);

  result.value.raw = raw;

  result.value.degrees = static_cast<float>(raw) * AS5048A_DEGREES_PER_COUNT;

  result.value.radians = static_cast<float>(raw) * AS5048A_RADIANS_PER_COUNT;

  result.status = record(status);

  return result;
}

/* -------------------------------------------------------------------------- */
/* Zero position                                                              */
/* -------------------------------------------------------------------------- */

As5048a::Result<uint16_t> As5048a::readZeroPosition() {
  Result<uint16_t> result{};

  auto high = readRegister(AS5048A_REG_ZERO_POSITION_HIGH);

  if (!high.ok()) {
    result.status = high.status;
    result.status.operation = Operation::ReadZero;
    result.status = record(result.status);
    return result;
  }

  auto low = readRegister(AS5048A_REG_ZERO_POSITION_LOW);

  if (!low.ok()) {
    result.status = low.status;
    result.status.operation = Operation::ReadZero;
    result.status = record(result.status);
    return result;
  }

  result.value = AS5048A_ZERO_POSITION_COMBINE(high.value & AS5048A_ZERO_HIGH_MASK,
                                               low.value & AS5048A_ZERO_LOW_MASK);

  result.status = low.status;
  result.status.operation = Operation::ReadZero;
  result.status = record(result.status);

  return result;
}

As5048a::Status As5048a::writeZeroPosition(uint16_t zeroPosition) {
  if (zeroPosition > AS5048A_ZERO_POSITION_MASK) {
    Status status{};
    status.error = Error::InvalidValue;
    status.operation = Operation::WriteZero;
    status.hal_status = HAL_ERROR;

    return record(status);
  }

  Status status =
      writeRegister(AS5048A_REG_ZERO_POSITION_HIGH, AS5048A_ZERO_POSITION_HIGH_VALUE(zeroPosition));

  if (!status.ok()) {
    status.operation = Operation::WriteZero;
    return record(status);
  }

  status =
      writeRegister(AS5048A_REG_ZERO_POSITION_LOW, AS5048A_ZERO_POSITION_LOW_VALUE(zeroPosition));

  status.operation = Operation::WriteZero;

  return record(status);
}

As5048a::Status As5048a::setCurrentPositionAsZero() {
  // Clear previous offset before measuring the current position.
  Status status = writeZeroPosition(0U);

  if (!status.ok()) {
    return status;
  }

  auto angle = readRegister(AS5048A_REG_ANGLE);

  if (!angle.ok()) {
    return angle.status;
  }

  return writeZeroPosition(angle.value & AS5048A_ZERO_POSITION_MASK);
}

/* -------------------------------------------------------------------------- */
/* Communication diagnostics                                                  */
/* -------------------------------------------------------------------------- */

As5048a::Result<uint16_t> As5048a::clearCommunicationErrors() {
  continuousAngleReadPrimed_ = false;

  auto result = readRegister(AS5048A_REG_CLEAR_ERROR);

  result.status.operation = Operation::ClearErrors;

  if (result.ok()) {
    result.status.sensor_errors = result.value;
    result.status.sensor_errors_valid = true;
  }

  result.status = record(result.status);

  return result;
}
