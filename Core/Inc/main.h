/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LIM0_L_Pin GPIO_PIN_13
#define LIM0_L_GPIO_Port GPIOC
#define LIM0_R_Pin GPIO_PIN_14
#define LIM0_R_GPIO_Port GPIOC
#define LIM1_L_Pin GPIO_PIN_15
#define LIM1_L_GPIO_Port GPIOC
#define LIM1_R_Pin GPIO_PIN_0
#define LIM1_R_GPIO_Port GPIOF
#define LIM2_L_Pin GPIO_PIN_1
#define LIM2_L_GPIO_Port GPIOF
#define M0_PWM_U_Pin GPIO_PIN_0
#define M0_PWM_U_GPIO_Port GPIOC
#define M0_PWM_V_Pin GPIO_PIN_1
#define M0_PWM_V_GPIO_Port GPIOC
#define M0_PWM_W_Pin GPIO_PIN_2
#define M0_PWM_W_GPIO_Port GPIOC
#define DRV_BKIN_Pin GPIO_PIN_3
#define DRV_BKIN_GPIO_Port GPIOC
#define M1_PWM_U_Pin GPIO_PIN_0
#define M1_PWM_U_GPIO_Port GPIOA
#define M1_PWM_V_Pin GPIO_PIN_1
#define M1_PWM_V_GPIO_Port GPIOA
#define M1_PWM_W_Pin GPIO_PIN_2
#define M1_PWM_W_GPIO_Port GPIOA
#define M0_IU_ADC_Pin GPIO_PIN_3
#define M0_IU_ADC_GPIO_Port GPIOA
#define M0_IV_ADC_Pin GPIO_PIN_4
#define M0_IV_ADC_GPIO_Port GPIOA
#define M1_IV_ADC_Pin GPIO_PIN_4
#define M1_IV_ADC_GPIO_Port GPIOC
#define M2_IV_ADC_Pin GPIO_PIN_5
#define M2_IV_ADC_GPIO_Port GPIOC
#define M1_IU_ADC_Pin GPIO_PIN_0
#define M1_IU_ADC_GPIO_Port GPIOB
#define M2_IU_ADC_Pin GPIO_PIN_1
#define M2_IU_ADC_GPIO_Port GPIOB
#define M0_nSCS_Pin GPIO_PIN_2
#define M0_nSCS_GPIO_Port GPIOB
#define M1_nSCS_Pin GPIO_PIN_10
#define M1_nSCS_GPIO_Port GPIOB
#define M0_IW_ADC_Pin GPIO_PIN_11
#define M0_IW_ADC_GPIO_Port GPIOB
#define M1_IW_ADC_Pin GPIO_PIN_12
#define M1_IW_ADC_GPIO_Port GPIOB
#define M2_nSCS_Pin GPIO_PIN_13
#define M2_nSCS_GPIO_Port GPIOB
#define M2_IW_ADC_Pin GPIO_PIN_14
#define M2_IW_ADC_GPIO_Port GPIOB
#define M2_PWM_U_Pin GPIO_PIN_6
#define M2_PWM_U_GPIO_Port GPIOC
#define M2_PWM_V_Pin GPIO_PIN_7
#define M2_PWM_V_GPIO_Port GPIOC
#define M2_PWM_W_Pin GPIO_PIN_8
#define M2_PWM_W_GPIO_Port GPIOC
#define ENC2_CS_Pin GPIO_PIN_8
#define ENC2_CS_GPIO_Port GPIOA
#define ENC1_CS_Pin GPIO_PIN_9
#define ENC1_CS_GPIO_Port GPIOA
#define ENC0_CS_Pin GPIO_PIN_10
#define ENC0_CS_GPIO_Port GPIOA
#define CAN1_RX_Pin GPIO_PIN_11
#define CAN1_RX_GPIO_Port GPIOA
#define CAN1_TX_Pin GPIO_PIN_12
#define CAN1_TX_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define ERR_LED_Pin GPIO_PIN_15
#define ERR_LED_GPIO_Port GPIOA
#define USR_LED_Pin GPIO_PIN_10
#define USR_LED_GPIO_Port GPIOC
#define CONTACT_Pin GPIO_PIN_11
#define CONTACT_GPIO_Port GPIOC
#define M2_nSLEEP_Pin GPIO_PIN_12
#define M2_nSLEEP_GPIO_Port GPIOC
#define M2_nFAULT_Pin GPIO_PIN_2
#define M2_nFAULT_GPIO_Port GPIOD
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define M1_nSLEEP_Pin GPIO_PIN_4
#define M1_nSLEEP_GPIO_Port GPIOB
#define M1_nFAULT_Pin GPIO_PIN_5
#define M1_nFAULT_GPIO_Port GPIOB
#define M0_nSLEEP_Pin GPIO_PIN_6
#define M0_nSLEEP_GPIO_Port GPIOB
#define M0_nFAULT_Pin GPIO_PIN_7
#define M0_nFAULT_GPIO_Port GPIOB
#define LIM2_R_Pin GPIO_PIN_9
#define LIM2_R_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
