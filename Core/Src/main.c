/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "cordic.h"
#include "fdcan.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "as5048a_adapter.h"
#include "current_sense.h"
#include "drv8316_adapter.h"
#include "foc_adapter.h"
#include "swo.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  SWO_Init();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_CORDIC_Init();
  MX_FDCAN1_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM8_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
    Error_Handler();
  };

  if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK) {
    Error_Handler();
  };

  if (drv8316_init_instance(DRV8316_INSTANCE_1) != HAL_OK) {
    Error_Handler();
  }

  if (drv8316_set_pwm_mode(DRV8316_INSTANCE_1, DRV8316_PWM_MODE_3X) != HAL_OK) {
    Error_Handler();
  };

  if (as5048a_init() != HAL_OK) {
    Error_Handler();
  }

  if (as5048a_begin_continuous_angle_read(AS5048A_INSTANCE_1) != HAL_OK) {
    Error_Handler();
  }

  /*
   * Wake DRV8316 before current calibration.
   * CSA offsets depend on the driver being active.
   */

  /*
   * Allow CSA bias/reference to settle.
   */
  HAL_Delay(10);

  uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
  uint32_t neutral = arr / 2U;

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, neutral);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, neutral);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, neutral);

  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  };

  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) {
    Error_Handler();
  };

  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) != HAL_OK) {
    Error_Handler();
  };

  if (HAL_ADCEx_InjectedStart(&hadc2)) {
    Error_Handler();
  };

  if (HAL_ADCEx_InjectedStart_IT(&hadc1)) {
    Error_Handler();
  };

  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK) {
    Error_Handler();
  }

  CurrentSense_Init();

  if (CurrentSense_ProcessAdcISR() != HAL_OK) {
    Error_Handler();
  };

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  HAL_GPIO_WritePin(USR_LED_GPIO_Port, USR_LED_Pin, 1);
  HAL_Delay(100);
  HAL_GPIO_WritePin(USR_LED_GPIO_Port, USR_LED_Pin, 0);
  HAL_Delay(100);
  HAL_GPIO_WritePin(USR_LED_GPIO_Port, USR_LED_Pin, 1);
  HAL_Delay(100);
  HAL_GPIO_WritePin(USR_LED_GPIO_Port, USR_LED_Pin, 0);

  axis_init();

  current_loop_on = false;
  axis_calibrated = true;
  theta_target = 0;
  DRV8316_Diagnostics_t diagnostics;

  static uint8_t previous_drv_state = 0;
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    if (!CurrentSense_IsReady()) {
      CurrentSense_ProcessAdcISR();
      continue;
    }

    if (!axis_calibrated)
      axis_calibrate_electrical_offset();
    if (drv8316_on != previous_drv_state) {
      if (drv8316_on)
        drv8316_wake(DRV8316_INSTANCE_1);
      else
        drv8316_sleep(DRV8316_INSTANCE_1);
      previous_drv_state = drv8316_on;
    }

    if (motor_headless_rotating) {
      if (axis_full_rotation(1, 5) != HAL_OK) {
        Error_Handler();
      };
      motor_headless_rotating = false;
    }
    axis_test_angle(3.20f);

    HAL_Delay(10U);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state
   */
  HAL_GPIO_WritePin(ERR_LED_GPIO_Port, ERR_LED_Pin, GPIO_PIN_SET);
  printf("Error Detected\r\n");
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
     file, line) */
  printf("Test\r\n");
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
