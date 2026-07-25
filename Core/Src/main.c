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
#include "gpio.h"
#include "spi.h"
#include "tim.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "drv8316/drv8316.h"
#include "drv8316/registers.h"

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

#define PI 3.14159265358979323846f
#define TWO_PI 6.28318530718f
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void SWO_Init() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // __HAL_RCC_DBGMCU_CLK_ENABLE();
    DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
    DBGMCU->CR &= ~DBGMCU_CR_TRACE_MODE;
    ITM->LAR = 0xC5ACCE55;
    TPI->SPPR = 0x00000002; // NRZ
    TPI->ACPR = 7;          // 84MHz / 42 = 2MHz SWO
    ITM->TCR |= ITM_TCR_ITMENA_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk;
    ITM->TER = 1UL;

    setvbuf(stdout, NULL, _IONBF, 0); // disable printf buffering
}

/**
 * @brief Converts a 16-bit word into a formatted binary string.
 * @param buf Must be at least 19 bytes long (16 bits + 1 space + 1 prefix
 * space/char + 1 null terminator)
 * @param val The 16-bit value to convert
 */
void to_binary_str(char *buf, uint16_t val) {
    int buf_idx = 0;

    // Optional: Add a nice visual prefix
    buf[buf_idx++] = '0';
    buf[buf_idx++] = 'b';

    // Loop through all 16 bits starting from the Most Significant Bit (Bit 15)
    for (int i = 15; i >= 0; i--) {
        // Add a clean space between the high byte and low byte for readability
        if (i == 7) {
            buf[buf_idx++] = ' ';
        }

        // Check if the specific bit is set, and write the corresponding
        // character
        buf[buf_idx++] = (val & (1 << i)) ? '1' : '0';
    }

    // Always null-terminate the string!
    buf[buf_idx] = '\0';
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU
     * Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
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
    /* USER CODE BEGIN 2 */

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    __HAL_TIM_MOE_ENABLE(&htim1);

    // 4. Force Fixed Test Duty Cycles directly in registers (ARR = 1000)
    TIM1->CCR1 = 600; // ~60% duty cycle (~1.98V DC on CH1)
    TIM1->CCR2 = 400; // ~40% duty cycle (~1.32V DC on CH2)
    TIM1->CCR3 = 400; // ~40% duty cycle (~1.32V DC on CH3)
    DRV8316_HandleTypeDef hdrv;
    HAL_GPIO_WritePin(M0_nSCS_GPIO_Port, M0_nSCS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(M1_nSCS_GPIO_Port, M1_nSCS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(M2_nSCS_GPIO_Port, M2_nSCS_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(M0_nSLEEP_GPIO_Port, M0_nSLEEP_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(M1_nSLEEP_GPIO_Port, M1_nSLEEP_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(M2_nSLEEP_GPIO_Port, M2_nSLEEP_Pin, GPIO_PIN_RESET);

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

    if (DRV8316_Init(&hdrv, &hspi1, &htim1, M0_nSCS_GPIO_Port, M0_nSCS_Pin,
                     M0_nFAULT_GPIO_Port, M0_nFAULT_Pin, M0_nSLEEP_GPIO_Port,
                     M0_nSLEEP_Pin, M0_PWM_U_GPIO_Port, M0_PWM_U_Pin,
                     M0_PWM_V_GPIO_Port, M0_PWM_V_Pin, M0_PWM_W_GPIO_Port,
                     M0_PWM_W_Pin) != HAL_OK)
        Error_Handler();

    if (DRV8316_Set_PWM_Mode(&hdrv, DRV8316_PWM_MODE_3X) != HAL_OK) {
        Error_Handler();
    }

    float theta_el = 0.0f;
    float target_speed_hz = 5.0f;
    float voltage_amplitude = 0.15f;
    float dt = 0.001f;

    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        // 5. Update timer CCR

        // Update timer CCR registers

        theta_el += TWO_PI * target_speed_hz * dt;
        if (theta_el >= TWO_PI)
            theta_el -= TWO_PI;

        // Normalized Phase calc
        float v_a = sinf(theta_el);
        float v_b = sinf(theta_el - TWO_PI / 3.0f);
        float v_c = sinf(theta_el + TWO_PI / 3.0f);

        // 3. Scale by Vq (target amplitude)

        // 4. Map [-1.0, +1.0] -> [0, ARR]
        uint16_t half_arr = TIM1->ARR / 2.0f;
        uint16_t ccr_a = (1.0f + voltage_amplitude * v_a) * half_arr;
        uint16_t ccr_b = (v_b + 1) * half_arr;
        uint16_t ccr_c = (v_c + 1) * half_arr;
        DRV8316_Set_PWM(&hdrv, ccr_a, ccr_b, ccr_c);

        HAL_Delay(1);
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
// In main.c, add this ITM send function
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        ITM_SendChar(*ptr++);
    }
    return len;
}

#define DBG_BUF_SIZE 128

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
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
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
