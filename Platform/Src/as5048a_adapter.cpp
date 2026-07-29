#include "as5048a_adapter.h"
#include "as5048a/as5048a.hpp"

#include "main.h"
#include "spi.h"
#include "stdio.h"
#include "stm32g4xx_hal_def.h"

/*
 * Board-specific wiring belongs here, not in Components/AS5048A.
 * Rename the ENC0/ENC1/ENC2 GPIO labels below to your CubeMX labels.
 * The examples assume a shared SPI bus with one chip select per encoder.
 */

As5048a::Config enc1_cfg{&hspi1, {ENC0_CS_GPIO_Port, ENC0_CS_Pin}, 100U};

As5048a::Config enc2_cfg{&hspi1, {ENC1_CS_GPIO_Port, ENC1_CS_Pin}, 100U};

As5048a::Config enc3_cfg{&hspi1, {ENC2_CS_GPIO_Port, ENC2_CS_Pin}, 100U};

As5048a enc1{enc1_cfg};
As5048a enc2{enc2_cfg};
As5048a enc3{enc3_cfg};

namespace {
As5048a *driverFor(const As5048a_Instance_t instance) noexcept {
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
} // namespace

extern "C" HAL_StatusTypeDef as5048a_init(void) {
    printf("AS5048A Init\r\n");
    HAL_StatusTypeDef status = enc1.initialize();
    if (status != HAL_OK) {
        return status;
    }
    return HAL_OK;

    // status = enc2.initialize();
    // if (status != HAL_OK) {
    //     return status;
    // }
    //
    // return enc3.initialize();
}

extern "C" HAL_StatusTypeDef
as5048a_init_instance(const As5048a_Instance_t instance) {
    As5048a *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->initialize() : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef as5048a_sample(const As5048a_Instance_t instance,
                                            AS5048A_Sample_t *const sample) {
    if (sample == nullptr) {
        return HAL_ERROR;
    }

    As5048a *const driver = driverFor(instance);
    if (driver == nullptr) {
        return HAL_ERROR;
    }

    return driver->sample(*sample);
}

extern "C" HAL_StatusTypeDef
as5048a_begin_continuous_angle_read(const As5048a_Instance_t instance) {
    As5048a *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->beginContinuousAngleRead() : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
as5048a_read_next_angle(const As5048a_Instance_t instance,
                        AS5048A_Angle_t *const angle) {
    if (angle == nullptr) {
        return HAL_ERROR;
    }

    As5048a *const driver = driverFor(instance);
    if (driver == nullptr) {
        return HAL_ERROR;
    }

    return driver->readNextAngle(*angle);
}

extern "C" HAL_StatusTypeDef
as5048a_read_zero_position(const As5048a_Instance_t instance,
                           uint16_t *const zero_position) {
    if (zero_position == nullptr) {
        return HAL_ERROR;
    }

    As5048a *const driver = driverFor(instance);
    if (driver == nullptr) {
        return HAL_ERROR;
    }

    return driver->readZeroPosition(*zero_position);
}

extern "C" HAL_StatusTypeDef
as5048a_write_zero_position(const As5048a_Instance_t instance,
                            const uint16_t zero_position) {
    As5048a *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->writeZeroPosition(zero_position)
                               : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
as5048a_set_current_position_as_zero(const As5048a_Instance_t instance) {
    As5048a *const driver = driverFor(instance);
    return (driver != nullptr) ? driver->setCurrentPositionAsZero() : HAL_ERROR;
}

extern "C" HAL_StatusTypeDef
as5048a_clear_communication_errors(const As5048a_Instance_t instance,
                                   uint16_t *const raw_errors) {
    if (raw_errors == nullptr) {
        return HAL_ERROR;
    }

    As5048a *const driver = driverFor(instance);
    if (driver == nullptr) {
        return HAL_ERROR;
    }

    return driver->clearCommunicationErrors(*raw_errors);
}
