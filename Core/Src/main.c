/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#define FOTA_TARGET_ADDRESS 0x08010000
uint32_t Flash_Read(uint32_t address, uint8_t *data, uint32_t length)
{
    while(length--)
    {
        *(data++)=*((uint8_t*)address++);
    }
		return 0;
}

uint32_t Flash_Erase(uint32_t address)
{
    FLASH_EraseInitTypeDef FlashSet;
    FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
    FlashSet.PageAddress = address;
    FlashSet.NbPages = 1;
    /*设置PageError，调用擦除函数*/
    uint32_t PageError = 0;
    HAL_FLASH_Unlock();
    HAL_FLASHEx_Erase(&FlashSet, &PageError);
    HAL_FLASH_Lock();
		return 0;
}

uint32_t Flash_Write(uint32_t address, uint8_t *data, uint32_t length)
{
    uint8_t buff[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t len = length / 4;
    // 解锁Flash
    HAL_FLASH_Unlock();
 
    // 写入数据到指定地址
    while(len--)
    {
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, *((uint32_t *)data));
      address += 4;
      data += 4;
    }

    len = length % 4;

    for(int i = 0; i < len; i++)
      buff[i] = data[i];

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, *((uint32_t *)buff));

    // 重新锁定Flash
    HAL_FLASH_Lock();
		return 0;
}


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#include "janpatch.h"

static janpatch_file_t source = {
  .file_address = FOTA_TARGET_ADDRESS,
  // .file_size = 47352,
};

static janpatch_file_t patch = {
  .file_address = 0x08030000,
  // .file_size = 8580,
};

static janpatch_file_t target = {
  .file_address = FOTA_TARGET_ADDRESS,
  .file_size = 64 * 1024,
};

static struct janpatch_mcu_config janpatch_mcu_config = {
  .flash_erase = Flash_Erase,
  .flash_read = Flash_Read,
  .flash_write = Flash_Write,
};


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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  uint8_t ret = 0;
  ret = janpatch_mcu_cinfig_fota(&janpatch_mcu_config, &source, &patch, &target);
  // if(ret == 0){
  //   uint8_t val = 0;
  //   for(int i = 0; i < target.file_seek; i++) {
  //     Flash_Read(FOTA_TARGET_ADDRESS + i, &val, 1);
  //       if(i % 2048 == 0) printf("\r\n");
  //     if(i % 16 == 0) printf("\r\n%08X: ", i); 
  //     printf("%02X ", val);
  //   }
  // }
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
