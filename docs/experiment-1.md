# 实验 1：建立工程，点亮 LED

**日期**：2026-04-29
**状态**：✅ 已完成

---

## 目标

确保工具链、下载、硬件一切正常，为后续 RTOS 开发建立可用的工程环境。

---

## 硬件平台

| 项目 | 配置 |
|------|------|
| 芯片 | STM32F407VET6 (LQFP100, Cortex-M4) |
| 时钟 | HSI 16MHz（SYSCLK = HCLK = APB1 = APB2 = 16MHz） |
| 调试 | SWD (PA13-SWDIO, PA14-SWCLK) |
| LED | PB2 推挽输出，低电平点亮（标签 `LED_G`） |
| NVIC | 优先级分组 4（4位抢占/0位子优先级），SysTick 优先级 15 |

## 构建系统

- CMake（由 STM32CubeMX 6.16.1 生成）
- STM32Cube FW_F4 V1.28.3
- 编译器：ARM GCC (arm-none-eabi-gcc)

## 内存布局

```
FLASH:  0x08000000, 512KB
SRAM:   0x20000000, 128KB
CCMRAM: 0x10000000, 64KB
栈: 1KB, 堆: 512B
```

## 当前程序

main.c 中 HAL 初始化 → 配置系统时钟 → 初始化 GPIO → 主循环 LED 500ms 间隔闪烁（HAL_Delay 阻塞延时）。

## 关键文件

| 文件 | 用途 |
|------|------|
| `YUOS.ioc` | STM32CubeMX 项目配置 |
| `CMakeLists.txt` | 顶层 CMake 配置 |
| `cmake/stm32cubemx/CMakeLists.txt` | HAL 库 CMake 模块 |
| `startup_stm32f407xx.s` | 启动汇编（向量表、Reset_Handler） |
| `STM32F407XX_FLASH.ld` | 链接脚本 |
| `Core/Src/main.c` | 主程序 |
| `Core/Src/stm32f4xx_it.c` | 中断服务函数 |

## 技术笔记

- **NVIC_PRIORITYGROUP_4**：RTOS 需要精细的抢占优先级控制（PendSV 做上下文切换，SysTick 做时间片）。4 位全给抢占优先级 = 16 个抢占级别，子优先级由 RTOS 软件管理——RTOS 开发的标准配置。
- **SysTick 的后续用途**：当前由 HAL_Delay 占用，后续 RTOS 将接管 SysTick 作为系统 Tick 心跳驱动调度器。
- **CCMRAM 的潜力**：64KB 紧耦合内存，零等待访问，后续可放 TCB 链表、就绪队列等内核热数据，减少总线竞争。

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
