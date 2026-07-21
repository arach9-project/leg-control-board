
#include "drv8316.h"
#include "main.h"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_spi.h"
#include <stdbool.h> // Provides bool, true, false
#include <stdint.h>
#include <stdio.h>
// one-time init, e.g. in main() after HAL_Init()

// delay function
static inline void delay_us_dwt(uint32_t us) {
    uint32_t cycles = (SystemCoreClock / 1000000UL) * us;
    uint32_t start = DWT->CYCCNT;

    while ((DWT->CYCCNT - start) < cycles) {
    }
}

HAL_StatusTypeDef DRV8316_SPI_Transfer(SPI_HandleTypeDef *hspi,
                                       GPIO_TypeDef *nscs_port,
                                       uint16_t nscs_pin, uint16_t *tx_frame,
                                       uint16_t *rx_frame) {
    HAL_StatusTypeDef ret;

    if ((hspi == NULL) || (nscs_port == NULL) || (tx_frame == NULL) ||
        (rx_frame == NULL)) {
        return HAL_ERROR;
    }

    uint8_t tx_bytes[2];
    uint8_t rx_bytes[2] = {0U, 0U};

    tx_bytes[0] = (*tx_frame >> 8) & 0xFFU; // MSB first: R/W, ADDR, parity
    tx_bytes[1] = *tx_frame & 0xFFU;        // LSB: data

    if (HAL_SPI_GetState(hspi) != HAL_SPI_STATE_READY) {
        return HAL_BUSY;
    }
    //
    HAL_GPIO_WritePin(nscs_port, nscs_pin, GPIO_PIN_RESET);

    ret = HAL_SPI_TransmitReceive(hspi, tx_bytes, rx_bytes, 2U, 100U);
    if (ret == HAL_OK) {
        while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY) != RESET) {
        }
    }

    HAL_GPIO_WritePin(nscs_port, nscs_pin, GPIO_PIN_SET);

    delay_us_dwt(1);

    if (ret != HAL_OK) {
        *rx_frame = 0U;
        return ret;
    }

    *rx_frame = ((uint16_t)rx_bytes[0] << 8) | ((uint16_t)rx_bytes[1]);

    return HAL_OK;
}

uint8_t count_bits(const uint16_t word) {
    uint16_t temp = word;
    uint8_t ones_count = 0;
    while (temp) {
        ones_count += (temp & 1);
        temp >>= 1;
    }
    return ones_count;
}

uint8_t even_parity(const uint16_t tx_frame) {
    return (count_bits(tx_frame) & 1U) != 0U;
}

HAL_StatusTypeDef DRV8316_Read_Register(DRV8316_HandleTypeDef *hdrv,
                                        const uint8_t address, uint8_t *value) {
    if ((hdrv == NULL) || (hdrv->hspi == NULL) || (value == NULL)) {
        return HAL_ERROR;
    }

    uint16_t tx_frame = 0;
    uint16_t rx_frame = 0;
    HAL_StatusTypeDef ret;

    tx_frame |= (1U << 15);

    tx_frame |= ((uint16_t)(address & 0x3F) << 9);

    if (even_parity(tx_frame)) {
        tx_frame |= (1U << 8);
    }

    ret = DRV8316_SPI_Transfer(hdrv->hspi, hdrv->cs_port, hdrv->cs_pin,
                               &tx_frame, &rx_frame);

    if (ret != HAL_OK) {
        return ret;
    }

    *value = (uint8_t)(rx_frame & 0xFF);

    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Write_Register(DRV8316_HandleTypeDef *hdrv,
                                         const uint8_t address,
                                         const uint8_t value, uint16_t *rx) {

    if ((hdrv == NULL) || (hdrv->hspi == NULL)) {
        return HAL_ERROR;
    }

    uint16_t tx_frame = 0;
    uint16_t rx_frame = 0;
    HAL_StatusTypeDef ret;

    tx_frame |= ((uint16_t)(address & 0x3FU) << 9); // address
    tx_frame |= (uint16_t)value;                    // new register value

    if (even_parity(tx_frame)) {
        tx_frame |= (1U << 8);
    }

    ret = DRV8316_SPI_Transfer(hdrv->hspi, hdrv->cs_port, hdrv->cs_pin,
                               &tx_frame, &rx_frame);

    if (ret != HAL_OK) {
        // printf("DRV8316 write failed: address=0x%02X HAL=%d\r\n", address,
        //        (int)ret);

        return ret;
    }

    if (rx != NULL) {
        *rx = rx_frame;
    }

    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Update_Diagnostics(DRV8316_HandleTypeDef *hdrv) {
    uint8_t reg_val = 0;
    HAL_StatusTypeDef ret;

    // Read and parse Register 0x00
    ret = DRV8316_Read_Register(hdrv, 0x00, &reg_val);
    if (ret != HAL_OK)
        return ret;
    hdrv->diagnostics.ic_status.raw = (uint8_t)(reg_val & 0x00FF);

    // If the master fault bit or flags are active, read the deep details
    if (hdrv->diagnostics.ic_status.fault_active ||
        hdrv->diagnostics.ic_status.raw != 0) {

        ret = DRV8316_Read_Register(hdrv, 0x01, &reg_val);
        if (ret == HAL_OK)
            hdrv->diagnostics.status_1.raw = reg_val;

        ret = DRV8316_Read_Register(hdrv, 0x02, &reg_val);
        if (ret == HAL_OK)
            hdrv->diagnostics.status_2.raw = reg_val;
    }

    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Init(DRV8316_HandleTypeDef *hdrv,
                               SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port,
                               uint16_t cs_pin, GPIO_TypeDef *nfault_port,
                               uint16_t nfault_pin, GPIO_TypeDef *nSLEEP_port,
                               uint16_t nSleep_pin) {
    if ((hdrv == NULL) || (hspi == NULL)) {
        return HAL_ERROR;
    }

    hdrv->hspi = hspi;
    hdrv->cs_port = cs_port;
    hdrv->cs_pin = cs_pin;
    hdrv->nfault_port = nfault_port;
    hdrv->nfault_pin = nfault_pin;
    hdrv->nSleep_port = nSLEEP_port;
    hdrv->nSleep_pin = nSleep_pin;

    HAL_GPIO_WritePin(hdrv->cs_port, hdrv->cs_pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(hdrv->nSleep_port, hdrv->nSleep_pin, GPIO_PIN_SET);

    HAL_Delay(2);
    DRV8316_Set_Register_Lock(hdrv, DRV8316_REG_LOCK_UNLOCK);

    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_Register_Lock(DRV8316_HandleTypeDef *hdrv,
                          const DRV8316_Register_Lock_Status_t val) {
    uint16_t rx_buf;
    if (DRV8316_Write_Register(hdrv, DRV8316_REG_CTRL_1, val, &rx_buf) !=
        HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Get_Register_Lock(DRV8316_HandleTypeDef *hdrv,
                          DRV8316_Register_Lock_Status_t *state) {
    uint8_t ctrl1;

    if ((hdrv == NULL) || (state == NULL)) {
        return HAL_ERROR;
    }

    if (DRV8316_Read_Register(hdrv, DRV8316_REG_CTRL_1, &ctrl1) != HAL_OK) {
        return HAL_ERROR;
    }

    switch (ctrl1 & 0x07) // Extract bits [2:0]
    {
        case 0x03:
            *state = DRV8316_REG_LOCK_UNLOCK;
            break;

        case 0x06:
            *state = DRV8316_REG_LOCK_LOCK;
            break;

        default:
            return HAL_ERROR;
    }

    return HAL_OK;
}

// void DRV8316Driver3PWM::init(SPIClass *_spi) {
//     DRV8316Driver::init(_spi);
//     setRegistersLocked(false);
//     delayMicroseconds(1);
//     DRV8316Driver::setPWMMode(DRV8316_PWMMode::PWM3_Mode);
//     BLDCDriver3PWM::init();
// };
//
// void DRV8316Driver6PWM::init(SPIClass *_spi) {
//     DRV8316Driver::init(_spi);
//     setRegistersLocked(false);
//     delayMicroseconds(1);
//     DRV8316Driver::setPWMMode(
//         DRV8316_PWMMode::PWM6_Mode); // default mode is 6-PWM
//     BLDCDriver6PWM::init();
// };
//
//
// void handleInterrupt() {
// }
//
// void DRV8316Driver::init(SPIClass *_spi) {
//     // TODO make SPI speed configurable
//     spi = _spi;
//     settings = SPISettings(1000000, MSBFIRST, SPI_MODE1);
//
//     // setup pins
//     pinMode(cs, OUTPUT);
//     digitalWrite(cs, HIGH); // switch off
//
//     // SPI has an internal SPI-device counter, it is possible to call
//     "begin()"
//     // from different devices
//     spi->begin();
//
//     if (_isset(nFault)) {
//         pinMode(nFault, INPUT);
//         // TODO add interrupt handler on the nFault pin if configured
//         // add configuration for how to handle faults... idea: interrupt
//         handler
//         // calls a callback, depending on the type of fault consider what
//         would
//         // be a useful configuration in practice? What do we want to do on a
//         // fault, e.g. over-temperature for example?
//
//         // attachInterrupt(digitalPinToInterrupt(nFault), handleInterrupt,
//         // PinStatus::FALLING);
//     }
// };
//
//
//

HAL_StatusTypeDef DRV8316_Get_IC_Status(DRV8316_HandleTypeDef *hdrv) {
    // IC_Status data;
    // Status__1 data1;
    // Status__2 data2;
    // uint16_t result = readSPI(IC_Status_ADDR);
    // data.reg = (result & 0x00FF);
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = readSPI(Status__1_ADDR);
    // data1.reg = (result & 0x00FF);
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = readSPI(Status__2_ADDR);
    // data2.reg = (result & 0x00FF);
    // return DRV8316Status(data, data1, data2);
}

HAL_StatusTypeDef DRV8316_Clear_Fault(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__2_ADDR);
    // Control__2 data;
    // data.reg = (result & 0x00FF);
    // data.CLR_FLT |= 1;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__2_ADDR, data.reg);
};

/*
 * CTRL2 Register (0x04)
 *
 *  Bit:   7   6   5   4   3   2   1   0
 *       +---+---+---+---+---+---+---+---+
 * CTRL2 |   |   |   |   |   | P | P | C |
 *       +---+---+---+---+---+---+---+---+
 *                             ^^^^^   ^
 *                        PWM_MODE  CLR_FLT
 *
 * PWM_MODE (Bits 2:1)
 *   00 = 6x PWM
 *   01 = 6x PWM + Current Limit
 *   10 = 3x PWM
 *   11 = 3x PWM + Current Limit
 */
HAL_StatusTypeDef DRV8316_Set_PWM_Mode(DRV8316_HandleTypeDef *hdrv,
                                       DRV8316_PWM_Mode_t mode) {
    uint8_t ctrl_2 = 0;
    if (DRV8316_Read_Register(hdrv, DRV8316_REG_CTRL_2, &ctrl_2) != HAL_OK) {
        return HAL_ERROR;
    };
    ctrl_2 &= ~(0x03U << 1);
    ctrl_2 |= (((uint8_t)mode & 0x03U) << 1);

    DRV8316_Write_Register(hdrv, DRV8316_REG_CTRL_2, ctrl_2, NULL);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Get_PWM_Mode(DRV8316_HandleTypeDef *hdrv,
                                       DRV8316_PWM_Mode_t *mode) {
    uint8_t ctrl_2 = 0;
    if (DRV8316_Read_Register(hdrv, DRV8316_REG_CTRL_2, &ctrl_2) != HAL_OK) {
        return HAL_ERROR;
    };
    *mode = (DRV8316_PWM_Mode_t)((ctrl_2 >> 1) & 0x03);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Get_Slew(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__2_ADDR);
    // Control__2 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_Slew)data.SLEW;
    // TODO: Implement this
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Set_Slew(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__2_ADDR);
    // Control__2 data;
    // data.reg = (result & 0x00FF);
    // data.SLEW = slewRate;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__2_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Get_SDO_Mode(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__2_ADDR);
    // Control__2 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_SDOMode)data.SDO_MODE;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Set_SDO_Mode(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__2_ADDR);
    // Control__2 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_SDOMode)data.SDO_MODE;
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_Is_Overtemperature_Reporting(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // return data.OTW_REP == OTW_REP_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_Set_Overtemperature_Reporting(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // data.OTW_REP = reportFault ? OTW_REP_ENABLE : OTW_REP_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__3_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Is_SPI_Fault_Reporting(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // return data.SPI_FLT_REP == SPI_FLT_REP_ENABLE;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_SPI_Fault_Reporting(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // data.SPI_FLT_REP = reportFault ? SPI_FLT_REP_ENABLE :
    // SPI_FLT_REP_DISABLE; delayMicroseconds(1); // delay at least 400ns
    // between operations result = writeSPI(Control__3_ADDR, data.reg);
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Is_Overvoltage_Protection(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // return data.OVP_EN == OVP_EN_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_Set_Overvoltage_Protection(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // data.OVP_EN = enabled ? OVP_EN_ENABLE : OVP_EN_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__3_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Get_Overvoltage_Level(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_OVP)data.OVP_SEL;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Set_Overvoltage_Level(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // data.OVP_SEL = voltage;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__3_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Get_PWM_100_Frequency(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_PWM100DUTY)data.PWM_100_DUTY_SEL;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Set_PWM_100_Frequency(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__3_ADDR);
    // Control__3 data;
    // data.reg = (result & 0x00FF);
    // data.PWM_100_DUTY_SEL = freq;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__3_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Get_OCP_Mode(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_OCPMode)data.OCP_MODE;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Set_OCP_Mode(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // data.OCP_MODE = ocpMode;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__4_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Get_OCP_Level(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_OCPLevel)data.OCP_LVL;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Set_OCP_Level(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // data.OCP_LVL = amps;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__4_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_Get_OCP_Retry_Time(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_OCPRetry)data.OCP_RETRY;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setOCPRetryTime(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // data.OCP_RETRY = ms;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__4_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_getOCPDeglitchTime(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_OCPDeglitch)data.OCP_DEG;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setOCPDeglitchTime(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // data.OCP_DEG = ms;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__4_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_isOCPClearInPWMCycleChange(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // return data.OCP_CBC == OCP_CBC_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_setOCPClearInPWMCycleChange(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // data.OCP_CBC = enable ? OCP_CBC_ENABLE : OCP_CBC_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__4_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_isDriverOffEnabled(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // return data.DRV_OFF == DRV_OFF_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setDriverOffEnabled(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__4_ADDR);
    // Control__4 data;
    // data.reg = (result & 0x00FF);
    // data.DRV_OFF = enabled ? DRV_OFF_ENABLE : DRV_OFF_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__4_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_getCurrentSenseGain(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__5_ADDR);
    // Control__5 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_CSAGain)data.CSA_GAIN;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setCurrentSenseGain(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__5_ADDR);
    // Control__5 data;
    // data.reg = (result & 0x00FF);
    // data.CSA_GAIN = gain;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__5_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_isActiveSynchronousRectificationEnabled(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__5_ADDR);
    // Control__5 data;
    // data.reg = (result & 0x00FF);
    // return data.EN_ASR == EN_ASR_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_setActiveSynchronousRectificationEnabled(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__5_ADDR);
    // Control__5 data;
    // data.reg = (result & 0x00FF);
    // data.EN_ASR = enabled ? EN_ASR_ENABLE : EN_ASR_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__5_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_isActiveAsynchronousRectificationEnabled(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__5_ADDR);
    // Control__5 data;
    // data.reg = (result & 0x00FF);
    // return data.EN_AAR == EN_AAR_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef
DRV8316_setActiveAsynchronousRectificationEnabled(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__5_ADDR);
    // Control__5 data;
    // data.reg = (result & 0x00FF);
    // data.EN_AAR = enabled ? EN_AAR_ENABLE : EN_AAR_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__5_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_getRecirculationMode(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__5_ADDR);
    // Control__5 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_Recirculation)data.ILIM_RECIR;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setRecirculationMode(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__5_ADDR);
    // Control__5 data;
    // data.reg = (result & 0x00FF);
    // data.ILIM_RECIR = recirculationMode;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__5_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_isBuckEnabled(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__6_ADDR);
    // Control__6 data;
    // data.reg = (result & 0x00FF);
    // return data.BUCK_DIS == BUCK_DIS_BUCK_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setBuckEnabled(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__6_ADDR);
    // Control__6 data;
    // data.reg = (result & 0x00FF);
    // data.BUCK_DIS = enabled ? BUCK_DIS_BUCK_ENABLE : BUCK_DIS_BUCK_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__6_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_getBuckVoltage(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__6_ADDR);
    // Control__6 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_BuckVoltage)data.BUCK_SEL;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setBuckVoltage(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__6_ADDR);
    // Control__6 data;
    // data.reg = (result & 0x00FF);
    // data.BUCK_SEL = volts;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__6_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_getBuckCurrentLimit(DRV8316_HandleTypeDef *hdrv) {
    // uint16_t result = readSPI(Control__6_ADDR);
    // Control__6 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_BuckCurrentLimit)data.BUCK_CL;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setBuckCurrentLimit(DRV8316_BuckCurrentLimit mamps) {
    // uint16_t result = readSPI(Control__6_ADDR);
    // Control__6 data;
    // data.reg = (result & 0x00FF);
    // data.BUCK_CL = mamps;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__6_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_isBuckPowerSequencingEnabled() {
    // uint16_t result = readSPI(Control__6_ADDR);
    // Control__6 data;
    // data.reg = (result & 0x00FF);
    // return data.BUCK_PS_DIS == BUCK_PS_DIS_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setBuckPowerSequencingEnabled(bool enabled) {
    // uint16_t result = readSPI(Control__6_ADDR);
    // Control__6 data;
    // data.reg = (result & 0x00FF);
    // data.BUCK_PS_DIS = enabled ? BUCK_PS_DIS_ENABLE : BUCK_PS_DIS_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns
    // between operations result = writeSPI(Control__6_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_getDelayTarget() {
    // uint16_t result = readSPI(Control__10_ADDR);
    // Control__10 data;
    // data.reg = (result & 0x00FF);
    // return (DRV8316_DelayTarget)data.DLY_TARGET;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setDelayTarget(DRV8316_DelayTarget us) {
    // uint16_t result = readSPI(Control__10_ADDR);
    // Control__10 data;
    // data.reg = (result & 0x00FF);
    // data.DLY_TARGET = us;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__10_ADDR, data.reg);
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_isDelayCompensationEnabled() {
    // uint16_t result = readSPI(Control__10_ADDR);
    // Control__10 data;
    // data.reg = (result & 0x00FF);
    // return data.DLYCMP_EN == DLYCMP_EN_ENABLE;
    return HAL_OK;
};

HAL_StatusTypeDef DRV8316_setDelayCompensationEnabled(bool enabled) {
    // uint16_t result = readSPI(Control__10_ADDR);
    // Control__10 data;
    // data.reg = (result & 0x00FF);
    // data.DLYCMP_EN = enabled ? DLYCMP_EN_ENABLE : DLYCMP_EN_DISABLE;
    // delayMicroseconds(1); // delay at least 400ns between operations
    // result = writeSPI(Control__10_ADDR, data.reg);
    return HAL_OK;
};
