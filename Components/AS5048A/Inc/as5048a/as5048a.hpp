#ifndef AS5048A_HPP
#define AS5048A_HPP

#include "as5048a/registers.h"
#include "as5048a/types.h"

#include "stm32g4xx_hal.h"

namespace as5048a {

enum class Error : uint8_t {
  None = 0,

  // Configuration and arguments
  InvalidConfig,
  InvalidAddress,
  InvalidValue,
  InvalidField,

  // STM32 SPI transport
  SpiBusy,
  SpiTimeout,
  SpiError,

  // AS5048A protocol
  ParityError,
  SensorErrorFlag,
  PipelineNotPrimed,

  // Sensor diagnostics
  OffsetCompensationIncomplete,
  CordicOverflow,
  MagnetTooStrong,
  MagnetTooWeak,
};

enum class Operation : uint8_t {
  None,
  Initialize,
  Transfer,
  ReadRegister,
  WriteRegister,
  ReadAngle,
  ReadDiagnostics,
  ReadMagnitude,
  BeginContinuous,
  ContinuousAngle,
  ReadZero,
  WriteZero,
  ClearErrors,
};

struct Status {
  Error error{Error::None};
  Operation operation{Operation::None};

  // STM32 SPI transport
  HAL_StatusTypeDef hal_status{HAL_OK};
  uint32_t spi_error{HAL_SPI_ERROR_NONE};

  // SPI transaction
  uint16_t transmitted_frame{0};
  uint16_t received_frame{0};
  uint16_t register_address{0};

  // AS5048A communication-error register
  uint16_t sensor_errors{0};
  bool sensor_errors_valid{false};

  // Magnetic diagnostics
  uint16_t diagnostics_raw{0};
  uint16_t magnitude_raw{0};
  uint8_t agc{0};

  bool diagnostics_valid{false};
  bool magnitude_valid{false};

  bool offset_compensation_finished{false};
  bool cordic_overflow{false};
  bool magnet_too_strong{false};
  bool magnet_too_weak{false};

  [[nodiscard]] bool ok() const noexcept {
    return error == Error::None;
  }

  explicit operator bool() const noexcept {
    return ok();
  }
};

template <typename T> struct Result {
  T value{};
  Status status{};

  [[nodiscard]] bool ok() const noexcept {
    return status.ok();
  }

  explicit operator bool() const noexcept {
    return ok();
  }
};

const char* errorName(Error error) noexcept;
const char* operationName(Operation operation) noexcept;

} // namespace as5048a

class As5048a final {
public:
  struct Pin {
    GPIO_TypeDef* port{nullptr};
    uint16_t number{0};

    [[nodiscard]] bool isValid() const noexcept {
      return port != nullptr;
    }
  };

  struct Config {
    SPI_HandleTypeDef* spi{nullptr};
    Pin chipSelect{};
    uint32_t spiTimeoutMs{100};
  };

  explicit As5048a(const Config& config) noexcept;

  As5048a(const As5048a&) = delete;
  As5048a& operator=(const As5048a&) = delete;
  As5048a(As5048a&&) = delete;
  As5048a& operator=(As5048a&&) = delete;

  using Status = as5048a::Status;
  using Error = as5048a::Error;
  using Operation = as5048a::Operation;

  template <typename T> using Result = as5048a::Result<T>;

  Status initialize();

  Result<AS5048A_Sample_t> sample();

  // Fast acquisition
  Status beginContinuousAngleRead();

  [[nodiscard]] bool continuousAngleReadPrimed() const noexcept;

  Result<AS5048A_Angle_t> readNextAngle();

  // Sensor configuration
  Result<uint16_t> readZeroPosition();
  Status writeZeroPosition(uint16_t zeroPosition);
  Status setCurrentPositionAsZero();

  // Diagnostics
  Result<uint16_t> clearCommunicationErrors();

  [[nodiscard]] const Status& lastStatus() const noexcept {
    return last_status_;
  }

private:
  static void enableCycleCounter() noexcept;
  static void delayMicroseconds(uint32_t microseconds) noexcept;

  static uint8_t countSetBits(uint16_t word) noexcept;
  static bool requiresParityBit(uint16_t frame) noexcept;

  static uint16_t makeReadCommand(uint16_t address) noexcept;

  Status transferFrame(uint16_t transmitFrame, uint16_t& receiveFrame);

  Status validateResponse(uint16_t frame, uint16_t address, Operation operation);

  Result<uint16_t> readRegister(uint16_t address);

  Status writeRegister(uint16_t address, uint16_t value, uint16_t* receiveFrame = nullptr);

  Result<uint16_t> getField(uint16_t address, uint16_t mask, uint8_t offset);

  Status setField(uint16_t address, uint16_t mask, uint8_t offset, uint16_t value);

  Status record(Status status) noexcept;

  Config config_{};

  bool continuousAngleReadPrimed_{false};

  Status last_status_{};
};

#endif
