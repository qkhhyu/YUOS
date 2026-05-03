# 实验 7：sleep() 任务延时睡眠

**日期**：2026-04-30
**状态**：✅ 已完成

---

## 目标

任务主动睡眠指定 tick 数，延时期间让出 CPU，到期后自动被唤醒。

---

## 实现

### sleep() — 任务侧延时

```c
void sleep(uint32_t ticks)
{
    uint32_t primask = enter_critical();
    current_tcb->delay_ticks = ticks;
    current_tcb->state = TASK_BLOCKED;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;  // 触发 PendSV，让出 CPU
    exit_critical(primask);
}
```

**关键**：不直接调用 `scheduler()`——只设好延时标记 + 触发 PendSV，让 PendSV_Handler 做完整上下文切换。

### SysTick_Handler — 倒计时器

```c
struct yuos_tcb *tasks[] = {&taska_tcb, &taskb_tcb};
for (uint32_t i = 0; i < 2; i++) {
    if (tasks[i]->state == TASK_BLOCKED) {
        uint32_t primask = enter_critical();
        tasks[i]->delay_ticks--;
        if (tasks[i]->delay_ticks == 0) {
            tasks[i]->state = TASK_READY;
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;  // 唤醒时触发切换
        }
        exit_critical(primask);
    }
}
```

每 1ms SysTick 对所有 BLOCKED 任务递减计数，归零即唤醒。

### scheduler() 更新

```c
void scheduler(void) {
    // ... 遍历选 next ...
    current_tcb = next;
    current_tcb->state = TASK_RUNNING;  // 标记当前运行任务
}
```

---

## 遇到的困难

### 调用 scheduler() 但不真正切换

最初在 `sleep()` 里调用 `scheduler()`，但 `scheduler()` 只是改了 `current_tcb` 的值——CPU 的 SP、PC、寄存器都没变，返回后继续跑原任务。

**修复**：`sleep()` 不调 `scheduler()`，而是写 `SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk` 触发 PendSV。让 PendSV_Handler 做完整的硬件上下文切换。

### 两个任务同时阻塞——死锁

当两个任务都调用 `sleep()` 后，`scheduler()` 找不到就绪任务，退回当前任务。当前任务已设 `state = BLOCKED` 但仍继续运行——sleep 变成 no-op，陷入「sleep → 被自己调度回来 → sleep」的死循环。

**当前对策**：验证阶段让 taskB 不调用 sleep，用忙等保持始终就绪。下一步将加入空闲任务（idle task）彻底解决。

---

## 验证方法

taskA 快闪 5 次后 sleep 5 秒，taskB 持续慢闪（忙等，始终就绪）：

```
LED：快闪×5（~1s）→ 慢闪（~5s）→ 快闪×5 → 交替循环
```

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
