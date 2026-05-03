# 补强 2：收紧任务状态机

**日期**：2026-04-30
**状态**：✅ 已完成

---

## 目标

任务状态转换必须严格、完整。调度器只选 `READY` 任务，切走的任务从 `RUNNING` 正确回退到 `READY`。

## 问题

之前 `scheduler()` 用 `delay_ticks == 0` 而非 `state == TASK_READY` 筛选任务，且被抢占的任务保持 `RUNNING` 不变——状态机不闭环。

## 改动

`scheduler()` 改为用 `state` 字段而非 `delay_ticks` 筛选：

```c
void scheduler(void)
{
    struct yuos_tcb *next = current_tcb;

    for (int i = 0; i < YUOS_MAX_TASKS; i++) {
        if (tcb_pool[i].state == TASK_UNUSED) continue;
        if (tcb_pool[i].state == TASK_READY) {                        // 只选 READY
            if (next->state != TASK_READY || tcb_pool[i].priority < next->priority) {
                next = &tcb_pool[i];
            }
        }
    }

    // 旧任务：被抢占 → READY（sleep 已设 BLOCKED，这里不干预）
    if (current_tcb != next && current_tcb->state == TASK_RUNNING) {
        current_tcb->state = TASK_READY;
    }

    current_tcb = next;
    current_tcb->state = TASK_RUNNING;
}
```

## 状态机完整流转

```
TASK_UNUSED ──→ TASK_READY ──→ TASK_RUNNING ──→ TASK_READY   (被抢占)
                 (创建)         (调度选中)       (切走)
                                            ──→ TASK_BLOCKED  (主动 sleep)
                                            
TASK_BLOCKED ──→ TASK_READY                                    (延时到期)
                 (SysTick 唤醒)
```

三个状态转换点各司其职：

| 转换 | 在哪里 | 条件 |
|------|--------|------|
| READY → RUNNING | scheduler() | 被选中 |
| RUNNING → READY | scheduler() | 被抢占（current != next 且 state == RUNNING） |
| RUNNING → BLOCKED | sleep() | 主动延时 |
| BLOCKED → READY | SysTick_Handler | delay_ticks 归零 |
| UNUSED → READY | task_create() | 从池分配 |

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
