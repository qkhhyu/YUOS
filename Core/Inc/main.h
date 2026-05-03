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
#include "stm32f4xx_hal.h"

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
#define LED_G_Pin GPIO_PIN_2
#define LED_G_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/* YUOS kernel types ---------------------------------------------------------*/
enum {
    TASK_UNUSED     = 0,
    TASK_READY      = 1,
    TASK_RUNNING    = 2,
    TASK_BLOCKED    = 3,
};

#define YUOS_MAX_PRIORITIES  255u
#define YUOS_PRIO_HIGHEST   0u
#define YUOS_PRIO_IDLE      (YUOS_MAX_PRIORITIES - 1u)

#define YUOS_MAX_TASKS 8    //最大任务数


#define STACK_SIZE 128


struct yuos_tcb {
    uint32_t *sp;  //栈指针 —— 切换任务就是切换这个
    uint32_t priority;// 任务优先级，数值越小优先级越高
    uint32_t state;	// 任务状态：就绪、运行、阻塞
	uint32_t delay_ticks;	// 任务剩余时间片（ticks）
};

/* Exported kernel variables -------------------------------------------------*/
// extern uint32_t taska_stack[STACK_SIZE];
// extern uint32_t taskb_stack[STACK_SIZE];
extern uint32_t stack_pool[YUOS_MAX_TASKS][STACK_SIZE];
// extern struct yuos_tcb taska_tcb, taskb_tcb;
extern struct yuos_tcb tcb_pool[YUOS_MAX_TASKS];
extern struct yuos_tcb *current_tcb;
extern struct yuos_tcb *yuos_idle_tcb;
/* Exported kernel functions -------------------------------------------------*/
struct yuos_tcb *task_create(int stack_size,uint32_t priority, void (*task_func)(void));
void scheduler(void);
void start_first_task(void);

uint32_t enter_critical(void);
void exit_critical(uint32_t primask);
void sleep(uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
