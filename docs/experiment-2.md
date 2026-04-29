# 实验 2：任务栈 + 手动切换（合作式调度）

**日期**：2026-04-29
**状态**：✅ 已完成

---

## 目标

用两个死循环函数（TaskA、TaskB），各自主动调用 `Switch()` 函数，实现从 A 跳到 B，再从 B 跳回 A。LED 交替呈现两种不同的闪烁节奏——任务**主动**让出 CPU，这是"合作式多任务"的核心。

---

## 实现步骤

### 第一步：基于实验 1 的工程

清空 main.c 中 while(1) 的点灯逻辑，保留 HAL 初始化和时钟配置。所有 RTOS 代码写在 main.c 中（后续再拆文件）。

### 第二步：定义任务栈和 TCB

在 `/* USER CODE BEGIN PV */` 中：

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

### 第三步：写任务函数

两个死循环，各自用忙等延时实现不同的闪烁周期，循环末尾调用 `Switch()` 主动让出 CPU：

```c
void taska(void) {
    while (1) {
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
        for (volatile int i = 0; i < 500000; i++);   // 亮 ~100ms
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
        for (volatile int i = 0; i < 500000; i++);   // 灭 ~100ms
        Switch();  // 主动交出 CPU
    }
}

void taskb(void) {
    while (1) {
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
        for (volatile int i = 0; i < 2000000; i++);  // 亮 ~400ms
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
        for (volatile int i = 0; i < 2000000; i++);  // 灭 ~400ms
        Switch();  // 主动交出 CPU
    }
}
```

### 第四步：task_create() — 初始化任务栈

任务栈需要被"伪造"成看起来像是被 `push {r4-r11, lr}` 保存过的样子，这样 `Switch()` 中 `pop {r4-r11, pc}` 就能"恢复"到任务函数的入口地址。

```c
void task_create(struct tcb *tcb, uint32_t *stack, int stack_size, void (*task_func)(void))
{
    uint32_t *sp = &stack[stack_size - 1];  // 栈顶 = 数组末尾

    // 栈布局（高地址→低地址）：PC, R11, R10, ..., R4
    // Switch() 中通过 pop {r4-r11, pc} 恢复
    *(--sp) = (uint32_t)task_func;  // PC —— 首次运行时从这里开始
    for (int i = 11; i >= 4; i--) {
        *(--sp) = i;                 // R11~R4 初始值任意
    }

    tcb->sp = sp;
}
```

**栈布局示意**（以 taskb 为例，假设 taskb_stack 地址从低到高）：

```
taskb_stack[118]: 4   ← sp 指向这里 (R4)
taskb_stack[119]: 5   (R5)
taskb_stack[120]: 6   (R6)
taskb_stack[121]: 7   (R7)
taskb_stack[122]: 8   (R8)
taskb_stack[123]: 9   (R9)
taskb_stack[124]: 10  (R10)
taskb_stack[125]: 11  (R11)
taskb_stack[126]: taskb地址 (PC)
taskb_stack[127]: 未使用
```

### 第五步：Switch() — 汇编上下文切换

这里是实验 2 的灵魂。`__attribute__((naked))` 阻止编译器生成函数序言/尾声，所有栈操作由我们手动控制。

```c
__attribute__((naked)) void Switch(void)
{
    __asm volatile (
        "push {r4-r11, lr}       \n"  // ① 保存 R4-R11 和返回地址
        "ldr r0, =current_tcb    \n"
        "ldr r1, [r0]            \n"
        "str sp, [r1]            \n"  // ② 当前 SP → current_tcb->sp

        "ldr r2, =taska_tcb      \n"
        "cmp r1, r2              \n"  // ③ 当前是 taskA 还是 taskB？
        "beq to_taskB            \n"  //    是 taskA → 切换到 taskB
        "ldr r0, =taska_tcb      \n"  //    是 taskB → 切换到 taskA
        "b restore               \n"
        "to_taskB:               \n"
        "ldr r0, =taskb_tcb      \n"
        "restore:                \n"
        "ldr r1, =current_tcb    \n"
        "str r0, [r1]            \n"  // ④ current_tcb = 新任务
        "ldr sp, [r0]            \n"  // ⑤ SP = 新任务栈指针  ★关键★

        "pop {r4-r11, pc}        \n"  // ⑥ 恢复 R4-R11，PC 跳转到新任务
    );
}
```

**六步解析**：

| 步骤 | 指令 | 作用 |
|------|------|------|
| ① | `push {r4-r11, lr}` | 保存"被调用者保存"寄存器（8个）+ 返回地址，共压栈 9 个字 |
| ② | `str sp, [r1]` | 把当前 SP 写入 `current_tcb->sp`，记录离开位置 |
| ③ | `cmp r1, r2` | 判断当前是谁在跑，决定切换到谁 |
| ④ | `str r0, [r1]` | 更新 `current_tcb` 指向新任务 |
| ⑤ | `ldr sp, [r0]` | **切换栈指针**——从此 CPU 在新任务的栈上运行 |
| ⑥ | `pop {r4-r11, pc}` | 恢复新任务曾保存的寄存器和 PC，直接跳到它的执行点 |

### 第六步：main() 启动第一个任务

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    task_create(&taska_tcb, taska_stack, STACK_SIZE, taska);
    task_create(&taskb_tcb, taskb_stack, STACK_SIZE, taskb);
    current_tcb = &taska_tcb;

    taska();  // 直接调用 taskA，它跑起来后通过 Switch() 切换到 taskB

    while (1) { /* 永远不会执行到这里 */ }
}
```

**为什么可以 `taska()` 直接调用？**

taskA 通过直接的函数调用跑在 MSP（主栈）上。它第一次调用 `Switch()` 时，汇编会把当前的 MSP 位置保存到 `taska_tcb.sp`，然后加载 `taskb_tcb.sp`（指向 taskb_stack），弹出 PC=taskb 地址，跳到 taskB。从此两个任务互相切换，main() 的 while(1) 永远不会被执行。

---

## 遇到的 Bug

### Bug 1：`str r0, [r0]` 破坏了 taskB 的栈指针

原始代码在 `restore:` 标签后有一行 `str r0, [r0]`：

```asm
restore:
    "str r0, [r0]            \n"   ← BUG!
    "ldr r1, =current_tcb    \n"
    "str r0, [r1]            \n"
    "ldr sp, [r0]            \n"
```

此时 `r0 = &taskb_tcb`，`str r0, [r0]` 把 `&taskb_tcb` 这个地址值写进了 `taskb_tcb.sp`（TCB 结构体的第一个成员就是 `sp`），直接覆盖了 task_create 设置好的 taskb_stack 指针。之后 `ldr sp, [r0]` 加载的就是垃圾值，taskB 永远无法正常执行。

**修复**：删除 `str r0, [r0]`。

### Bug 2：没保存 LR，`bx lr` 只能回到原调用者

原始代码：

```asm
"push {r4-r11}            \n"   ← 缺 lr
...
"pop {r4-r11}             \n"   ← 只弹出 8 个寄存器
"bx lr                    \n"   ← 用的是旧 LR，回到旧的返回地址
```

`push` 没保存 LR，`bx lr` 时 LR 仍是 Switch 被调用时的值（指向调用者 taskA 中 Switch 的下一行）。所以 switch 出去后永远回到 taskA，taskB 永远得不到执行。

**修复**：
- `push {r4-r11, lr}` —— 保存 9 个寄存器（含返回地址）
- `pop {r4-r11, pc}` —— pop 9 个寄存器，PC 直接跳转到新任务的执行点

---

## 执行流示意

```
main()
  │
  ├─ task_create(&taska_tcb, ...)   // 初始化 taskA 栈
  ├─ task_create(&taskb_tcb, ...)   // 初始化 taskB 栈
  ├─ current_tcb = &taska_tcb
  └─ taska()                        // 直接调用，跑在 MSP 上
       │
       │  while(1): LED快闪 → Switch()
       │                              │
       │    push {r4-r11,lr}          │ MSP ← 保存 taskA 离开位置
       │    保存 SP → taska_tcb.sp    │
       │    SP ← taskb_tcb.sp         │ 切换到 taskB 的栈
       │    pop {r4-r11,pc}           │ PC = taskb 函数地址 → 跳转！
       │                              │
       │                         taskb()  // 现在跑在 taskb_stack 上
       │                              │
       │                              │  while(1): LED慢闪 → Switch()
       │                              │
       │    pop {r4-r11,pc} ←─────────│  push {r4-r11,lr}
       │    PC = taskA Switch后地址    │  保存 SP → taskb_tcb.sp
       │    ← 跳回 taskA              │  SP ← taska_tcb.sp
       │                              │
       │  while(1): LED快闪 → Switch()│
       │  ...交替循环...              │
```

---

## 验证方法

1. 编译下载到 STM32F407VE 开发板
2. 观察 LED（PB2）：
   - 一会儿以 ~200ms 周期快闪（taskA）
   - 一会儿以 ~800ms 周期慢闪（taskB）
   - 交替往复
3. 如果只以一种频率闪烁：说明切换没发生，用调试器在 Switch() 处设断点单步跟踪

---

## 关键理解

- **栈指针切换 = 任务切换**：整个切换的核心就是 `ldr sp, [r0]` 这一条指令——SP 一变，后续的 pop/bx 全都作用在新任务的栈上了
- **合作式的含义**：任务不调用 Switch()，别人永远跑不了。如果一个任务死循环不调用 Switch()，整个系统就卡死
- **naked 属性**：不带 `__attribute__((naked))`，编译器会在函数开头加 `push {lr}` 等指令，破坏我们手动构造的栈布局
- **下一步**：实验 3 将用 SysTick 定时器实现抢占，任务不再需要"主动礼让"

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
