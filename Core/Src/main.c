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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


uint32_t taska_stack[STACK_SIZE];
uint32_t taskb_stack[STACK_SIZE];

/* 任务控制块 */
struct tcb taska_tcb, taskb_tcb;
struct tcb *current_tcb;

/*
 * task_create — 初始化任务栈（PendSV 异常帧布局）
 *
 * 栈从高地址到低地址：
 *   xPSR, PC, LR, R12, R3, R2, R1, R0  ← 硬件自动压栈 (8 words)
 *   R11, R10, R9, R8, R7, R6, R5, R4    ← PendSV 手动压栈 (8 words)
 */
void task_create(struct tcb *tcb, uint32_t *stack, int stack_size, void (*task_func)(void))
{
    uint32_t *sp = &stack[stack_size - 1];

    /* 硬件异常返回帧 —— bx lr 时硬件自动弹出 */
    *(--sp) = 0x01000000;           /* xPSR: 必须置 Thumb 位 */
    *(--sp) = (uint32_t)task_func;  /* PC:  首次运行入口 */
    *(--sp) = 0x00000000;           /* LR */
    *(--sp) = 0x00000000;           /* R12 */
    *(--sp) = 0x00000000;           /* R3  */
    *(--sp) = 0x00000000;           /* R2  */
    *(--sp) = 0x00000000;           /* R1  */
    *(--sp) = 0x00000000;           /* R0  */

    /* R4-R11 —— PendSV 手动弹出 */
    for (int i = 11; i >= 4; i--) {
        *(--sp) = i;
    }

    tcb->sp = sp;
}

/*
 * scheduler — 轮询调度（实验3简化版：A→B→A→B）
 */
void scheduler(void)
{
    current_tcb = (current_tcb == &taska_tcb) ? &taskb_tcb : &taska_tcb;
}

/*
 * start_first_task — 通过 SVC 进入 Handler 模式，跳转到第一个任务
 * 不能在 Thread 模式下直接加载任务栈并 bx lr
 */
__attribute__((naked)) void start_first_task(void)
{
    __asm volatile (
        "svc 0    \n"  /* 触发 SVC 异常，进入 Handler 模式 */
        "bx lr    \n"
    );
}

/* USER CODE END 0 */

/* taska: 快闪 ~200ms 周期 — 不再主动让出 CPU，由 SysTick 抢占 */
void taska(void)
{
	while(1)
	{
		HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
		for (volatile int i = 0; i < 500000; i++);
		HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
		for (volatile int i = 0; i < 500000; i++);
	}
}

/* taskb: 慢闪 ~800ms 周期 */
void taskb(void)
{
	while(1)
	{
		HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
		for (volatile int i = 0; i < 2000000; i++);
		HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
		for (volatile int i = 0; i < 2000000; i++);
	}
}

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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  task_create(&taska_tcb, taska_stack, STACK_SIZE, taska);
  task_create(&taskb_tcb, taskb_stack, STACK_SIZE, taskb);

  /*
   * PendSV 必须设为最低优先级 (15)。
   * 这样 PendSV 不会抢占 SysTick，保证上下文切换在 ISR 结束后才执行。
   */
  NVIC_SetPriority(PendSV_IRQn, 15);

  current_tcb = &taska_tcb;
  start_first_task();  /* SVC → Handler 模式 → 跳转到 taska */

  /* 不会执行到这里 */
  while (1)
  {
  }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_G_Pin */
  GPIO_InitStruct.Pin = LED_G_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_G_GPIO_Port, &GPIO_InitStruct);

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
