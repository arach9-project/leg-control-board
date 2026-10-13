#include "as5048a_adapter.h"
#include "as5048a/as5048a.hpp"

#include "main.h"
#include "spi.h"
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* Encoder configuration                                                      */
/* -------------------------------------------------------------------------- */

As5048a::Config enc1_cfg{&hspi1, {ENC0_CS_GPIO_Port, ENC0_CS_Pin}, 100U};

As5048a::Config enc2_cfg{&hspi1, {ENC1_CS_GPIO_Port, ENC1_CS_Pin}, 100U};

As5048a::Config enc3_cfg{&hspi1, {ENC2_CS_GPIO_Port, ENC2_CS_Pin}, 100U};

As5048a enc1{enc1_cfg};
As5048a enc2{enc2_cfg};
As5048a enc3{enc3_cfg};

namespace {

/* -------------------------------------------------------------------------- */
/* Per-encoder diagnostics                                                    */
/* -------------------------------------------------------------------------- */

// Reuse the Status type from the AS5048A library.
// Unlike lastStatus(), these retain the most recent failure.
as5048a::Status last_failure[3]{};

volatile uint32_t failure_count[3]{};
volatile uint32_t success_count[3]{};

/* -------------------------------------------------------------------------- */
/* Driver lookup                                                              */
/* -------------------------------------------------------------------------- */

As5048a* driverFor(As5048a_Instance_t instance) noexcept {
  switch (instance) {
  case AS5048A_INSTANCE_1:
    return &enc1;

  case AS5048A_INSTANCE_2:
    return &enc2;

  case AS5048A_INSTANCE_3:
    return &enc3;

  default:
    return nullptr;
  }
}

int indexFor(As5048a_Instance_t instance) noexcept {
  switch (instance) {
  case AS5048A_INSTANCE_1:
    return 0;

  case AS5048A_INSTANCE_2:
    return 1;

  case AS5048A_INSTANCE_3:
    return 2;

  default:
    return -1;
  }
}

/* -------------------------------------------------------------------------- */
/* HAL conversion                                                             */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef toHalStatus(const as5048a::Status& status) noexcept {
  using Error = as5048a::Error;

  switch (status.error) {
  case Error::None:
    return HAL_OK;

  case Error::SpiBusy:
    return HAL_BUSY;

  case Error::SpiTimeout:
    return HAL_TIMEOUT;

  default:
    return HAL_ERROR;
  }
}

/* -------------------------------------------------------------------------- */
/* Status tracking                                                            */
/* -------------------------------------------------------------------------- */

HAL_StatusTypeDef recordStatus(As5048a_Instance_t instance,
                               const as5048a::Status& status) noexcept {
  const int index = indexFor(instance);

  if (index < 0) {
    return HAL_ERROR;
  }

  if (status.ok()) {
    success_count[index]++;
  } else {
    last_failure[index] = status;
    failure_count[index]++;
  }

  return toHalStatus(status);
}

HAL_StatusTypeDef invalidArgument(As5048a_Instance_t instance) noexcept {
  as5048a::Status status{};

  status.error = as5048a::Error::InvalidValue;
  status.hal_status = HAL_ERROR;

  return recordStatus(instance, status);
}

/* -------------------------------------------------------------------------- */
/* Diagnostic access                                                          */
/* -------------------------------------------------------------------------- */

const as5048a::Status* statusFor(As5048a_Instance_t instance) noexcept {
  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return nullptr;
  }

  return &driver->lastStatus();
}

} // namespace

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

extern "C" HAL_StatusTypeDef as5048a_init(void) {
  HAL_StatusTypeDef result = HAL_OK;

  const As5048a_Instance_t instances[] = {AS5048A_INSTANCE_1, AS5048A_INSTANCE_2,
                                          AS5048A_INSTANCE_3};

  for (const auto instance : instances) {
    const HAL_StatusTypeDef status = as5048a_init_instance(instance);

    if (status != HAL_OK && result == HAL_OK) {
      result = status;
    }
  }

  return result;
}

extern "C" HAL_StatusTypeDef as5048a_init_instance(As5048a_Instance_t instance) {
  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  const auto status = driver->initialize();

  return recordStatus(instance, status);
}

/* -------------------------------------------------------------------------- */
/* Complete sample                                                            */
/* -------------------------------------------------------------------------- */

extern "C" HAL_StatusTypeDef as5048a_sample(As5048a_Instance_t instance, AS5048A_Sample_t* sample) {
  if (sample == nullptr) {
    return invalidArgument(instance);
  }

  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  const auto result = driver->sample();

  // Preserve decoded measurements and flags even when the
  // sample fails magnetic-health validation.
  *sample = result.value;

  if (!result.ok()) {
    const auto& status = result.status;

    printf("\r\n[AS5048A %u] %s\r\n"
           "Operation: %s\r\n"
           "HAL status: %u\r\n"
           "SPI error: 0x%08lX\r\n"
           "TX frame: 0x%04X\r\n"
           "RX frame: 0x%04X\r\n"
           "Register: 0x%04X\r\n",
           static_cast<unsigned>(instance + 1), as5048a::errorName(status.error),
           as5048a::operationName(status.operation), static_cast<unsigned>(status.hal_status),
           static_cast<unsigned long>(status.spi_error),
           static_cast<unsigned>(status.transmitted_frame),
           static_cast<unsigned>(status.received_frame),
           static_cast<unsigned>(status.register_address));

    if (status.diagnostics_valid) {
      printf("Diagnostics: 0x%04X\r\n"
             "OCF: %u\r\n"
             "COF: %u\r\n"
             "Magnet too strong: %u\r\n"
             "Magnet too weak: %u\r\n"
             "AGC: %u\r\n",
             static_cast<unsigned>(status.diagnostics_raw),
             static_cast<unsigned>(status.offset_compensation_finished),
             static_cast<unsigned>(status.cordic_overflow),
             static_cast<unsigned>(status.magnet_too_strong),
             static_cast<unsigned>(status.magnet_too_weak), static_cast<unsigned>(status.agc));
    }

    if (status.magnitude_valid) {
      printf("Magnetic magnitude: %u\r\n", static_cast<unsigned>(status.magnitude_raw));
    }

    if (status.sensor_errors_valid) {
      printf("Sensor communication errors: 0x%04X\r\n",
             static_cast<unsigned>(status.sensor_errors));
    }
  }

  return recordStatus(instance, result.status);
}

/* -------------------------------------------------------------------------- */
/* Continuous angle acquisition                                               */
/* -------------------------------------------------------------------------- */

extern "C" HAL_StatusTypeDef as5048a_begin_continuous_angle_read(As5048a_Instance_t instance) {
  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  const auto status = driver->beginContinuousAngleRead();

  return recordStatus(instance, status);
}

extern "C" HAL_StatusTypeDef as5048a_is_continuous_angle_read_primed(As5048a_Instance_t instance,
                                                                     bool* primed) {
  if (primed == nullptr) {
    return invalidArgument(instance);
  }

  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  *primed = driver->continuousAngleReadPrimed();

  return HAL_OK;
}

extern "C" HAL_StatusTypeDef as5048a_read_next_angle(As5048a_Instance_t instance,
                                                     AS5048A_Angle_t* angle) {
  if (angle == nullptr) {
    return invalidArgument(instance);
  }

  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  const auto result = driver->readNextAngle();

  if (result.ok()) {
    *angle = result.value;
  }

  return recordStatus(instance, result.status);
}

/* -------------------------------------------------------------------------- */
/* Zero position                                                              */
/* -------------------------------------------------------------------------- */

extern "C" HAL_StatusTypeDef as5048a_read_zero_position(As5048a_Instance_t instance,
                                                        uint16_t* zero_position) {
  if (zero_position == nullptr) {
    return invalidArgument(instance);
  }

  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  const auto result = driver->readZeroPosition();

  if (result.ok()) {
    *zero_position = result.value;
  }

  return recordStatus(instance, result.status);
}

extern "C" HAL_StatusTypeDef as5048a_write_zero_position(As5048a_Instance_t instance,
                                                         uint16_t zero_position) {
  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  const auto status = driver->writeZeroPosition(zero_position);

  return recordStatus(instance, status);
}

extern "C" HAL_StatusTypeDef as5048a_set_current_position_as_zero(As5048a_Instance_t instance) {
  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  const auto status = driver->setCurrentPositionAsZero();

  return recordStatus(instance, status);
}

/* -------------------------------------------------------------------------- */
/* Communication errors                                                       */
/* -------------------------------------------------------------------------- */

extern "C" HAL_StatusTypeDef as5048a_clear_communication_errors(As5048a_Instance_t instance,
                                                                uint16_t* raw_errors) {
  if (raw_errors == nullptr) {
    return invalidArgument(instance);
  }

  As5048a* driver = driverFor(instance);

  if (driver == nullptr) {
    return HAL_ERROR;
  }

  const auto result = driver->clearCommunicationErrors();

  if (result.ok()) {
    *raw_errors = result.value;
  }

  return recordStatus(instance, result.status);
}

/* -------------------------------------------------------------------------- */
/* Diagnostic getters                                                         */
/* -------------------------------------------------------------------------- */

extern "C" uint32_t as5048a_get_last_error(As5048a_Instance_t instance) {
  const auto* status = statusFor(instance);

  if (status == nullptr) {
    return static_cast<uint32_t>(as5048a::Error::InvalidConfig);
  }

  return static_cast<uint32_t>(status->error);
}

extern "C" uint32_t as5048a_get_last_operation(As5048a_Instance_t instance) {
  const auto* status = statusFor(instance);

  if (status == nullptr) {
    return static_cast<uint32_t>(as5048a::Operation::None);
  }

  return static_cast<uint32_t>(status->operation);
}

extern "C" uint32_t as5048a_get_last_spi_error(As5048a_Instance_t instance) {
  const auto* status = statusFor(instance);

  return status != nullptr ? status->spi_error : 0U;
}

extern "C" uint16_t as5048a_get_last_tx_frame(As5048a_Instance_t instance) {
  const auto* status = statusFor(instance);

  return status != nullptr ? status->transmitted_frame : 0U;
}

extern "C" uint16_t as5048a_get_last_rx_frame(As5048a_Instance_t instance) {
  const auto* status = statusFor(instance);

  return status != nullptr ? status->received_frame : 0U;
}

/* -------------------------------------------------------------------------- */
/* Deferred diagnostic printing                                               */
/* -------------------------------------------------------------------------- */

extern "C" void as5048a_print_pending_errors(void) {
  static uint32_t printed_failure_count[3]{};

  const As5048a_Instance_t instances[] = {AS5048A_INSTANCE_1, AS5048A_INSTANCE_2,
                                          AS5048A_INSTANCE_3};

  for (uint32_t i = 0; i < 3; i++) {
    as5048a::Status failure{};
    uint32_t failures = 0;
    uint32_t successes = 0;
    bool primed = false;

    // Copy the cached status without allowing an ADC
    // interrupt to modify it halfway through the copy.
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();

    failures = failure_count[i];
    successes = success_count[i];

    if (failures != printed_failure_count[i]) {
      failure = last_failure[i];
      primed = driverFor(instances[i])->continuousAngleReadPrimed();
    }

    if (primask == 0U) {
      __enable_irq();
    }

    if (failures == printed_failure_count[i]) {
      continue;
    }

    printed_failure_count[i] = failures;

    printf("[AS5048A %lu] %s\r\n"
           "  Operation: %s\r\n"
           "  HAL status: %lu\r\n"
           "  SPI error: 0x%08lX\r\n"
           "  TX: 0x%04X\r\n"
           "  RX: 0x%04X\r\n"
           "  Register: 0x%04X\r\n"
           "  Pipeline primed: %u\r\n"
           "  Successful operations: %lu\r\n"
           "  Failed operations: %lu\r\n",
           static_cast<unsigned long>(i + 1), as5048a::errorName(failure.error),
           as5048a::operationName(failure.operation),
           static_cast<unsigned long>(failure.hal_status),
           static_cast<unsigned long>(failure.spi_error),
           static_cast<unsigned int>(failure.transmitted_frame),
           static_cast<unsigned int>(failure.received_frame),
           static_cast<unsigned int>(failure.register_address), static_cast<unsigned int>(primed),
           static_cast<unsigned long>(successes), static_cast<unsigned long>(failures));

    if (failure.sensor_errors_valid) {
      printf("  Sensor error register: 0x%04X\r\n",
             static_cast<unsigned int>(failure.sensor_errors));
    }
  }
}
