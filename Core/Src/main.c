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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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
typedef enum
{
  LED_MODE_BREATH = 0U,
  LED_MODE_BLINK,
  LED_MODE_OFF
} LedMode;

#define UART_LINE_SIZE 32U
#define UART_QUEUE_SIZE 4U
#define PWM_MAX_DUTY 1000U

static volatile uint8_t led_mode = LED_MODE_BREATH;
static volatile uint8_t breath_speed = 1U;
static uint8_t uart_rx_byte;
static char uart_rx_line[UART_LINE_SIZE];
static uint8_t uart_rx_length;
static uint8_t uart_rx_discarding;
static char uart_rx_queue[UART_QUEUE_SIZE][UART_LINE_SIZE];
static volatile uint8_t uart_rx_queue_head;
static volatile uint8_t uart_rx_queue_tail;
static volatile uint32_t uart_rejected_lines;
static volatile uint32_t uart_error_count;
static volatile uint8_t uart_rx_restart_failed;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void UART_ProcessCommands(void);
static void UART_ProcessCommand(const char *command);
static void UART_Printf(const char *format, ...);
static void UART_ReportReceiveErrors(void);
static uint8_t UART_GetLine(char *line);
static const char *UART_ModeName(uint8_t mode);

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
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) != HAL_OK)
  {
    Error_Handler();
  }
  UART_Printf("Breathing LED ready (PA6, 1 kHz). Commands: SPD=1..10, "
              "MODE=BREATH|BLINK|OFF, ?\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    UART_ProcessCommands();
    UART_ReportReceiveErrors();
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
static void UART_Printf(const char *format, ...)
{
  char message[112];
  int message_length;
  va_list arguments;

  va_start(arguments, format);
  message_length = vsnprintf(message, sizeof(message), format, arguments);
  va_end(arguments);
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

static const char *UART_ModeName(uint8_t mode)
{
  switch (mode)
  {
    case LED_MODE_BREATH:
      return "BREATH";
    case LED_MODE_BLINK:
      return "BLINK";
    case LED_MODE_OFF:
      return "OFF";
    default:
      Error_Handler();
      return "";
  }
}

static void UART_ProcessCommand(const char *command)
{
  uint8_t command_length = (uint8_t)strlen(command);

  if (strcmp(command, "?") == 0)
  {
    UART_Printf("MODE=%s SPD=%u\r\n",
                UART_ModeName(led_mode), (unsigned int)breath_speed);
  }
  else if ((command_length >= 5U) && (strncmp(command, "SPD=", 4U) == 0))
  {
    uint8_t new_speed;

    if ((command_length == 5U) &&
        (command[4] >= '1') && (command[4] <= '9'))
    {
      new_speed = (uint8_t)(command[4] - '0');
    }
    else if ((command_length == 6U) &&
             (command[4] == '1') && (command[5] == '0'))
    {
      new_speed = 10U;
    }
    else
    {
      UART_Printf("ERR: invalid command\r\n");
      return;
    }

    breath_speed = new_speed;
    UART_Printf("OK SPD=%u\r\n", (unsigned int)breath_speed);
  }
  else if (strcmp(command, "MODE=BREATH") == 0)
  {
    led_mode = LED_MODE_BREATH;
    UART_Printf("OK MODE=BREATH\r\n");
  }
  else if (strcmp(command, "MODE=BLINK") == 0)
  {
    led_mode = LED_MODE_BLINK;
    UART_Printf("OK MODE=BLINK\r\n");
  }
  else if (strcmp(command, "MODE=OFF") == 0)
  {
    led_mode = LED_MODE_OFF;
    UART_Printf("OK MODE=OFF\r\n");
  }
  else
  {
    UART_Printf("ERR: invalid command\r\n");
  }
}

static uint8_t UART_GetLine(char *line)
{
  uint8_t has_line = 0U;
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  if (uart_rx_queue_tail != uart_rx_queue_head)
  {
    memcpy(line, uart_rx_queue[uart_rx_queue_tail], UART_LINE_SIZE);
    uart_rx_queue_tail =
      (uint8_t)((uart_rx_queue_tail + 1U) % UART_QUEUE_SIZE);
    has_line = 1U;
  }
  if (primask == 0U)
  {
    __enable_irq();
  }

  return has_line;
}

static void UART_ProcessCommands(void)
{
  char command[UART_LINE_SIZE];

  while (UART_GetLine(command) != 0U)
  {
    UART_ProcessCommand(command);
  }
}

static void UART_ReportReceiveErrors(void)
{
  uint32_t rejected;
  uint32_t uart_errors;
  uint8_t restart_failed;
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  rejected = uart_rejected_lines;
  uart_rejected_lines = 0U;
  uart_errors = uart_error_count;
  uart_error_count = 0U;
  restart_failed = uart_rx_restart_failed;
  uart_rx_restart_failed = 0U;
  if (primask == 0U)
  {
    __enable_irq();
  }

  if (rejected != 0U)
  {
    UART_Printf("ERR: %lu command line(s) rejected\r\n",
                (unsigned long)rejected);
  }
  if (uart_errors != 0U)
  {
    UART_Printf("ERR: %lu UART receive error(s)\r\n",
                (unsigned long)uart_errors);
  }
  if (restart_failed != 0U)
  {
    UART_Printf("ERR: UART receive restart failed\r\n");
    if (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) != HAL_OK)
    {
      Error_Handler();
    }
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    static uint8_t update_divider;
    static uint8_t active_mode = 0xFFU;
    static uint8_t blink_ticks;
    static uint8_t blink_on;
    static uint8_t breath_increasing = 1U;
    static uint16_t brightness;

    update_divider++;
    if (update_divider >= 10U)
    {
      uint8_t mode = led_mode;
      uint16_t step = (uint16_t)breath_speed * 5U;

      update_divider = 0U;
      if (mode != active_mode)
      {
        active_mode = mode;
        brightness = 0U;
        breath_increasing = 1U;
        blink_ticks = 0U;
        blink_on = 0U;
      }

      switch (mode)
      {
        case LED_MODE_BREATH:
          if (breath_increasing != 0U)
          {
            if ((uint16_t)(brightness + step) >= PWM_MAX_DUTY)
            {
              brightness = PWM_MAX_DUTY;
              breath_increasing = 0U;
            }
            else
            {
              brightness = (uint16_t)(brightness + step);
            }
          }
          else if (brightness <= step)
          {
            brightness = 0U;
            breath_increasing = 1U;
          }
          else
          {
            brightness = (uint16_t)(brightness - step);
          }
          __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, brightness);
          break;

        case LED_MODE_BLINK:
          blink_ticks++;
          if (blink_ticks >= 50U)
          {
            blink_ticks = 0U;
            blink_on = (uint8_t)!blink_on;
          }
          __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,
                                (blink_on != 0U) ? PWM_MAX_DUTY : 0U);
          break;

        case LED_MODE_OFF:
          __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
          break;

        default:
          __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
          break;
      }
    }
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uint8_t received = uart_rx_byte;

    if ((received == '\r') || (received == '\n'))
    {
      if (uart_rx_discarding != 0U)
      {
        uart_rejected_lines++;
      }
      else if (uart_rx_length != 0U)
      {
        uint8_t next_head =
          (uint8_t)((uart_rx_queue_head + 1U) % UART_QUEUE_SIZE);

        if (next_head == uart_rx_queue_tail)
        {
          uart_rejected_lines++;
        }
        else
        {
          uart_rx_line[uart_rx_length] = '\0';
          memcpy(uart_rx_queue[uart_rx_queue_head], uart_rx_line,
                 (size_t)uart_rx_length + 1U);
          uart_rx_queue_head = next_head;
        }
      }
      uart_rx_length = 0U;
      uart_rx_discarding = 0U;
    }
    else if (uart_rx_discarding == 0U)
    {
      if (uart_rx_length < (UART_LINE_SIZE - 1U))
      {
        uart_rx_line[uart_rx_length] = (char)received;
        uart_rx_length++;
      }
      else
      {
        uart_rx_discarding = 1U;
      }
    }

    if (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) != HAL_OK)
    {
      uart_rx_restart_failed = 1U;
    }
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart_error_count++;
    if (HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1U) != HAL_OK)
    {
      uart_rx_restart_failed = 1U;
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
