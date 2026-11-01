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
#include <stdio.h>
#include <string.h>
#include "flash_layout.h"
#include <application_header.h>
#include "bootloader_jump.h"
#include "bl_ota.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
void JumpToApplication(void);
int bootloader_is_app_valid(void);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint32_t mem_base = OTA_START_ADDR;  // used with external or dual flash
static uint32_t mem_cursor = 0;
static uint32_t ota_image_total_size = 16;  // expected OTA Header size

void ota_mem_set_total_size(uint32_t size)
{
    ota_image_total_size = size;
}

int ota_read_mem(uint8_t *buf, uint32_t len)
{
    if (mem_cursor + len > ota_image_total_size)
        return -1;

    uint8_t *flash_ptr = (uint8_t *)(mem_base + mem_cursor);

    memcpy(buf, flash_ptr, len);

    mem_cursor += len;
    return 0;
}
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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
   HAL_Delay(100);
   char data[] = "Inside Bootloader!!\r\n";
   HAL_UART_Transmit(&huart1, (uint8_t *)data, strlen(data), 1000);



   if (check_ota_request() == 0)
   {
 	  HAL_UART_Transmit(&huart1, (uint8_t *)"Performing OTA...\n", 18, 100);

 	  ota_stream_t stream =
 	  {
 	      .read = ota_read_mem,
 	      .set_total_size = ota_mem_set_total_size
 	  };

 	  bl_ota_ctx_t ctx;
 	 if (bl_ota_run(&ctx, &stream) != 0)
 	 {
 		 HAL_UART_Transmit(&huart1, (uint8_t *)"OTA Failed...\n", 14, 100);
 		  while (1)
 		  {
 			  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
 			  HAL_Delay(250);
 		  }
 	 }
 	 else {
 		 HAL_UART_Transmit(&huart1, (uint8_t *)"OTA Flashed Successfully, Jumping to app...\n", 44, 100);
 	 }

   }

   int err = bootloader_is_app_valid();

  /* USER CODE END 2 */
   if (err != 0)
    {
  	  HAL_UART_Transmit(&huart1, (uint8_t *)"Failed to Jump!!\r\n", 18, 100);
  	  switch(err){
	  case 1:
		  HAL_UART_Transmit(&huart1, (uint8_t *)"MAGIC ERROR!!\r\n", 15, 100);
		  break;

	  case 2:
		  HAL_UART_Transmit(&huart1, (uint8_t *)"RESET ERROR!!\r\n", 15, 100);
		  break;

	  case 3:
		  HAL_UART_Transmit(&huart1, (uint8_t *)"SIZE ERROR!!\r\n", 14, 100);
		  break;

	  case 4:
		  HAL_UART_Transmit(&huart1, (uint8_t *)"CRC ERROR!!\r\n", 13, 100);
		  break;

	  default:
		  HAL_UART_Transmit(&huart1, (uint8_t *)"ERROR!!\r\n", 9, 100);
		  break;

  	  }
  	  while (1)
  	  {
  		  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  		  HAL_Delay(3000);
  	  }
    }

   JumpToApplication();
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  for( int i = 0; i < 21; i++ )
	 	  {
	 		  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	 		  HAL_Delay(500);


	 	  }
	          HAL_Delay(1000);


    /* USER CODE BEGIN 3 */
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
