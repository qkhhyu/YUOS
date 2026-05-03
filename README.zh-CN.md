# YUOS

中文 | [English](README.md)

YUOS 是一个面向资源受限微控制器的轻量级实时操作系统。它围绕小型抢占式内核、确定性的任务调度、适合 Cortex-M 的上下文切换机制，以及平台无关内核和 CPU 移植层分离的结构进行设计。

当前首个支持平台是 STM32F407VET6 (Cortex-M4)。内核结构会尽量保持可移植，后续目标包括 STM32F1/F4、Nordic nRF52，以及基于 Cortex-M33 的芯片。

## 项目目标

- 提供一个紧凑的嵌入式抢占式 RTOS 内核。
- 保持调度器和内核对象的平台无关性。
- 使用小型 Cortex-M 移植层处理上下文切换、异常返回和中断控制。
- 支持任务延时、同步、消息传递、软件定时器和运行诊断等常用 RTOS 能力。

## 当前平台

| 项目 | 内容 |
|------|------|
| MCU | STM32F407VET6 |
| 内核 | ARM Cortex-M4 |
| Flash | 512 KB |
| RAM | 128 KB SRAM + 64 KB CCMRAM |
| 构建系统 | CMake + Ninja |
| 工程基础 | STM32CubeMX |

## 功能特性

已实现或正在完善：

- 任务控制块和独立任务栈。
- 基于 SysTick 和 PendSV 的上下文切换。
- 优先级调度。
- 就绪、运行、阻塞、未使用等任务状态。
- 临界区保护。
- 任务 sleep 和 tick 唤醒。
- 从 TCB 池和栈池动态创建任务。
- idle task 支持。

计划支持：

- 就绪队列和延时队列。
- 时间片轮转和任务主动让出 CPU。
- 信号量、互斥锁和优先级继承。
- 消息队列和软件定时器。
- 栈溢出检查、运行时诊断和 Fault 分析。
- 按需支持 FPU 上下文保存。

完整路线和实现记录见 [DEVELOPMENT.md](DEVELOPMENT.md)。

## 开发方式

当前 STM32 工程会先把内核代码放在接近硬件验证的位置，便于稳定核心机制：

```text
Core/Src/main.c
Core/Src/stm32f4xx_it.c
Core/Inc/main.h
```

目标工程结构：

```text
YUOS/kernel/          平台无关的调度器和内核对象
YUOS/port/cortex_m/   Cortex-M 启动、PendSV/SVC、MSP/PSP、临界区
YUOS/include/         YUOS 对外 API 头文件
Core/Src/main.c       应用测试任务
```

## 移植方向

YUOS 会尽量把平台无关内核和平台相关移植层分开。STM32F407 之后，最自然的第一个移植目标是 STM32F103 (Cortex-M3)，再往后可以尝试 Nordic nRF52/nRF53/nRF54 或 Cortex-M33 芯片。

移植边界：

- `kernel`：任务管理、调度器、链表、延时、信号量、互斥锁、消息队列。
- `port`：CPU 上下文切换、异常返回、关中断、FPU 上下文和启动细节。

## 许可说明

YUOS 内核代码目标是自包含实现。不要直接复制第三方 RTOS 的源码、注释、文件结构或私有实现细节。

当前工程包含厂商组件：

- CMSIS：Apache-2.0 许可。
- STM32 HAL Driver：BSD-3-Clause 兜底许可。
- STM32 Device Header：Apache-2.0 兜底许可。

请保留 `Drivers/` 下的原始 license 文件。后续如果发布或用于商业项目，建议为 YUOS 自研内核部分单独添加 `LICENSE` 文件，明确使用范围。
