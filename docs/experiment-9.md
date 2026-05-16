# 实验 9：yield() + 同优先级时间片轮转

**日期**：2026-05-03
**状态**：✅ 已完成

---

## 目标

实现 `task_yield()` 主动让出 CPU，以及 SysTick 驱动的同优先级时间片自动轮转。

## 实现

### TCB 新增字段

```c
struct yuos_tcb {
    // ...
    uint32_t last_tick;    // 上次被调度时的系统 tick
    uint32_t time_slice;   // 时间片长度（ticks），0 表示不启用
};
```

### 全局 tick 计数器

```c
struct yuos_kernel {
    uint32_t ticks;  // 每 1ms +1
};
struct yuos_kernel yuos_os = {0};
```

### task_yield()

```c
void task_yield(void) {
    current_tcb->state = TASK_READY;
    yuos_ready_list_insert(current_tcb);
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}
```

### 时间片自动轮转（SysTick_Handler）

```c
yuos_os.ticks++;
if (current_tcb->time_slice) {
    current_tcb->time_slice--;
    if (current_tcb->time_slice == 0) {
        current_tcb->state = TASK_READY;
        current_tcb->time_slice = YUOS_TIME_SLICE;
        yuos_ready_list_insert(current_tcb);
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}
```

### scheduler 同优先级轮转

```c
void scheduler(void) {
    struct yuos_tcb *next = ready_list;
    // ...
    current_tcb->last_tick = yuos_os.ticks;
    current_tcb->time_slice = YUOS_TIME_SLICE;
}
```

同优先级通过 `last_tick` 选最久没跑的 + 时间片到后自动重新入就绪队列实现公平轮转。

## 关键理解

- **yield vs sleep**：yield 是 READY→READY（主动礼让），sleep 是 RUNNING→BLOCKED（等待时间）
- **时间片 = SysTick 自动调 yield**：硬件替你主动让出 CPU
- **last_tick 用途**：同优先级多个就绪任务时，选等最久的那个

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
