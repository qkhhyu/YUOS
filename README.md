# YUOS

[中文](README.zh-CN.md) | English

YUOS is a lightweight real-time operating system for resource-constrained microcontrollers. It is designed around a small preemptive kernel, deterministic task scheduling, Cortex-M friendly context switching, and a clean separation between platform-independent kernel code and CPU-specific port code.

The initial platform is STM32F407VET6 (Cortex-M4). The kernel architecture is intended to be portable across Cortex-M MCUs such as STM32F1/F4, Nordic nRF52, and newer Cortex-M33 based devices.

## Goals

- Provide a compact preemptive RTOS kernel for embedded applications.
- Keep the core scheduler and kernel objects platform-independent.
- Use a small Cortex-M port layer for context switching, exception return, and interrupt control.
- Support practical RTOS primitives such as task delays, synchronization, message passing, timers, and diagnostics.

## Current Platform

| Item | Value |
|------|-------|
| MCU | STM32F407VET6 |
| Core | ARM Cortex-M4 |
| Flash | 512 KB |
| RAM | 128 KB SRAM + 64 KB CCMRAM |
| Build system | CMake + Ninja |
| Generated base | STM32CubeMX |

## Features

Implemented or in progress:

- Task control blocks and per-task stacks.
- SysTick and PendSV based context switching.
- Priority-based scheduling.
- Task states for ready, running, blocked, and unused slots.
- Critical section protection.
- Tick-based task sleep and wakeup.
- Dynamic task creation from TCB and stack pools.
- Idle task support.

Planned:

- Ready lists and delay lists.
- Time slicing and task yielding.
- Semaphores, mutexes, and priority inheritance.
- Message queues and software timers.
- Stack overflow checks, runtime diagnostics, and fault analysis.
- FPU context handling where required.

See [DEVELOPMENT.md](DEVELOPMENT.md) for the full roadmap and implementation notes.

## Development Approach

The current STM32 project keeps kernel code close to the bring-up code while the core mechanisms are being stabilized:

```text
Core/Src/main.c
Core/Src/stm32f4xx_it.c
Core/Inc/main.h
```

The intended project structure is:

```text
YUOS/kernel/          platform-independent scheduler and kernel objects
YUOS/port/cortex_m/   Cortex-M startup, PendSV/SVC, MSP/PSP, critical sections
YUOS/include/         public YUOS API headers
Core/Src/main.c       application/demo tasks
```

## Portability

The kernel is being designed so the platform-independent part can be reused across Cortex-M MCUs. The first expected porting target after STM32F407 is STM32F103 (Cortex-M3), followed by other Cortex-M chips such as Nordic nRF52/nRF53/nRF54 or Cortex-M33 devices.

The main portability boundary is:

- `kernel`: task management, scheduler, lists, delays, semaphores, mutexes, queues.
- `port`: CPU-specific context switch, exception return, interrupt masking, FPU context, and startup details.

## Licensing Notes

YUOS kernel code is intended to be self-contained. Third-party RTOS source code should not be copied into this project.

The project currently includes vendor components:

- CMSIS: Apache-2.0 license.
- STM32 HAL driver: BSD-3-Clause fallback license.
- STM32 device headers: Apache-2.0 fallback license.

Keep the original license files under `Drivers/`. Before publishing or using YUOS commercially, add a dedicated `LICENSE` file for the self-written YUOS kernel code.
