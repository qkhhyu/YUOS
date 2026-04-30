# 实验 3：SysTick 中断（抢占式调度）

**日期**：2026-04-30
**状态**：✅ 已完成

---

## 目标

不管当前任务愿不愿意，时间一到硬件强行打断切换。任务只管死循环，SysTick 定时触发 PendSV 完成上下文切换——任务完全无感知。

---

## 实现步骤

### 第一步：task_create 栈布局改造

实验2的栈布局匹配 `Switch()` 的手动 push/pop，实验3要匹配 PendSV 的异常帧格式：

```
高地址
  xPSR = 0x01000000  ← 硬件异常返回帧 (8 words，bx lr 时硬件自动弹出)
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

Cortex-M 进入异常时**硬件自动**压栈 R0-R3, R12, LR, PC, xPSR（8 个寄存器），PendSV_Handler 只需压 R4-R11（8 个）。总共 16 个 word。

### 第二步：SysTick_Handler — 定时触发 PendSV

```c
void SysTick_Handler(void)
{
    HAL_IncTick();  // 保留 HAL 库使用
    {
        static uint32_t tick_count = 0;
        tick_count++;
        if (tick_count >= 500) {
            tick_count = 0;
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;  // 触发 PendSV
        }
    }
}
```

- 每 1ms tick_count++，500 次即 ~500ms 切换一次
- 不在 SysTick ISR 里直接做切换，只"通知" PendSV

### 第三步：PendSV_Handler — 上下文切换核心

```asm
push {r4-r11}           ; ① 手动保存 R4-R11（硬件已保存 R0-R3,R12,LR,PC,xPSR）
ldr r0, =current_tcb
ldr r1, [r0]
str sp, [r1]            ; ② 当前 SP → current_tcb->sp
mov r4, lr              ; ③ 暂存 EXC_RETURN (bl 会覆盖 LR)
bl scheduler            ; ④ C 函数切换 current_tcb
mov lr, r4              ; ⑤ 恢复 EXC_RETURN
ldr r0, =current_tcb
ldr r1, [r0]
ldr sp, [r1]            ; ⑥ 加载新任务 SP
pop {r4-r11}            ; ⑦ 恢复 R4-R11
bx lr                   ; ⑧ 异常返回 → 硬件弹栈 → 跳转到新任务
```

### 第四步：SVC_Handler — 启动第一个任务

```asm
ldr r0, =current_tcb
ldr r1, [r0]
ldr sp, [r1]            ; SP = 第一个任务的栈指针
pop {r4-r11}            ; 恢复 R4-R11
bx lr                   ; 异常返回 → 硬件弹出 R0-R3,PC → 任务启动
```

`start_first_task()` 触发 `SVC 0` 进入 Handler 模式，bx lr 才有异常返回语义。

### 第五步：main() 初始化

```c
task_create(&taska_tcb, taska_stack, STACK_SIZE, taska);
task_create(&taskb_tcb, taskb_stack, STACK_SIZE, taskb);
NVIC_SetPriority(PendSV_IRQn, 15);  // PendSV 设为最低优先级
current_tcb = &taska_tcb;
start_first_task();  // SVC → 跳转到 taska
```

---

## PendSV 切换全过程

```
时间轴
─────→

① taskA 在 Thread 模式正常执行
   SP → taska_stack

② SysTick 计数到 500，置位 PendSV 挂起位
   PendSV 不立即执行——优先级最低(15)，等所有高优先级中断先处理完

③ PendSV 开始响应
   硬件自动压栈到 taska_stack（8 words）：
   ┌──────────┐
   │  xPSR    │ ← 硬件自动保存
   │  PC      │   （taskA 被打算时正在执行的指令地址）
   │  LR      │
   │  R12     │
   │  R3/R2   │
   │  R1/R0   │
   │  R11     │ ← push {r4-r11} 手动保存
   │  ...     │
   │  R4      │
   └──────────┘
   SP 现在指向 R4（栈底）

④ str sp, [r1]  →  taska_tcb.sp = SP
   把 taskA 的"断点位置"写入它的身份证（TCB）

⑤ bl scheduler  →  current_tcb = &taskb_tcb
   调度器把全局指针指向 taskB 的 TCB

⑥ ldr sp, [r1]  →  SP = taskb_tcb.sp
   SP 切到 taskb_stack —— 切换发生！
   ┌──────────┐
   │  xPSR    │
   │  PC      │ ← taskB 上次被打算时保存的断点
   │  ...     │
   │  R4      │
   └──────────┘

⑦ pop {r4-r11}  →  CPU 的 R4~R11 恢复为 taskB 的值

⑧ bx lr  →  硬件弹栈 R0-R3,R12,LR,PC,xPSR
             PC 变成 taskB 的断点 → CPU 无缝继续跑 taskB
```

**核心就两个赋值**：
- `str sp, [r1]` — 当前 SP → 旧任务 TCB.sp（保存断点）
- `ldr sp, [r1]` — 新任务 TCB.sp → SP 寄存器（加载断点）

SP 寄存器一变，后续所有 push/pop/bx 全作用在新任务的栈上——任务就切过去了。

---

## 遇到的 Bug

### Bug 1：`bl scheduler` 覆盖 LR

**症状**：LED 亮一下→全灭→系统挂死

**根因**：`bl` 把返回地址写入 LR，覆盖了异常入口时的 EXC_RETURN 值（`0xFFFFFFF9`）。`bx lr` 时 LR 不是异常返回标记，CPU 跳非法地址进入 HardFault。

### Bug 2：`ldr r2, [lr]` 尝试暂存 LR

**症状**：HardFault

**根因**：LR = `0xFFFFFFF9` 是特殊编码值，不是合法内存地址。`ldr` 试图从该地址读数据，CPU 直接 HardFault。

### Bug 3：用 R2 暂存 LR

**根因**：R0-R3 是 caller-saved 寄存器，`bl scheduler` 的 C 代码可随意修改它们。`scheduler()` 返回后 R2 大概率已被污染。

**最终修复**：

```asm
mov r4, lr              ; 借 R4 暂存（R4 是 callee-saved，C 函数保证不破坏）
bl scheduler
mov lr, r4              ; 归还 EXC_RETURN
```

---

## 关键理解

- **抢占 vs 合作**：实验 2 的任务必须主动调用 Switch() 让出 CPU；实验 3 的任务只管死循环，SysTick 硬件定时打断
- **PendSV 的意义**：如果直接在 SysTick ISR 里做切换，会阻塞其他同/低优先级中断。PendSV 是最低优先级，等所有 ISR 处理完再切换
- **异常返回机制**：bx lr 在 Handler 模式下识别 EXC_RETURN 值（`0xFFFFFFF9`），通知硬件自动弹出异常帧——所以 PendSV 只需要手动管理 R4-R11
- **SVC 启动**：main() 在 Thread 模式，bx lr 不是异常返回。SVC 进入 Handler 模式后，bx lr 才有异常返回语义
- **AAPCS 调用约定**：R0-R3 caller-saved（C 函数可破坏），R4-R11 callee-saved（C 函数必须保护）。汇编和 C 混编时必须遵守

---

## 验证方法

1. 编译下载到 STM32F407VE 开发板
2. 观察 LED（PB2）：自动在快闪（~200ms）和慢闪（~800ms）之间交替
3. 任务代码里没有 Switch() 调用——切换完全由硬件驱动

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
