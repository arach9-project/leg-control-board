
#include "drv8316.h"
#include "drv8316_fields.h"
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

/*
 * These helpers expect masks that are already shifted into their register
 * positions. They preserve all unrelated register bits.
 */

static HAL_StatusTypeDef DRV8316_Get_Field(DRV8316_HandleTypeDef *hdrv,
                                           uint8_t register_address,
                                           uint8_t mask, uint8_t offset,
                                           uint8_t *value) {
    uint8_t reg_value = 0U;

    if ((hdrv == NULL) || (value == NULL)) {
        return HAL_ERROR;
    }

    if (DRV8316_Read_Register(hdrv, register_address, &reg_value) != HAL_OK) {
        return HAL_ERROR;
    }

    *value = (uint8_t)((reg_value & mask) >> offset);
    return HAL_OK;
}

static HAL_StatusTypeDef DRV8316_Set_Field(DRV8316_HandleTypeDef *hdrv,
                                           uint8_t register_address,
                                           uint8_t mask, uint8_t offset,
                                           uint8_t value) {
    uint8_t reg_value = 0U;

    if (hdrv == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Read_Register(hdrv, register_address, &reg_value) != HAL_OK) {
        return HAL_ERROR;
    }

    reg_value &= (uint8_t)~mask;
    reg_value |= (uint8_t)((value << offset) & mask);

    return DRV8316_Write_Register(hdrv, register_address, reg_value, NULL);
}

/* ============================================================
 * CTRL_3
 * ============================================================ */

HAL_StatusTypeDef
DRV8316_Get_Overtemperature_Reporting(DRV8316_HandleTypeDef *hdrv,
                                      DRV8316_OTW_Report_t *reporting) {
    uint8_t value = 0U;

    if (reporting == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_3, DRV8316_CTRL3_OTW_REP_MASK,
                          DRV8316_CTRL3_OTW_REP_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *reporting = (DRV8316_OTW_Report_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_Overtemperature_Reporting(DRV8316_HandleTypeDef *hdrv,
                                      DRV8316_OTW_Report_t reporting) {
    if ((uint8_t)reporting > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_3,
                             DRV8316_CTRL3_OTW_REP_MASK,
                             DRV8316_CTRL3_OTW_REP_OFFSET, (uint8_t)reporting);
}

HAL_StatusTypeDef
DRV8316_Get_Overvoltage_Protection(DRV8316_HandleTypeDef *hdrv,
                                   DRV8316_OVP_Enable_t *enabled) {
    uint8_t value = 0U;

    if (enabled == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_3, DRV8316_CTRL3_OVP_EN_MASK,
                          DRV8316_CTRL3_OVP_EN_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *enabled = (DRV8316_OVP_Enable_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_Overvoltage_Protection(DRV8316_HandleTypeDef *hdrv,
                                   DRV8316_OVP_Enable_t enabled) {
    if ((uint8_t)enabled > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_3,
                             DRV8316_CTRL3_OVP_EN_MASK,
                             DRV8316_CTRL3_OVP_EN_OFFSET, (uint8_t)enabled);
}

HAL_StatusTypeDef DRV8316_Get_Overvoltage_Level(DRV8316_HandleTypeDef *hdrv,
                                                DRV8316_OVP_Level_t *level) {
    uint8_t value = 0U;

    if (level == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_3, DRV8316_CTRL3_OVP_SEL_MASK,
                          DRV8316_CTRL3_OVP_SEL_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *level = (DRV8316_OVP_Level_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_Overvoltage_Level(DRV8316_HandleTypeDef *hdrv,
                                                DRV8316_OVP_Level_t level) {
    if ((uint8_t)level > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_3,
                             DRV8316_CTRL3_OVP_SEL_MASK,
                             DRV8316_CTRL3_OVP_SEL_OFFSET, (uint8_t)level);
}

HAL_StatusTypeDef
DRV8316_Get_PWM_100_Frequency(DRV8316_HandleTypeDef *hdrv,
                              DRV8316_PWM100DutyFrequency_t *frequency) {
    uint8_t value = 0U;

    if (frequency == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(
            hdrv, DRV8316_REG_CTRL_3, DRV8316_CTRL3_PWM_100_DUTY_MASK,
            DRV8316_CTRL3_PWM_100_DUTY_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *frequency = (DRV8316_PWM100DutyFrequency_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_PWM_100_Frequency(DRV8316_HandleTypeDef *hdrv,
                              DRV8316_PWM100DutyFrequency_t frequency) {
    if ((uint8_t)frequency > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(
        hdrv, DRV8316_REG_CTRL_3, DRV8316_CTRL3_PWM_100_DUTY_MASK,
        DRV8316_CTRL3_PWM_100_DUTY_OFFSET, (uint8_t)frequency);
}

/* ============================================================
 * CTRL_4
 * ============================================================ */

HAL_StatusTypeDef DRV8316_Get_OCP_Mode(DRV8316_HandleTypeDef *hdrv,
                                       DRV8316_OCP_Mode_t *mode) {
    uint8_t value = 0U;

    if (mode == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_MODE_MASK,
                          DRV8316_CTRL4_OCP_MODE_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *mode = (DRV8316_OCP_Mode_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_OCP_Mode(DRV8316_HandleTypeDef *hdrv,
                                       DRV8316_OCP_Mode_t mode) {
    if ((uint8_t)mode > 3U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_4,
                             DRV8316_CTRL4_OCP_MODE_MASK,
                             DRV8316_CTRL4_OCP_MODE_OFFSET, (uint8_t)mode);
}

HAL_StatusTypeDef DRV8316_Get_OCP_Level(DRV8316_HandleTypeDef *hdrv,
                                        DRV8316_OCP_Level_t *level) {
    uint8_t value = 0U;

    if (level == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_LVL_MASK,
                          DRV8316_CTRL4_OCP_LVL_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *level = (DRV8316_OCP_Level_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_OCP_Level(DRV8316_HandleTypeDef *hdrv,
                                        DRV8316_OCP_Level_t level) {
    if ((uint8_t)level > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_4,
                             DRV8316_CTRL4_OCP_LVL_MASK,
                             DRV8316_CTRL4_OCP_LVL_OFFSET, (uint8_t)level);
}

HAL_StatusTypeDef
DRV8316_Get_OCP_Retry_Time(DRV8316_HandleTypeDef *hdrv,
                           DRV8316_OCP_RetryTime_t *retry_time) {
    uint8_t value = 0U;

    if (retry_time == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_4,
                          DRV8316_CTRL4_OCP_RETRY_MASK,
                          DRV8316_CTRL4_OCP_RETRY_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *retry_time = (DRV8316_OCP_RetryTime_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_OCP_Retry_Time(DRV8316_HandleTypeDef *hdrv,
                           DRV8316_OCP_RetryTime_t retry_time) {
    if ((uint8_t)retry_time > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(
        hdrv, DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_RETRY_MASK,
        DRV8316_CTRL4_OCP_RETRY_OFFSET, (uint8_t)retry_time);
}

HAL_StatusTypeDef
DRV8316_Get_OCP_Deglitch_Time(DRV8316_HandleTypeDef *hdrv,
                              DRV8316_OCP_DeglitchTime_t *deglitch_time) {
    uint8_t value = 0U;

    if (deglitch_time == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_DEG_MASK,
                          DRV8316_CTRL4_OCP_DEG_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *deglitch_time = (DRV8316_OCP_DeglitchTime_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_OCP_Deglitch_Time(DRV8316_HandleTypeDef *hdrv,
                              DRV8316_OCP_DeglitchTime_t deglitch_time) {
    if ((uint8_t)deglitch_time > 3U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(
        hdrv, DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_DEG_MASK,
        DRV8316_CTRL4_OCP_DEG_OFFSET, (uint8_t)deglitch_time);
}

HAL_StatusTypeDef DRV8316_Get_OCP_CBC(DRV8316_HandleTypeDef *hdrv,
                                      DRV8316_OCP_CBC_t *enabled) {
    uint8_t value = 0U;

    if (enabled == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_4, DRV8316_CTRL4_OCP_CBC_MASK,
                          DRV8316_CTRL4_OCP_CBC_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *enabled = (DRV8316_OCP_CBC_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_OCP_CBC(DRV8316_HandleTypeDef *hdrv,
                                      DRV8316_OCP_CBC_t enabled) {
    if ((uint8_t)enabled > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_4,
                             DRV8316_CTRL4_OCP_CBC_MASK,
                             DRV8316_CTRL4_OCP_CBC_OFFSET, (uint8_t)enabled);
}

HAL_StatusTypeDef DRV8316_Get_Driver_State(DRV8316_HandleTypeDef *hdrv,
                                           DRV8316_DriverState_t *state) {
    uint8_t value = 0U;

    if (state == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_4, DRV8316_CTRL4_DRV_OFF_MASK,
                          DRV8316_CTRL4_DRV_OFF_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *state = (DRV8316_DriverState_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_Driver_State(DRV8316_HandleTypeDef *hdrv,
                                           DRV8316_DriverState_t state) {
    if ((uint8_t)state > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_4,
                             DRV8316_CTRL4_DRV_OFF_MASK,
                             DRV8316_CTRL4_DRV_OFF_OFFSET, (uint8_t)state);
}

/* ============================================================
 * CTRL_5
 * ============================================================ */

HAL_StatusTypeDef DRV8316_Get_Current_Sense_Gain(DRV8316_HandleTypeDef *hdrv,
                                                 DRV8316_CSA_Gain_t *gain) {
    uint8_t value = 0U;

    if (gain == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_5, DRV8316_CTRL5_CSA_GAIN_MASK,
                          DRV8316_CTRL5_CSA_GAIN_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *gain = (DRV8316_CSA_Gain_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_Current_Sense_Gain(DRV8316_HandleTypeDef *hdrv,
                                                 DRV8316_CSA_Gain_t gain) {
    if ((uint8_t)gain > 3U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_5,
                             DRV8316_CTRL5_CSA_GAIN_MASK,
                             DRV8316_CTRL5_CSA_GAIN_OFFSET, (uint8_t)gain);
}

HAL_StatusTypeDef DRV8316_Get_ASR(DRV8316_HandleTypeDef *hdrv,
                                  DRV8316_ASR_Enable_t *enabled) {
    uint8_t value = 0U;

    if (enabled == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_5, DRV8316_CTRL5_EN_ASR_MASK,
                          DRV8316_CTRL5_EN_ASR_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *enabled = (DRV8316_ASR_Enable_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_ASR(DRV8316_HandleTypeDef *hdrv,
                                  DRV8316_ASR_Enable_t enabled) {
    if ((uint8_t)enabled > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_5,
                             DRV8316_CTRL5_EN_ASR_MASK,
                             DRV8316_CTRL5_EN_ASR_OFFSET, (uint8_t)enabled);
}

HAL_StatusTypeDef DRV8316_Get_AAR(DRV8316_HandleTypeDef *hdrv,
                                  DRV8316_AAR_Enable_t *enabled) {
    uint8_t value = 0U;

    if (enabled == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_5, DRV8316_CTRL5_EN_AAR_MASK,
                          DRV8316_CTRL5_EN_AAR_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *enabled = (DRV8316_AAR_Enable_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_AAR(DRV8316_HandleTypeDef *hdrv,
                                  DRV8316_AAR_Enable_t enabled) {
    if ((uint8_t)enabled > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_5,
                             DRV8316_CTRL5_EN_AAR_MASK,
                             DRV8316_CTRL5_EN_AAR_OFFSET, (uint8_t)enabled);
}

HAL_StatusTypeDef
DRV8316_Get_ILIM_Recirculation(DRV8316_HandleTypeDef *hdrv,
                               DRV8316_ILIM_Recirculation_t *mode) {
    uint8_t value = 0U;

    if (mode == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_5,
                          DRV8316_CTRL5_ILIM_RECIR_MASK,
                          DRV8316_CTRL5_ILIM_RECIR_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *mode = (DRV8316_ILIM_Recirculation_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_ILIM_Recirculation(DRV8316_HandleTypeDef *hdrv,
                               DRV8316_ILIM_Recirculation_t mode) {
    if ((uint8_t)mode > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_5,
                             DRV8316_CTRL5_ILIM_RECIR_MASK,
                             DRV8316_CTRL5_ILIM_RECIR_OFFSET, (uint8_t)mode);
}

/* ============================================================
 * CTRL_6
 * ============================================================ */

HAL_StatusTypeDef DRV8316_Get_Buck_State(DRV8316_HandleTypeDef *hdrv,
                                         DRV8316_BuckDisable_t *state) {
    uint8_t value = 0U;

    if (state == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_DIS_MASK,
                          DRV8316_CTRL6_BUCK_DIS_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *state = (DRV8316_BuckDisable_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_Buck_State(DRV8316_HandleTypeDef *hdrv,
                                         DRV8316_BuckDisable_t state) {
    if ((uint8_t)state > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_6,
                             DRV8316_CTRL6_BUCK_DIS_MASK,
                             DRV8316_CTRL6_BUCK_DIS_OFFSET, (uint8_t)state);
}

HAL_StatusTypeDef DRV8316_Get_Buck_Voltage(DRV8316_HandleTypeDef *hdrv,
                                           DRV8316_BuckVoltage_t *voltage) {
    uint8_t value = 0U;

    if (voltage == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_SEL_MASK,
                          DRV8316_CTRL6_BUCK_SEL_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *voltage = (DRV8316_BuckVoltage_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_Buck_Voltage(DRV8316_HandleTypeDef *hdrv,
                                           DRV8316_BuckVoltage_t voltage) {
    if ((uint8_t)voltage > 3U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_6,
                             DRV8316_CTRL6_BUCK_SEL_MASK,
                             DRV8316_CTRL6_BUCK_SEL_OFFSET, (uint8_t)voltage);
}

HAL_StatusTypeDef
DRV8316_Get_Buck_Current_Limit(DRV8316_HandleTypeDef *hdrv,
                               DRV8316_BuckCurrentLimit_t *limit) {
    uint8_t value = 0U;

    if (limit == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_6, DRV8316_CTRL6_BUCK_CL_MASK,
                          DRV8316_CTRL6_BUCK_CL_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *limit = (DRV8316_BuckCurrentLimit_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_Buck_Current_Limit(DRV8316_HandleTypeDef *hdrv,
                               DRV8316_BuckCurrentLimit_t limit) {
    if ((uint8_t)limit > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_6,
                             DRV8316_CTRL6_BUCK_CL_MASK,
                             DRV8316_CTRL6_BUCK_CL_OFFSET, (uint8_t)limit);
}

HAL_StatusTypeDef
DRV8316_Get_Buck_Power_Sequencing(DRV8316_HandleTypeDef *hdrv,
                                  DRV8316_BuckPowerSequence_t *state) {
    uint8_t value = 0U;

    if (state == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_6,
                          DRV8316_CTRL6_BUCK_PS_DIS_MASK,
                          DRV8316_CTRL6_BUCK_PS_DIS_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *state = (DRV8316_BuckPowerSequence_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_Buck_Power_Sequencing(DRV8316_HandleTypeDef *hdrv,
                                  DRV8316_BuckPowerSequence_t state) {
    if ((uint8_t)state > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_6,
                             DRV8316_CTRL6_BUCK_PS_DIS_MASK,
                             DRV8316_CTRL6_BUCK_PS_DIS_OFFSET, (uint8_t)state);
}

/* ============================================================
 * CTRL_10
 * ============================================================ */

HAL_StatusTypeDef DRV8316_Get_Delay_Target(DRV8316_HandleTypeDef *hdrv,
                                           DRV8316_DelayTarget_t *target) {
    uint8_t value = 0U;

    if (target == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_10,
                          DRV8316_CTRL10_DLY_TARGET_MASK,
                          DRV8316_CTRL10_DLY_TARGET_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *target = (DRV8316_DelayTarget_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_Delay_Target(DRV8316_HandleTypeDef *hdrv,
                                           DRV8316_DelayTarget_t target) {
    if ((uint8_t)target > 0x0FU) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_10,
                             DRV8316_CTRL10_DLY_TARGET_MASK,
                             DRV8316_CTRL10_DLY_TARGET_OFFSET, (uint8_t)target);
}

HAL_StatusTypeDef
DRV8316_Get_Delay_Compensation(DRV8316_HandleTypeDef *hdrv,
                               DRV8316_DelayCompensation_t *enabled) {
    uint8_t value = 0U;

    if (enabled == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_10,
                          DRV8316_CTRL10_DLYCMP_EN_MASK,
                          DRV8316_CTRL10_DLYCMP_EN_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *enabled = (DRV8316_DelayCompensation_t)value;
    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_Delay_Compensation(DRV8316_HandleTypeDef *hdrv,
                               DRV8316_DelayCompensation_t enabled) {
    if ((uint8_t)enabled > 1U) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_10,
                             DRV8316_CTRL10_DLYCMP_EN_MASK,
                             DRV8316_CTRL10_DLYCMP_EN_OFFSET, (uint8_t)enabled);
}
HAL_StatusTypeDef DRV8316_Get_SDO_Mode(DRV8316_HandleTypeDef *hdrv,
                                       DRV8316_SDO_Mode_t *mode) {
    uint8_t value = 0U;

    if (mode == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_2, DRV8316_CTRL2_SDO_MODE_MASK,
                          DRV8316_CTRL2_SDO_MODE_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *mode = (DRV8316_SDO_Mode_t)value;

    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_SDO_Mode(DRV8316_HandleTypeDef *hdrv,
                                       DRV8316_SDO_Mode_t mode) {
    if ((uint8_t)mode > (uint8_t)DRV8316_SDO_MODE_PUSH_PULL) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_2,
                             DRV8316_CTRL2_SDO_MODE_MASK,
                             DRV8316_CTRL2_SDO_MODE_OFFSET, (uint8_t)mode);
}

HAL_StatusTypeDef DRV8316_Get_Slew(DRV8316_HandleTypeDef *hdrv,
                                   DRV8316_SlewRate_t *slew) {
    uint8_t value = 0U;

    if (slew == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_2, DRV8316_CTRL2_SLEW_MASK,
                          DRV8316_CTRL2_SLEW_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *slew = (DRV8316_SlewRate_t)value;

    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_Slew(DRV8316_HandleTypeDef *hdrv,
                                   DRV8316_SlewRate_t slew) {
    if ((uint8_t)slew > (uint8_t)DRV8316_SLEW_RATE_200V_US) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_2, DRV8316_CTRL2_SLEW_MASK,
                             DRV8316_CTRL2_SLEW_OFFSET, (uint8_t)slew);
}

HAL_StatusTypeDef DRV8316_Get_PWM_Mode(DRV8316_HandleTypeDef *hdrv,
                                       DRV8316_PWM_Mode_t *mode) {
    uint8_t value = 0U;

    if (mode == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_2, DRV8316_CTRL2_PWM_MODE_MASK,
                          DRV8316_CTRL2_PWM_MODE_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *mode = (DRV8316_PWM_Mode_t)value;

    return HAL_OK;
}

HAL_StatusTypeDef DRV8316_Set_PWM_Mode(DRV8316_HandleTypeDef *hdrv,
                                       DRV8316_PWM_Mode_t mode) {
    if ((uint8_t)mode > (uint8_t)DRV8316_PWM_MODE_3X_CL) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(hdrv, DRV8316_REG_CTRL_2,
                             DRV8316_CTRL2_PWM_MODE_MASK,
                             DRV8316_CTRL2_PWM_MODE_OFFSET, (uint8_t)mode);
}

HAL_StatusTypeDef
DRV8316_Get_Register_Lock(DRV8316_HandleTypeDef *hdrv,
                          DRV8316_RegisterLock_t *lock_status) {
    uint8_t value = 0U;

    if (lock_status == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Get_Field(hdrv, DRV8316_REG_CTRL_1, DRV8316_CTRL1_REG_LOCK_MASK,
                          DRV8316_CTRL1_REG_LOCK_OFFSET, &value) != HAL_OK) {
        return HAL_ERROR;
    }

    *lock_status = (DRV8316_RegisterLock_t)value;

    return HAL_OK;
}

HAL_StatusTypeDef
DRV8316_Set_Register_Lock(DRV8316_HandleTypeDef *hdrv,
                          DRV8316_RegisterLock_t lock_status) {
    if ((lock_status != DRV8316_REG_LOCK_UNLOCK) &&
        (lock_status != DRV8316_REG_LOCK_LOCK)) {
        return HAL_ERROR;
    }

    return DRV8316_Set_Field(
        hdrv, DRV8316_REG_CTRL_1, DRV8316_CTRL1_REG_LOCK_MASK,
        DRV8316_CTRL1_REG_LOCK_OFFSET, (uint8_t)lock_status);
}

HAL_StatusTypeDef DRV8316_Lock_Registers(DRV8316_HandleTypeDef *hdrv) {
    return DRV8316_Set_Register_Lock(hdrv, DRV8316_REG_LOCK_LOCK);
}

HAL_StatusTypeDef DRV8316_Unlock_Registers(DRV8316_HandleTypeDef *hdrv) {
    return DRV8316_Set_Register_Lock(hdrv, DRV8316_REG_LOCK_UNLOCK);
}

HAL_StatusTypeDef DRV8316_Clear_Faults(DRV8316_HandleTypeDef *hdrv) {
    uint8_t ctrl_2 = 0U;

    if (hdrv == NULL) {
        return HAL_ERROR;
    }

    if (DRV8316_Read_Register(hdrv, DRV8316_REG_CTRL_2, &ctrl_2) != HAL_OK) {
        return HAL_ERROR;
    }

    /*
     * CLR_FLT is write-one-to-clear.
     * Preserve the other CTRL_2 fields and set bit 0.
     */
    ctrl_2 |= DRV8316_CTRL2_CLR_FLT_MASK;

    return DRV8316_Write_Register(hdrv, DRV8316_REG_CTRL_2, ctrl_2, NULL);
}
