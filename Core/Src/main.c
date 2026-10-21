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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
static const uint16_t led_pwm_pulse_counts[] = {200U, 500U, 800U};
static uint8_t led_pwm_duty_index = 0U;
static uint32_t runtime_seconds = 0U;
static volatile uint32_t scheduler_tick_count = 0U;

typedef void (*SchedulerTaskFunction)(void);

typedef struct
{
  uint16_t period_ms;
  uint16_t elapsed_ms;
  volatile uint8_t pending;
  SchedulerTaskFunction function;
} SchedulerTask;

static void Task_ToggleLed1(void);
static void Task_ToggleLed2(void);
static void Task_PrintRuntime(void);
static void Task_UpdateLedPwm(void);

static SchedulerTask scheduler_tasks[] =
{
  {100U, 0U, 0U, Task_ToggleLed1},
  {500U, 0U, 0U, Task_ToggleLed2},
  {1000U, 0U, 0U, Task_PrintRuntime},
  {1000U, 0U, 0U, Task_UpdateLedPwm}
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Scheduler_RunReadyTasks(void);

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
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, led_pwm_pulse_counts[0]);
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Scheduler_RunReadyTasks();
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
static void Scheduler_RunReadyTasks(void)
{
  uint32_t task_index;

  for (task_index = 0U;
       task_index < (sizeof(scheduler_tasks) / sizeof(scheduler_tasks[0]));
       task_index++)
  {
    uint8_t task_pending;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    task_pending = scheduler_tasks[task_index].pending;
    scheduler_tasks[task_index].pending = 0U;
    if (primask == 0U)
    {
      __enable_irq();
    }

    if (task_pending != 0U)
    {
      scheduler_tasks[task_index].function();
    }
  }
}

static void Task_ToggleLed1(void)
{
  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
}

static void Task_ToggleLed2(void)
{
  HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
}

static void Task_PrintRuntime(void)
{
  char message[32];
  int message_length;

  runtime_seconds++;
  message_length = snprintf(message, sizeof(message),
                            "Runtime: %lu s\r\n",
                            (unsigned long)runtime_seconds);
  if ((message_length < 0) || ((size_t)message_length >= sizeof(message)))
  {
    Error_Handler();
  }
  if (HAL_UART_Transmit(&huart1, (uint8_t *)message,
                        (uint16_t)message_length, HAL_MAX_DELAY) != HAL_OK)
  {
    Error_Handler();
  }
}

static void Task_UpdateLedPwm(void)
{
  led_pwm_duty_index = (uint8_t)((led_pwm_duty_index + 1U) % 3U);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,
                        led_pwm_pulse_counts[led_pwm_duty_index]);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    uint32_t task_index;

    scheduler_tick_count++;
    for (task_index = 0U;
         task_index < (sizeof(scheduler_tasks) / sizeof(scheduler_tasks[0]));
         task_index++)
    {
      scheduler_tasks[task_index].elapsed_ms++;
      if (scheduler_tasks[task_index].elapsed_ms >=
          scheduler_tasks[task_index].period_ms)
      {
        scheduler_tasks[task_index].elapsed_ms = 0U;
        scheduler_tasks[task_index].pending = 1U;
      }
    }
  }
}

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
