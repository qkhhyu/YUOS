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

```
第一阶段：构建最小内核骨架
  实验1 ──► 建立工程，点亮LED
  实验2 ──► 任务栈 + 手动切换（合作式调度）
  实验3 ──► SysTick 中断（抢占式调度）
  实验4 ──► 加入优先级

第二阶段：核心调度与任务管理
  实验5 ──► 完善 TCB（状态、优先级、延时字段）
  实验6 ──► task_create() 动态创建任务
  实验7 ──► 临界区保护（关/开中断）
  实验8 ──► task_delay() 任务延时睡眠

第三阶段：任务间通信
  实验9 ──► 信号量（P/V 操作）
  实验10 ─► 互斥锁（处理优先级反转）
  实验11 ─► 消息队列（FIFO 环形缓冲区）

第四阶段：完善与扩展
  实验12 ─► 时间片轮转 + 移植到另一款 MCU
```

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

---

### 实验 3：SysTick 中断（抢占式调度）

**状态**：🔲 待开始

**目标**：不管当前任务愿不愿意，时间一到硬件强行打断切换。

实现要点：
- 配置 SysTick 每 1ms 触发一次中断
- 在 ISR 里调用切换逻辑
- SysTick ISR 中判断 tick_count，达到阈值时标记需要切换，触发 PendSV

参考核心代码：

```c
void SysTick_Handler(void) {
    static int tick_count = 0;
    tick_count++;
    if (tick_count >= 500) {  // 每500ms切换一次
        tick_count = 0;
        current_tcb = (current_tcb == &taskA_tcb) ? &taskB_tcb : &taskA_tcb;
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;  // 触发PendSV
    }
}
```

PendSV 汇编骨架：

```asm
PendSV_Handler:
    // 1. 硬件已自动压栈 R0-R3,R12,LR,PC,xPSR
    // 2. 手动压栈 R4-R11
    push {r4-r11}
    // 3. 保存当前任务 SP → TCB
    ldr r0, =current_tcb
    ldr r1, [r0]
    str sp, [r1]
    // 4. 调用 C 调度函数
    bl scheduler
    // 5. 加载新任务 SP ← TCB   ★ 关键！切换栈指针
    ldr r0, =current_tcb
    ldr r1, [r0]
    ldr sp, [r1]
    // 6. 恢复新任务 R4-R11
    pop {r4-r11}
    // 7. 异常返回，硬件自动弹出 R0-R3,R12,LR,PC,xPSR
    bx lr
```

**里程碑**：RTOS 的灵魂——"抢占"诞生

---

### 实验 4：加入优先级

**状态**：🔲 待开始

**目标**：高优先级就绪时，强行剥夺低优先级任务的 CPU。

实现要点：
- TCB 添加"优先级"成员
- 切换逻辑加入优先级判断

**里程碑**：代码现在可以叫 RTOS 了

---

### 实验 5~8：核心调度与任务管理

**状态**：🔲 待开始

| 实验 | 内容 |
|------|------|
| 5 | 完善 TCB：添加状态、优先级、延时等字段 |
| 6 | 实现 `task_create()`：动态创建任务并初始化 TCB 和栈 |
| 7 | 实现临界区保护：关中断/开中断 |
| 8 | 实现 `task_delay()`：任务主动睡眠，延时期间让出 CPU |

---

### 实验 9~11：任务间通信

**状态**：🔲 待开始

| 实验 | 内容 |
|------|------|
| 9 | 信号量（P/V 操作） |
| 10 | 互斥锁（处理优先级反转问题） |
| 11 | 消息队列（FIFO 环形缓冲区） |

---

### 实验 12：时间片轮转 + 跨平台移植

**状态**：🔲 待开始

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

---

> 行动是治愈迷茫的唯一解药。当你看到同一颗 LED 在两个任务的控制下呈现出不同的闪烁节奏，你就亲手写出了自己操作系统的第一个内核。那一刻的顿悟，会比看任何理论书都来得深刻。

---

*最后更新：2026-04-29（实验2完成）*
