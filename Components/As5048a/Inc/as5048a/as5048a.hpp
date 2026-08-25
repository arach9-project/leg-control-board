#ifndef AS5048A_HPP
#define AS5048A_HPP

#include "as5048a/registers.h"
#include "as5048a/types.h"
#include "stdint.h"
#include "stm32g4xx_hal.h"

class As5048a final {
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
        Pin chipSelect{};
        uint32_t spiTimeoutMs{100U};
    };

    explicit As5048a(const Config &config) noexcept;

    As5048a(const As5048a &) = delete;
    As5048a &operator=(const As5048a &) = delete;
    As5048a(As5048a &&) = delete;
    As5048a &operator=(As5048a &&) = delete;

    HAL_StatusTypeDef initialize();

    // Reads angle + magnitude + diagnostics in one shot. `sample` is always
    // fully written, even on failure; `sample.valid` is only true when the
    // return is HAL_OK.
    HAL_StatusTypeDef sample(AS5048A_Sample_t &sample);

    // Fast path for polling loops: call beginContinuousAngleRead() once,
    // then readNextAngle() repeatedly (skips the magnitude/diagnostics
    // reads that sample() does every time).
    HAL_StatusTypeDef beginContinuousAngleRead();
    HAL_StatusTypeDef readNextAngle(AS5048A_Angle_t &angle);

    HAL_StatusTypeDef readZeroPosition(uint16_t &zeroPosition);
    HAL_StatusTypeDef writeZeroPosition(uint16_t zeroPosition);
    HAL_StatusTypeDef setCurrentPositionAsZero();

    // Reads CLEAR_ERROR (0x0001); reading it also clears the sensor's
    // latched communication-error flags.
    HAL_StatusTypeDef clearCommunicationErrors(uint16_t &rawErrors);

  private:
    static void enableCycleCounter() noexcept;
    static void delayMicroseconds(uint32_t microseconds) noexcept;
    static uint8_t countSetBits(uint16_t word) noexcept;
    static bool requiresParityBit(uint16_t frame) noexcept;
    static bool isValueInRange(uint16_t value, uint16_t maximum) noexcept;

    HAL_StatusTypeDef transferFrame(uint16_t transmitFrame,
                                    uint16_t &receiveFrame);
    HAL_StatusTypeDef readRegister(uint16_t address, uint16_t &value);
    HAL_StatusTypeDef writeRegister(uint16_t address, uint16_t value,
                                    uint16_t *receiveFrame = nullptr);
    HAL_StatusTypeDef getField(uint16_t registerAddress, uint16_t mask,
                               uint8_t offset, uint16_t &value);
    HAL_StatusTypeDef setField(uint16_t registerAddress, uint16_t mask,
                               uint8_t offset, uint16_t value);

    Config config_{};
    bool continuousAngleReadPrimed_{false};
};
#endif // !AS5048A_HPP
