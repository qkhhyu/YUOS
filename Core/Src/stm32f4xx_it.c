/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */


// extern struct yuos_tcb taska_tcb, taskb_tcb;
// extern struct yuos_tcb *current_tcb;
extern void scheduler(void);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  *
  * YUOS: 启动第一个任务。start_first_task() 触发 SVC 进入 Handler 模式。
  * 加载 current_tcb 指向的任务栈，弹出 R4-R11，bx lr 做异常返回，
  * 硬件自动弹出 R0-R3,R12,LR,PC,xPSR —— CPU 开始执行第一个任务。
  */
__attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile (
        "ldr r0, =current_tcb    \n"
        "ldr r1, [r0]            \n"
        "ldr r0, [r1]            \n"  //r0 = current_tcb->sp
        "ldmia r0!, {r4-r11}       \n"  //从任务栈弹出 R4-R11（ldmia：多寄存器加载，自动递增）
        "msr psp, r0              \n"  //PSP = 任务栈当前指针
        "mov r0,#2                \n"  //切换到 PSP 模式
        "msr CONTROL, r0          \n"  //CONTROL[1]=1 切 PSP，CONTROL[0]=1 非特权
        "isb                     \n"  //指令同步屏障，确保 CONTROL 寄存器修改生效
        "ldr lr, =0xFFFFFFFD     \n"
        "bx lr                   \n"  //异常返回 → 硬件弹出 R0-R3,PC 等 → 任务启动 EXC_RETURN 会自动选 PSP（因为 CONTROL=3）
    );
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  *
  * YUOS: 上下文切换核心。
  *  1. 硬件已自动压栈 R0-R3,R12,LR,PC,xPSR (8 words)
  *  2. 手动压栈 R4-R11 (8 words)
  *  3. 保存 SP → current_tcb->sp
  *  4. 调用 C 调度器选下一任务
  *  5. 加载新任务 SP ← current_tcb->sp
  *  6. 弹出 R4-R11
  *  7. bx lr → 硬件自动弹出异常帧 → CPU 无缝执行新任务
  */
__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile (
        "mrs r0, psp"           "\n"  //步骤1: 获取当前任务栈顶地址（PSP
        "stmdb r0!, {r4-r11}"   "\n"  //步骤2: 手动保存 R4-R11（stmdb：多寄存器存储，自动递减） 
        "ldr r1, =current_tcb    \n"
        "ldr r2, [r1]            \n"
        "str r0, [r2]            \n"  //步骤3: SP → current_tcb->sp
        "mov r4, lr              \n"
        "bl scheduler            \n"  //步骤4: 调用 C 调度器切换 current_tcb 
        "mov lr, r4              \n"  //恢复 lr，防止 scheduler 修改它
        "ldr r1, =current_tcb    \n"
        "ldr r2, [r1]            \n"
        "ldr r0, [r2]            \n"  //步骤5: 新任务 SP
        "ldmia r0!, {r4-r11}     \n"  //步骤6: 恢复 R4-R11
        "msr psp, r0             \n"  //更新 PSP
        "isb                     \n"  //指令同步屏障，确保上下文切换完成
        "bx lr                   \n"  //步骤7: 异常返回
    );
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /*
   * YUOS: 每 500ms 触发一次 PendSV 上下文切换。
   * SysTick 1ms tick，计数到 500 即 ~500ms。
   */
  // {
  //     static uint32_t tick_count = 0;
  //     tick_count++;
  //     if (tick_count >= 500) {
  //         tick_count = 0;
  //         SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;  /* 触发 PendSV */
  //     }
  // }
    // struct yuos_tcb *tasks[] = {&taska_tcb, &taskb_tcb};
    for(uint32_t i = 0; i < sizeof(tcb_pool)/sizeof(tcb_pool[0]); i++) 
	{
		if (tcb_pool[i].state == TASK_BLOCKED) 
		{
			uint32_t primask = enter_critical();
			tcb_pool[i].delay_ticks--;
			if(tcb_pool[i].delay_ticks ==0)
			{
				tcb_pool[i].state = TASK_READY;
				SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
			}
			exit_critical(primask);
		}
	}


  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
