# YUOS — 从零开始开发一个 RTOS 内核

> 适用对象：有单片机裸机编程经验，想彻底理解 RTOS 内核原理的嵌入式开发者。
> 硬件平台：STM32F407VET6 (Cortex-M4, 512KB Flash, 128KB RAM, 64KB CCMRAM)
> 最终目标：亲手写出一个具备抢占式调度、任务延时、任务间通信的微型 RTOS 内核。

---

## 一、内核是什么

### 1.1 裸机程序的问题

```c
while(1) {
    点亮LED;
    延时1秒;
    熄灭LED;
    延时1秒;
}
```

CPU 死死守着这一个循环。如果还想让 CPU 去读温度传感器，必须把读温度的代码也塞进同一个循环。**延时期间 CPU 在"闭眼"，完全无法响应其他事情。**

### 1.2 RTOS 内核解决什么

把单片机想象成只有一个服务员的餐厅：

| | 裸机程序 | RTOS 内核 |
|---|---|---|
| 模式 | 服务员只能一桌一桌服务，A桌没完就不能去B桌 | 超级大管家，每隔一小段时间巡视一圈 |
| 决策 | 无 | 看哪桌客人最着急（优先级最高），立刻切换 |
| 效果 | 串行、阻塞 | 快速跳转，人眼察觉不到，感觉"同时"运行 |

**内核的核心工作就是"切换"——让 CPU 在不同任务间快速跳转。**

### 1.3 "切换"到底是怎么实现的？就三步

1. **保存现场**：暂停当前任务，把 CPU 所有寄存器的值原封不动存入该任务的专属栈
2. **恢复现场**：从新任务的栈里，把上次保存的寄存器值全部弹回 CPU
3. **开始执行**：CPU 无缝衔接地继续跑新任务

**内核的本质：就是一段管理函数，不停执行上面这三步。**

---

## 二、核心概念速查表

| 核心概念 | 需要掌握的 |
|----------|-----------|
| 栈 (Stack) | 每个任务专属的本地存储区，存函数调用、局部变量。**切换任务本质就是切换栈指针 SP** |
| 任务控制块 (TCB) | 任务的"身份证"，至少包含栈指针成员，后续扩展优先级、状态、延时等 |
| PendSV 异常 | ARM Cortex-M 核专为 OS 任务切换设计的"绅士中断"，用它实现切换最安全、最优雅 |
| SysTick 定时器 | 内核的"心跳"，每隔固定时间（如 1ms）产生中断，调度器在这里判断是否需要切换 |
| 链表 | 内核管理多个任务的核心数据结构，TCB 通过链表串联起来 |
| 临界区 | 不可打断的代码段，通过关中断/开中断来保护 |

---

## 三、开发路线图

当前路线的目标不是重复造一遍已有实验，而是沿着已有成果继续补齐一个 RTOS 必须具备的核心能力。前 8 个实验作为学习基础已经完成，后续重点转向“语义正确、结构清晰、可以真实使用”。

| 阶段 | 状态 | 内容 | 学习重点 |
|------|------|------|----------|
| 实验 1 | ✅ 已完成 | 建立工程，点亮 LED | 工具链、下载、GPIO、基础启动流程 |
| 实验 2 | ✅ 已完成 | 任务栈 + 手动切换（合作式调度） | 栈指针切换、R4-R11/LR 保存恢复、naked 汇编 |
| 实验 3 | ✅ 已完成 | SysTick + PendSV 抢占式调度 | 异常入口/返回、PendSV 延后切换、SysTick 心跳 |
| 实验 4 | ✅ 已完成 | 加入优先级 | 固定优先级调度、最高优先级就绪任务运行 |
| 实验 5 | ✅ 已完成 | 完善 TCB | `sp`、`priority`、`state`、`delay_ticks` 的职责 |
| 实验 6 | ✅ 已完成 | 临界区保护 | PRIMASK、关中断保护内核共享数据 |
| 实验 7 | ✅ 已完成 | `sleep()` 任务延时睡眠 | 阻塞态、tick 递减、到期唤醒 |
| 实验 8 | ✅ 已完成 | `task_create()` 动态创建任务 | TCB 池、栈池、BSS 零初始化约定 |

后续推荐路线：

| 阶段 | 状态 | 内容 | 目标 |
|------|------|------|------|
| 补强 1 | ✅ 已完成（2026-04-30） | 修正任务栈模型：Handler 使用 MSP，任务使用 PSP | 让内核符合 Cortex-M RTOS 的标准异常模型，为真实项目打地基 |
| 补强 2 | ✅ 已完成（2026-04-30） | 收紧任务状态机 | 调度器只选择 `READY` 任务，切走的 `RUNNING` 任务回到 `READY` 或进入 `BLOCKED` |
| 补强 3 | ✅ 已完成（2026-04-30） | 加入 idle task | 所有业务任务阻塞时系统仍有合法任务运行，解决”阻塞任务被调度回来”的问题 |
| 补强 4 | ✅ 已完成（2026-05-03） | 完善 `task_create()` 工程护栏 | 参数检查、栈 8 字节对齐、任务返回处理、栈溢出标记 |
| 实验 9 | ✅ 已完成（2026-05-03） | `yield()` + 同优先级时间片轮转 | 学会主动让出 CPU，并让同优先级任务公平运行 |
| 实验 10 | ✅ 已完成（2026-05-03） | 就绪队列与延时队列 | 从遍历 TCB 池过渡到链表/队列，理解 RTOS 数据结构骨架 |
| 实验 11 | 🔲 待开始 | 信号量 | 任务等待/唤醒、ISR 释放信号量、资源计数 |
| 实验 12 | 🔲 待开始 | 互斥锁 + 优先级继承 | 处理优先级反转，这是 RTOS 能否上项目的重要分水岭 |
| 实验 13 | 🔲 待开始 | 消息队列 | FIFO 环形缓冲区、任务间传参、阻塞读写 |
| 实验 14 | 🔲 待开始 | 软件定时器 | 定时回调、tick list、延时任务和定时任务的区别 |
| 实验 15 | 🔲 待开始 | 内存管理 | 固定块内存池优先，再学习简单 heap，避免碎片化 |
| 实验 16 | 🔲 待开始 | 中断与内核 API 规则 | 区分任务上下文 API 和 ISR-safe API，设计 `FromISR` 类接口 |
| 实验 17 | 🔲 待开始 | 调试与可靠性 | HardFault 解析、栈水位、断言、运行时统计、trace hook |
| 实验 18 | 🔲 待开始 | FPU 上下文保存 | STM32F407 带 FPU，任务使用浮点时需要保存 S16-S31 等上下文 |
| 实验 19 | 🔲 待开始 | 移植层拆分 | 把内核通用代码和 Cortex-M port 代码分离，形成可移植结构 |
| 实验 20 | 🔲 待开始 | 真实小项目验证 | 用 YUOS 驱动 LED、按键、串口、传感器等多个任务，验证可用性 |

阶段性完成标准：

- 学完补强 1-4：掌握 Cortex-M RTOS 上下文切换的正确骨架。
- 学完实验 9-10：掌握调度器、就绪队列、延时队列这些内核基础结构。
- 学完实验 11-14：掌握任务同步、互斥、通信、定时器这些 RTOS 应用核心。
- 学完实验 15-18：具备面向真实项目的稳定性、安全性和调试能力。
- 学完实验 19-20：得到一个结构清楚、能在小型嵌入式项目中使用的教学型 RTOS。

---

## 四、实验记录

### 实验 1：建立工程，点亮 LED

**状态**：✅ 已完成（2026-04-29）

**目标**：确保工具链、下载、硬件一切正常。

**硬件平台**：
- 芯片：STM32F407VET6 (LQFP100)
- 时钟：HSI 16MHz（SYSCLK = HCLK = APB1 = APB2 = 16MHz）
- 调试：SWD (PA13-SWDIO, PA14-SWCLK)
- LED：PB2 推挽输出，低电平点亮（标签 `LED_G`）
- NVIC：优先级分组 4（4位抢占/0位子优先级），SysTick 优先级 15

**构建系统**：CMake（由 STM32CubeMX 6.16.1 生成），STM32Cube FW_F4 V1.28.3

```
内存布局：
  FLASH:  0x08000000, 512KB
  SRAM:   0x20000000, 128KB
  CCMRAM: 0x10000000, 64KB
栈: 1KB, 堆: 512B
```

**当前程序**：main.c 中 HAL 初始化 → 配置系统时钟 → 初始化 GPIO → 主循环 LED 500ms 间隔闪烁（HAL_Delay 阻塞延时）。

**关键文件**：

| 文件 | 用途 |
|------|------|
| `YUOS.ioc` | STM32CubeMX 项目配置 |
| `CMakeLists.txt` | 顶层 CMake 配置 |
| `cmake/stm32cubemx/CMakeLists.txt` | HAL 库 CMake 模块 |
| `startup_stm32f407xx.s` | 启动汇编（向量表、Reset_Handler） |
| `STM32F407XX_FLASH.ld` | 链接脚本 |
| `Core/Src/main.c` | 主程序 |
| `Core/Src/stm32f4xx_it.c` | 中断服务函数（SysTick_Handler 在此） |

**技术笔记**：

- **为什么选 NVIC_PRIORITYGROUP_4？** RTOS 需要精细的抢占优先级控制（PendSV 做上下文切换，SysTick 做时间片）。4 位全给抢占优先级 = 16 个抢占级别，子优先级由 RTOS 软件管理——这是 RTOS 开发的标准配置。
- **SysTick 的后续用途**：当前由 HAL_Delay 占用，后续 RTOS 需要接管 SysTick 作为系统 Tick 心跳来驱动调度器。
- **CCMRAM 的潜力**：64KB 紧耦合内存，零等待访问，后续可放 TCB 链表、就绪队列等内核热数据，减少总线竞争。

> 详细记录：[docs/experiment-1.md](docs/experiment-1.md)

---

### 实验 2：任务栈 + 手动切换（合作式调度）

**状态**：✅ 已完成（2026-04-29）

**目标**：两个死循环函数（TaskA、TaskB），各自主动调用 Switch()，从 A 跳到 B，再从 B 跳回 A。LED 交替呈现快闪/慢闪——任务主动让出 CPU，非抢占。

#### 已实现代码

**TCB 和任务栈**（`Core/Src/main.c`）：

```c
#define STACK_SIZE 128
uint32_t taska_stack[STACK_SIZE];
uint32_t taskb_stack[STACK_SIZE];

struct tcb {
    uint32_t *sp;  // 栈指针——最关键的成员
};
struct tcb taska_tcb, taskb_tcb;
struct tcb *current_tcb;
```

**task_create() — 初始化任务栈**：

```c
void task_create(struct tcb *tcb, uint32_t *stack, int stack_size, void (*task_func)(void))
{
    uint32_t *sp = &stack[stack_size - 1];

    // 栈布局（高地址→低地址）：PC, R11, R10, ..., R4
    // Switch() 中通过 pop {r4-r11, pc} 恢复
    *(--sp) = (uint32_t)task_func;  // PC —— 首次运行时从这里开始
    for (int i = 11; i >= 4; i--) {
        *(--sp) = i;                 // R11~R4 的初始值
    }
    tcb->sp = sp;
}
```

**Switch() — 汇编上下文切换**：

```c
__attribute__((naked)) void Switch(void)
{
    __asm volatile (
        "push {r4-r11, lr}       \n"  // 保存 R4-R11 和返回地址
        "ldr r0, =current_tcb    \n"
        "ldr r1, [r0]            \n"
        "str sp, [r1]            \n"  // 当前 SP → current_tcb->sp

        "ldr r2, =taska_tcb      \n"
        "cmp r1, r2              \n"
        "beq to_taskB            \n"
        "ldr r0, =taska_tcb      \n"
        "b restore               \n"
        "to_taskB:               \n"
        "ldr r0, =taskb_tcb      \n"
        "restore:                \n"
        "ldr r1, =current_tcb    \n"
        "str r0, [r1]            \n"  // current_tcb = 新任务
        "ldr sp, [r0]            \n"  // SP = 新任务栈指针

        "pop {r4-r11, pc}        \n"  // 恢复 R4-R11，PC 跳转到新任务
    );
}
```

**main() 启动**：

```c
task_create(&taska_tcb, taska_stack, STACK_SIZE, taska);
task_create(&taskb_tcb, taskb_stack, STACK_SIZE, taskb);
current_tcb = &taska_tcb;
taska();  // 直接调用，之后由 Switch() 接管
```

#### 遇到的 Bug 及原因

| Bug | 症状 | 根因 |
|-----|------|------|
| `str r0, [r0]` 破坏栈指针 | taskB 永远不执行 | 把 `&taskb_tcb` 写入了 `taskb_tcb.sp`，覆盖了初始化好的栈指针 |
| `push {r4-r11}` 缺 LR + `bx lr` | Switch 只能回到调用者 | switch 出去后 `bx lr` 用的是旧的 LR，永远跳到旧任务的返回地址 |

**修复**：删掉 `str r0, [r0]`；`push` 加上 `lr`，`pop` 用 `pc` 替代 `bx lr`。

#### 执行流

```
taskA 调用 Switch()
  → push {r4-r11, lr}    // LR=taskA中Switch后的返回地址
  → 保存 SP → taskA_tcb.sp
  → 加载 taskB_tcb.sp → SP 切到 taskB 的栈
  → pop {r4-r11, pc}     // PC=taskb，跳过去！

taskB 调用 Switch()
  → push {r4-r11, lr}    // LR=taskB中Switch后的返回地址
  → 保存 SP → taskB_tcb.sp
  → 加载 taskA_tcb.sp → SP 切回 taskA 的栈(MSP)
  → pop {r4-r11, pc}     // PC=taskA中Switch的下一行，接着跑！
```

**现象**：同一个 LED 交替呈现快闪（taskA ~200ms 周期）和慢闪（taskB ~800ms 周期）。

**关键理解**：
- `__attribute__((naked))` 禁止编译器生成函数序言/尾声，避免破坏手动栈操作
- `push {r4-r11, lr}` 保存"被调用者保存"寄存器 + 返回地址，共 9 个字
- `pop {r4-r11, pc}` 恢复寄存器并直接跳转到新任务（替代 `bx lr`）
- 这种切换方式是**合作式**的——任务必须主动调用 Switch() 才能让出 CPU，不调用别人永远跑不了

> 详细记录：[docs/experiment-2.md](docs/experiment-2.md)

---

### 实验 3：SysTick 中断（抢占式调度）

**状态**：✅ 已完成（2026-04-30）

**目标**：不管当前任务愿不愿意，时间一到硬件强行打断切换。

#### 实现要点

**整体流程**：

```
SysTick (每1ms) → 计数到500 → 触发 PendSV
                                  ↓
PendSV: 保存当前任务 → 调用 scheduler() → 恢复新任务
                                  ↓
                          bx lr → 硬件自动弹栈 → CPU 开始跑新任务
```

**SysTick_Handler** (`Core/Src/stm32f4xx_it.c`)：
- 保留 HAL_IncTick() 供 HAL 库使用
- 每 1ms tick_count++，到 500（~500ms）触发 PendSV
- 不直接做切换——只在 SysTick 里"通知" PendSV

**PendSV_Handler** (`Core/Src/stm32f4xx_it.c`)：
```asm
push {r4-r11}         ; 手动保存 R4-R11（硬件已自动保存 R0-R3,R12,LR,PC,xPSR）
str sp, [current_tcb] ; 当前 SP → 旧任务 TCB
bl scheduler          ; C 函数，改变 current_tcb 指向新任务
ldr sp, [current_tcb] ; 新任务 SP ← 新任务 TCB
pop {r4-r11}          ; 恢复 R4-R11
bx lr                 ; 异常返回，硬件弹出 R0-R3,PC 等 → 无缝跳转！
```

**PendSV 切换全过程**（假设 taskA 正在跑，SysTick 触发切换）：

```
时间轴
─────→

① taskA 在 Thread 模式正常执行
   SP → taska_stack  (当前在 taskA 的栈上)

② SysTick 计数到 500，置位 PendSV 挂起位
   PendSV 不立即执行——它优先级最低(15)，等所有高优先级中断先处理完

③ PendSV 开始响应
   硬件自动压栈到 taska_stack（8 words）：
   ┌──────────┐
   │  xPSR    │ ← 硬件自动保存
   │  PC      │   （taskA 被打算时正在执行的指令地址）
   │  LR      │
   │  R12     │
   │  R3      │
   │  R2      │
   │  R1      │
   │  R0      │
   │  R11     │ ← push {r4-r11} 手动保存
   │  R10     │
   │  R9      │
   │  R8      │
   │  R7      │
   │  R6      │
   │  R5      │
   │  R4      │
   └──────────┘
   SP 现在指向 R4（栈底）

④ str sp, [r1]  →  taska_tcb.sp = SP
   ↑ 把 taskA 的"断点位置"写入它的身份证（TCB）

⑤ bl scheduler  →  current_tcb = &taskb_tcb
   调度器把全局指针指向 taskB 的 TCB

⑥ ldr sp, [r1]  →  SP = taskb_tcb.sp
   ↑ SP 从 taska_stack 切到 taskb_stack —— 切换发生！
   ┌──────────┐
   │  xPSR    │
   │  PC      │ ← taskB 上次被打算时保存的断点
   │  LR      │
   │  ...     │
   │  R5      │
   │  R4      │
   └──────────┘
   SP 现在指向 taskB 的栈

⑦ pop {r4-r11}  →  CPU 的 R4~R11 恢复为 taskB 上次的值

⑧ bx lr  →  硬件自动弹栈：R0~R3, R12, LR, PC, xPSR
             PC 变成 taskB 的断点 → CPU 无缝继续跑 taskB
```

**核心就两个赋值**：
- `str sp, [r1]` — 当前 SP → 旧任务 TCB.sp（保存断点）
- `ldr sp, [r1]` — 新任务 TCB.sp → SP 寄存器（加载断点）

**SP 寄存器一变，后续所有 push/pop/bx 全作用在新任务的栈上——任务就切过去了。**

**SVC_Handler** (`Core/Src/stm32f4xx_it.c`)：
- `start_first_task()` 触发 `SVC 0` 进入 Handler 模式
- SVC 里加载第一个任务的 SP → pop R4-R11 → bx lr → 任务启动
- 不在 Thread 模式下直接 bx lr（Thread 模式的 bx lr 不是异常返回）

**task_create 栈布局变化**（`Core/Src/main.c`）：

与实验 2 不同，现在的栈布局要匹配 PendSV 的异常帧格式：

```
高地址
  xPSR = 0x01000000  ← 硬件异常返回帧 (8 words)
  PC   = task_func
  LR   = 0
  R12  = 0
  R3   = 0
  R2   = 0
  R1   = 0
  R0   = 0
  R11  = 11           ← PendSV 手动帧 (8 words)
  ...
  R4   = 4
低地址 ← SP
```

**优先级配置**：
- NVIC 优先级分组 4（全抢占），与实验 1 一致
- SysTick 优先级：15（HAL_Init 设置）
- PendSV 优先级：15（手动设置 `NVIC_SetPriority(PendSV_IRQn, 15)`）
- 两者同优先级 → SysTick 不会抢占 PendSV，反之亦然

#### 关键理解

- **抢占 vs 合作**：实验 2 的任务必须主动调用 Switch() 让出 CPU；实验 3 的任务只管死循环，SysTick 硬件定时打断，任务完全无感知
- **PendSV 的意义**：如果直接在 SysTick ISR 里做切换，会阻塞其他同/低优先级中断。PendSV 是最低优先级，等所有 ISR 处理完再切换——最安全优雅
- **异常返回机制**：bx lr 在 Handler 模式下识别 EXC_RETURN 值，通知硬件自动弹出 R0-R3,R12,LR,PC,xPSR——这就是为什么 PendSV 只需要手动管理 R4-R11
- **SVC 启动**：main() 在 Thread 模式，不能直接加载任务栈 + bx lr（Thread 模式的 LR 不是 EXC_RETURN）。SVC 进入 Handler 模式后，bx lr 才有异常返回的语义

#### 观察现象

同一颗 LED 仍然呈现快闪/慢闪交替——但这次任务代码里没有 Switch() 调用，切换完全由硬件 SysTick 定时驱动。这就是"抢占"。

#### 遇到的 Bug

| Bug | 症状 | 根因 |
|-----|------|------|
| `bl scheduler` 覆盖 LR | LED 亮一下→全灭→系统挂死 | `bl` 把返回地址写入 LR，覆盖了异常入口时的 EXC_RETURN 值（`0xFFFFFFF9`）。`bx lr` 时 LR 不再是异常返回标记，不会做硬件弹栈，CPU 跳到非法地址进入 HardFault |
| `ldr r2, [lr]` 尝试暂存 LR | HardFault | LR = `0xFFFFFFF9` 是一个特殊编码值，不是合法内存地址。`ldr` 试图从该地址读数据，CPU 直接 HardFault |
| 用 R2 暂存 LR | 编译看似正确，运行时破坏 | R0-R3 是 caller-saved 寄存器，`bl scheduler` 执行的 C 代码可随意修改它们。`scheduler()` 返回后 R2 大概率已被污染 |

**最终修复**（`PendSV_Handler`）：

```asm
mov r4, lr              ; 借 R4 暂存 EXC_RETURN（R4 是 callee-saved，C 函数保证不破坏）
bl scheduler
mov lr, r4              ; 归还 EXC_RETURN
```

**里程碑**：RTOS 的灵魂——"抢占"诞生

> 详细记录：[docs/experiment-3.md](docs/experiment-3.md)

---

### 实验 4：加入优先级

**状态**：✅ 已完成（2026-04-30）

**目标**：高优先级就绪时，强行剥夺低优先级任务的 CPU。

#### 实现内容

**TCB 新增 priority 字段**（`Core/Inc/main.h`）：

```c
struct tcb {
    uint32_t *sp;        /* 栈指针 */
    uint32_t priority;   /* 任务优先级，数值越小优先级越高 */
};
```

**task_create() 新增 priority 参数**（`Core/Src/main.c`）：

```c
void task_create(struct tcb *tcb, uint32_t *stack, int stack_size,
                 uint32_t priority, void (*task_func)(void))
{
    // ... 栈初始化不变 ...
    tcb->priority = priority;
}
```

**scheduler() 改为优先级调度**（`Core/Src/main.c`）：

```c
void scheduler(void)
{
    current_tcb = taska_tcb.priority < taskb_tcb.priority
                  ? &taska_tcb : &taskb_tcb;
}
```

**main() 中指定优先级**：

```c
task_create(&taska_tcb, taska_stack, STACK_SIZE, 1, taska);  // 优先级 1（高）
task_create(&taskb_tcb, taskb_stack, STACK_SIZE, 2, taskb);  // 优先级 2（低）
```

#### 观察现象

LED 一直快闪——taskA 优先级高（1 < 2），始终被调度选中；taskB 优先级低，永远饿死（饥饿）。这验证了优先级调度正确生效。

#### 当前局限

`scheduler()` 硬编码了 `taska_tcb` / `taskb_tcb` 全局变量比较，仅支持两个任务。后续实验将用链表和状态字段替代此临时实现。

**里程碑**：代码现在可以叫 RTOS 了

> 详细记录：[docs/experiment-4.md](docs/experiment-4.md)

---

### 实验 5：完善 TCB

**状态**：✅ 已完成（2026-04-30）

**目标**：添加任务状态和延时字段，为 sleep() 做准备。

#### 实现内容

**TCB 重命名并扩展**（`Core/Inc/main.h`）：

```c
enum {
    TASK_READY   = 0,
    TASK_RUNNING = 1,
    TASK_BLOCKED = 2,
};

struct yuos_tcb {
    uint32_t *sp;         /* 栈指针 */
    uint32_t priority;    /* 优先级，数值越小越高 */
    uint32_t state;       /* 状态：就绪/运行/阻塞 */
    uint32_t delay_ticks; /* 剩余延时 tick 数，0 = 无延时 */
};
```

**task_create() 初始化新字段**（`Core/Src/main.c`）：

```c
void task_create(struct yuos_tcb *tcb, ...)
{
    // ... 栈初始化不变 ...
    tcb->priority    = priority;
    tcb->state       = TASK_READY;
    tcb->delay_ticks = 0;
}
```

**scheduler() 改为遍历数组**（`Core/Src/main.c`）：

```c
void scheduler(void)
{
    struct yuos_tcb *next = current_tcb;
    struct yuos_tcb *tasks[] = { &taska_tcb, &taskb_tcb };

    for (int i = 0; i < 2; i++) {
        if (tasks[i]->delay_ticks == 0) {
            if (next->delay_ticks > 0 || tasks[i]->priority < next->priority)
                next = tasks[i];
        }
    }

    current_tcb = next;
}
```

#### 关键变化

1. **struct tcb → struct yuos_tcb**：避免与 HAL 库或未来代码的命名冲突
2. **scheduler() 从硬编码 if-else → 数组遍历**：加新任务只需往数组里加元素，调度逻辑不动
3. **state 字段已定义但未参与调度**：当前仅用 `delay_ticks` 判断，state 将在实验 7/8 中启用
4. **两任务都无延时**：行为与实验 4 一致（taskA 高优先级一直跑），验证重构无回归

> 详细记录：[docs/experiment-5.md](docs/experiment-5.md)

---

### 实验 6：临界区保护（关/开中断）

**状态**：✅ 已完成（2026-04-30）

**目标**：保护内核关键数据结构（TCB、就绪列表）不被中断破坏。

#### 实现内容

**enter_critical() / exit_critical()**（`Core/Src/main.c`）：

```c
uint32_t enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();  // 读取当前中断状态
    __disable_irq();
    return primask;                       // 返回旧状态
}

void exit_critical(uint32_t primask)
{
    __set_PRIMASK(primask);               // 恢复到旧状态
}
```

**scheduler() 中使用**：

```c
void scheduler(void)
{
    // ... 遍历选任务 ...
    uint32_t primask = enter_critical();
    current_tcb = next;
    exit_critical(primask);
}
```

#### 设计决策

- **保存/恢复 PRIMASK，而非计数嵌套**：更简单、无全局变量、在 ISR 内调用也安全（不会错误开启中断）
- **临界区只保护 `current_tcb = next` 一行**：保护范围精确到最小——只保需要保的

#### 观察现象

与实验 5 行为一致（LED 快闪），临界区保护无可见副作用——这是正确结果。

> 详细记录：[docs/experiment-6.md](docs/experiment-6.md)

---

### 实验 7：sleep() 任务延时睡眠

**状态**：✅ 已完成（2026-04-30）

**目标**：任务主动睡眠，延时期间让出 CPU，到期自动唤醒。

#### 实现内容

**sleep() — 任务延时函数**（`Core/Src/main.c`）：

```c
void sleep(uint32_t ticks)
{
    uint32_t primask = enter_critical();
    current_tcb->delay_ticks = ticks;
    current_tcb->state = TASK_BLOCKED;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;  // 触发 PendSV
    exit_critical(primask);
}
```

不调用 `scheduler()`——只设延时+触发展开，让 PendSV_Handler 做真正的切换。

**SysTick_Handler — 倒计时器**（`Core/Src/stm32f4xx_it.c`）：

```c
struct yuos_tcb *tasks[] = {&taska_tcb, &taskb_tcb};
for (uint32_t i = 0; i < 2; i++) {
    if (tasks[i]->state == TASK_BLOCKED) {
        uint32_t primask = enter_critical();
        tasks[i]->delay_ticks--;
        if (tasks[i]->delay_ticks == 0) {
            tasks[i]->state = TASK_READY;
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
        exit_critical(primask);
    }
}
```

每 1ms 对所有 BLOCKED 任务递减 delay_ticks，归零时唤醒并触发切换。

**scheduler() 更新**：选中任务后设置 `state = TASK_RUNNING`。

#### 验证策略

taskB 不调用 sleep（始终就绪），taskA 快闪 5 次后 sleep(5000)：

```c
void taska(void) {
    while(1) {
        for (int i = 0; i < 5; i++) {
            LED_ON(); for (volatile int i=0;i<500000;i++);   // ~100ms
            LED_OFF(); for (volatile int i=0;i<500000;i++);
        }
        sleep(5000);  // 阻塞 5 秒
    }
}

void taskb(void) {
    while(1) {
        LED_ON(); for (volatile int i=0;i<2000000;i++);   // ~400ms
        LED_OFF(); for (volatile int i=0;i<2000000;i++);
    }
}
```

**观察现象**：LED 快闪 5 下（taskA）→ 慢闪约 5 秒（taskB）→ 快闪 5 下（taskA）→ 交替循环。sleep() 生效——任务主动睡眠、按时唤醒、优先级抢占全部跑通。

#### 当前局限

- 两个任务同时阻塞时无空闲任务兜底，`scheduler()` 退回当前任务（此时 sleep 变 no-op）
- taskB 被迫用忙等保持就绪——下一步实验将加入空闲任务解决

> 详细记录：[docs/experiment-7.md](docs/experiment-7.md)

---

### 实验 8：task_create() 动态创建任务

**状态**：✅ 已完成（2026-04-30）

**目标**：从硬编码全局变量过渡到任务池机制，`task_create()` 自动分配 TCB 和栈。

#### 实现内容

**任务池**（`Core/Src/main.c`）：

```c
#define YUOS_MAX_TASKS 4

struct yuos_tcb tcb_pool[YUOS_MAX_TASKS];       // TCB 池
uint32_t stack_pool[YUOS_MAX_TASKS][STACK_SIZE]; // 栈池
```

**task_create() 从池中分配**：

```c
struct yuos_tcb *task_create(int stack_size, uint32_t priority,
                              void (*task_func)(void))
{
    struct yuos_tcb *tcb = NULL;
    int slot = -1;

    for (int i = 0; i < YUOS_MAX_TASKS; i++) {
        if (tcb_pool[i].state == TASK_UNUSED) {
            tcb = &tcb_pool[i];
            slot = i;
            break;
        }
    }

    if (tcb == NULL) return NULL;  // 池满

    uint32_t *sp = &stack_pool[slot][stack_size - 1];
    // ... 栈初始化不变 ...
    tcb->sp = sp;
    tcb->priority = priority;
    tcb->state = TASK_READY;
    tcb->delay_ticks = 0;
    return tcb;
}
```

**调用方改为接收返回指针**：

```c
struct yuos_tcb *taska_tcb, *taskb_tcb;  // 改为指针

taska_tcb = task_create(STACK_SIZE, 1, taska);
taskb_tcb = task_create(STACK_SIZE, 2, taskb);
current_tcb = taska_tcb;
```

**scheduler() / SysTick_Handler 改为池遍历**：不再硬编码两个任务，统一用 `sizeof(tcb_pool)/sizeof(tcb_pool[0])` 遍历。

#### 遇到的 Bug

| Bug | 症状 | 根因 |
|-----|------|------|
| `TASK_READY = 0` 与 BSS 零初始化冲突 | LED 不亮，系统崩溃 | enum 中 `TASK_READY=0`，池中所有 TCB 的 state 默认 0 → 全部被误认为就绪态，无一空闲槽。改为 `TASK_UNUSED=0` |
| `tcb` 未赋值 | 野指针崩溃 | 循环中设了 `sp` 没设 `tcb` |

#### 关键变化

- 增删任务不再需要声明全局 TCB 和栈——`task_create()` 一行搞定，池大小控制上限
- enum 顺序调整为 `TASK_UNUSED=0`：利用 BSS 默认零值，免除显式初始化

> 详细记录：[docs/experiment-8.md](docs/experiment-8.md)

---

### 后续补强与实验路线

**状态**：🔲 待开始

从这里开始不重复前 8 个实验，而是在已有代码上补齐 RTOS 走向可用所需的关键能力。

| 阶段 | 内容 | 为什么要做 |
|------|------|------------|
| 补强 1 | 修正任务栈模型：Handler 使用 MSP，任务使用 PSP | 当前实验已经理解了切换原理，下一步要切到 Cortex-M RTOS 的标准模型 |
| 补强 2 | 收紧任务状态机 | 让 `READY/RUNNING/BLOCKED` 的语义闭合，避免阻塞任务被错误调度 |
| 补强 3 | 加入 idle task | 保证所有业务任务阻塞时系统仍有合法执行流 |
| 补强 4 | 完善 `task_create()` 工程护栏 | 增加参数检查、栈对齐、任务返回处理、栈溢出检测 |
| 实验 9 | `yield()` + 同优先级时间片轮转 | 从固定优先级调度扩展到公平调度 |
| 实验 10 | 就绪队列与延时队列 | 从全表扫描过渡到真正的 RTOS 内核数据结构 |
| 实验 11 | 信号量 | 学会任务等待、唤醒、资源计数和 ISR 释放同步对象 |
| 实验 12 | 互斥锁 + 优先级继承 | 解决优先级反转，进入真实项目必须理解这一关 |
| 实验 13 | 消息队列 | 实现任务间数据传递，形成常用通信机制 |
| 实验 14 | 软件定时器 | 支持延时回调和周期性软件任务 |
| 实验 15 | 内存管理 | 优先实现固定块内存池，再学习简单 heap |
| 实验 16 | 中断与内核 API 规则 | 明确哪些 API 能在 ISR 中调用，哪些只能在任务中调用 |
| 实验 17 | 调试与可靠性 | HardFault 解析、断言、栈水位、运行时统计 |
| 实验 18 | FPU 上下文保存 | STM32F407 使用浮点时必须考虑 FPU 寄存器上下文 |
| 实验 19 | 移植层拆分 | 分离 kernel 通用代码和 Cortex-M port 代码 |
| 实验 20 | 真实小项目验证 | 用多个任务驱动 LED、按键、串口、传感器，验证 RTOS 可用性 |

---

## 五、核心变量与数据结构

```c
// 两个任务的栈空间
uint32_t taskA_stack[128];
uint32_t taskB_stack[128];

// 任务控制块 TCB
struct TCB {
    uint32_t *sp;  // 栈指针——这是最关键的成员
};
struct TCB taskA_tcb, taskB_tcb;

// 指向当前运行任务的 TCB
struct TCB *current_tcb;
```

---

## 六、单 LED 验证任务切换

即使只有一个 LED，也能验证两个任务在轮流控制同一个硬件：

```c
void TaskA(void) {   // 快闪：~200ms 周期
    while (1) {
        LED_ON();
        for (volatile int i = 0; i < 100000; i++);  // ~100ms
        LED_OFF();
        for (volatile int i = 0; i < 100000; i++);  // ~100ms
    }
}

void TaskB(void) {   // 慢闪：~800ms 周期
    while (1) {
        LED_ON();
        for (volatile int i = 0; i < 400000; i++);  // ~400ms
        LED_OFF();
        for (volatile int i = 0; i < 400000; i++);  // ~400ms
    }
}
```

**观察现象**：同一个 LED 在 200ms 快闪和 800ms 慢闪之间规律交替，证明两个任务在轮流控制同一个硬件——这就是 RTOS 的核心能力。

---

## 七、学习方法论

### 7.1 RTOS 最核心的三个文件

不管是 FreeRTOS 还是 RT-Thread，核心代码就三个：

| 文件 | 作用 |
|------|------|
| `task.c` | 任务创建、切换、调度 |
| `list.c` | 链表操作，内核的数据结构骨架 |
| `port.c` | 芯片相关汇编代码，保存和恢复寄存器，**内核的心脏** |

### 7.2 推荐资源

- **视频**：B 站搜索"野火 FreeRTOS 内核实现与应用"、"正点原子 FreeRTOS 源码剖析"
- **书籍**：
  - 《嵌入式实时操作系统——理论基础》（清华大学出版社）
  - 《FreeRTOS 内核实现与应用开发实战指南：基于 STM32》
  - 《Hands-On RTOS with Microcontrollers》（英文）

### 7.3 必备调试工具

- **串口打印**：最基本也最重要的调试手段
- **逻辑分析仪**：调试时序问题的神器
- **Git 版本控制**：每完成一个功能就 commit，随时回退，也记录成长轨迹

### 7.4 当前开发方式：先原型，后内核化

当前阶段可以继续把 RTOS 代码写在 `Core/Src/main.c` 和 `Core/Src/stm32f4xx_it.c` 中。这样做的目标是降低学习成本：先把任务切换、调度、延时、临界区这些机制亲手跑通，不急着一开始就设计复杂目录。

当完成 `MSP/PSP` 修正、任务状态机、idle task、`task_create()` 护栏之后，就可以逐步拆分为真正的内核结构：

```text
Core/Src/main.c              应用测试任务，只负责创建任务和验证功能
YUOS/kernel/yuos_task.c      任务创建、状态管理、调度器
YUOS/kernel/yuos_list.c      链表、就绪队列、延时队列
YUOS/kernel/yuos_sem.c       信号量
YUOS/kernel/yuos_mutex.c     互斥锁
YUOS/kernel/yuos_queue.c     消息队列
YUOS/port/cortex_m/port.c    临界区、触发 PendSV、启动第一个任务
YUOS/port/cortex_m/port_asm.s 上下文切换汇编
YUOS/include/*.h             对外 API
```

拆分原则：

- `kernel` 目录只放平台无关逻辑，不直接依赖 STM32 HAL。
- `port` 目录只放 Cortex-M 相关代码，例如 MSP/PSP、PendSV、SVC、PRIMASK、FPU 上下文。
- `main.c` 最终只作为示例应用，不再承载内核实现。
- 每完成一个机制再拆一个模块，避免为了目录漂亮而提前抽象。

### 7.5 自研边界与许可策略

YUOS 的目标是通过自己实现来学习 RTOS。可以学习 FreeRTOS、RT-Thread、Zephyr 等项目的思想和概念，但不要直接复制它们的源码、注释、文件结构或私有实现细节。

通用操作系统概念本身不构成抄袭，例如 TCB、优先级调度、PendSV 上下文切换、信号量、互斥锁、消息队列、软件定时器。这些是 RTOS 的公共技术基础，可以自己重新实现。

当前工程中的第三方组件需要保留原始许可：

| 组件 | 当前许可情况 | 使用建议 |
|------|--------------|----------|
| CMSIS | Apache-2.0 | 保留 `Drivers/CMSIS/LICENSE.txt` 和相关版权声明 |
| STM32 HAL Driver | BSD-3-Clause 兜底许可 | 保留 `Drivers/STM32F4xx_HAL_Driver/LICENSE.txt` 和 ST 版权声明 |
| STM32 Device Header | Apache-2.0 兜底许可 | 保留 `Drivers/CMSIS/Device/ST/STM32F4xx/LICENSE.txt` |
| YUOS 内核代码 | 自己决定 | 可闭源商用，也可选择 MIT/BSD-3-Clause/Apache-2.0 等许可证 |

后续如果希望 YUOS 用于商业项目，建议遵守以下边界：

- 自己写内核实现，不复制 GPL/LGPL 或许可证不明确的源码。
- 如果参考第三方文章或开源项目，只吸收原理，不搬运代码。
- 如果确实引入第三方代码片段，必须记录来源、许可证和修改内容。
- 将自研内核代码和厂商 HAL/CMSIS 代码分目录管理，方便后续替换平台或审查许可证。
- 项目正式发布前，为 YUOS 自研部分单独添加一个 `LICENSE` 文件，明确使用范围。

---

> 行动是治愈迷茫的唯一解药。当你看到同一颗 LED 在两个任务的控制下呈现出不同的闪烁节奏，你就亲手写出了自己操作系统的第一个内核。那一刻的顿悟，会比看任何理论书都来得深刻。

---

*最后更新：2026-05-03（实验 10 完成）*
