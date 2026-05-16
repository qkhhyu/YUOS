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

#define YUOS_MAX_PRIORITIES  256u
#define YUOS_PRIO_HIGHEST   0u
#define YUOS_PRIO_IDLE      (YUOS_MAX_PRIORITIES - 1u)
#define YUOS_TIME_SLICE 5u  //时间片轮询的时间

#define YUOS_MAX_TASKS 8    //最大任务数


#define STACK_SIZE 128


struct yuos_tcb {
    uint32_t *sp;  //栈指针 —— 切换任务就是切换这个
    uint32_t priority;// 任务优先级，数值越小优先级越高
    uint32_t state;	// 任务状态：就绪、运行、阻塞
	uint32_t delay_ticks;	// 任务剩余时间片（ticks）
    uint32_t last_tick;    // 上次被调度的系统 tick，用于计算运行时间
    uint32_t time_slice;    // 任务的时间片长度，单位 ticks
    struct yuos_tcb *next;  // 用于就绪队列的链表指针
    struct yuos_tcb *prev;  // 双向链表指针
};

struct yuos_kernel
{
    uint32_t ticks; // 系统运行的总 tick 数
};

struct yuos_sem
{
    uint32_t count; // 信号量计数
    struct yuos_tcb *wait_list; // 等待该信号量的任务队列
};


/* Exported kernel variables -------------------------------------------------*/
// extern uint32_t taska_stack[STACK_SIZE];
// extern uint32_t taskb_stack[STACK_SIZE];
extern struct yuos_kernel yuos_os;
extern uint32_t stack_pool[YUOS_MAX_TASKS][STACK_SIZE];
// extern struct yuos_tcb taska_tcb, taskb_tcb;
extern struct yuos_tcb tcb_pool[YUOS_MAX_TASKS];
extern struct yuos_tcb *current_tcb;
extern struct yuos_tcb *yuos_idle_tcb;

extern struct yuos_tcb *ready_list;
extern struct yuos_tcb *delay_list;
/* Exported kernel functions -------------------------------------------------*/
struct yuos_tcb *task_create(int stack_size,uint32_t priority, void (*task_func)(void));
void yuos_scheduler(void);
void start_first_task(void);

void yuos_ready_list_insert(struct yuos_tcb *tcb);
void yuos_delay_list_insert(struct yuos_tcb *tcb);
void yuos_ready_list_remove(struct yuos_tcb *tcb);
void yuos_delay_list_remove(struct yuos_tcb *tcb);

void yuos_sem_init(struct yuos_sem *sem, uint32_t initial_count);
void yuos_sem_wait(struct yuos_sem *sem);
void yuos_sem_post(struct yuos_sem *sem);

uint32_t enter_critical(void);
void exit_critical(uint32_t primask);
void sleep(uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
