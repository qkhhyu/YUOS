# 实验 5：完善 TCB

**日期**：2026-04-30
**状态**：✅ 已完成

---

## 目标

扩展 TCB，加入任务状态和延时字段，为后续 `task_delay()` 做准备。同时将 `scheduler()` 从硬编码比较改为数组遍历结构。

---

## 实现步骤

### 第一步：TCB 新增 state 和 delay_ticks

```c
enum {
    TASK_READY   = 0,
    TASK_RUNNING = 1,
    TASK_BLOCKED = 2,
};

struct yuos_tcb {          // 改名：避免与 HAL 库冲突
    uint32_t *sp;          // 栈指针
    uint32_t priority;     // 优先级，数值越小越高
    uint32_t state;        // 任务状态
    uint32_t delay_ticks;  // 剩余延时 tick 数，0 = 无延时
};
```

### 第二步：task_create() 初始化新字段

```c
void task_create(struct yuos_tcb *tcb, ..., uint32_t priority, void (*task_func)(void))
{
    // ... 栈初始化不变 ...
    tcb->priority    = priority;
    tcb->state       = TASK_READY;
    tcb->delay_ticks = 0;
}
```

### 第三步：scheduler() 改为数组遍历

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

---

## 观察现象

LED 一直快闪——与实验 4 行为一致，验证重构无回归。

---

## 关键理解

- **数据驱动 vs 代码驱动**：用数组 + for 替代 if-else 枚举——加新任务只需往数组加元素，调度逻辑不动
- **字段分阶段启用**：`state` 和 `delay_ticks` 已定义但当前调度只用 `delay_ticks`，`state` 在后续实验中逐步参与调度
- **命名空间保护**：`struct tcb → struct yuos_tcb`，为自己的内核代码留出干净的命名空间

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
