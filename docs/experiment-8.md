# 实验 8：task_create() 动态创建任务

**日期**：2026-04-30
**状态**：✅ 已完成

---

## 目标

去掉硬编码的全局 TCB 和栈变量，改用任务池机制——`task_create()` 自动从池中分配 TCB 和栈，返回指针。

---

## 实现

### 任务池

```c
struct yuos_tcb tcb_pool[YUOS_MAX_TASKS];        // TCB 池
uint32_t stack_pool[YUOS_MAX_TASKS][STACK_SIZE];  // 栈池
```

两者通过同一索引 `slot` 对应：`tcb_pool[slot]` 的栈是 `stack_pool[slot][]`。

### task_create() — 从池分配

```c
struct yuos_tcb *task_create(int stack_size, uint32_t priority,
                              void (*task_func)(void))
{
    struct yuos_tcb *tcb = NULL;
    int slot = -1;

    // 扫描空闲槽
    for (int i = 0; i < YUOS_MAX_TASKS; i++) {
        if (tcb_pool[i].state == TASK_UNUSED) {
            tcb = &tcb_pool[i];
            slot = i;
            break;
        }
    }

    if (tcb == NULL) return NULL;  // 池满

    // 从 slot 对应的栈初始化
    uint32_t *sp = &stack_pool[slot][stack_size - 1];
    // ... 硬件异常帧 + R4-R11 初始化 ...

    tcb->sp = sp;
    tcb->priority = priority;
    tcb->state = TASK_READY;
    tcb->delay_ticks = 0;
    return tcb;
}
```

### 调用方变化

```c
// 旧：全局 TCB 地址传入
struct yuos_tcb taska_tcb;                     // 全局变量
task_create(&taska_tcb, taska_stack, 128, 1, taska);

// 新：返回指针
struct yuos_tcb *taska_tcb;                    // 指针
taska_tcb = task_create(128, 1, taska);
```

---

## 遇到的 Bug

### BSS 零初始化与状态枚举冲突

**症状**：LED 不亮，系统崩溃

**根因**：

```c
// 原来的枚举顺序
enum { TASK_READY = 0, TASK_RUNNING = 1, TASK_BLOCKED = 2, TASK_UNUSED = 3 };
```

全局数组 `tcb_pool[]` 在 BSS 段，默认初始化为 0。`state` 域全是 0 → 全部被识别为 `TASK_READY`，无一空闲槽。`task_create()` 找不到空位，返回野指针。

**修复**：`TASK_UNUSED = 0`，利用 BSS 零值代表空闲：

```c
enum { TASK_UNUSED = 0, TASK_READY = 1, TASK_RUNNING = 2, TASK_BLOCKED = 3 };
```

几乎所有 RTOS 都把空闲/未使用态设为零值，这是嵌入式编程的常见约定。

### 循环中 tcb 未赋值

找到空闲槽后只设置了 `sp` 和 `slot`，漏了 `tcb = &tcb_pool[i]`。后续 `tcb->sp = sp` 操作野指针。

**修复**：循环内补上 `tcb = &tcb_pool[i];`。

---

## 关键理解

- **任务池 = TCB 数组 + 栈数组，索引绑定**：`slot` 是纽带，TCB 和栈通过同一索引关联
- **零值 = 空闲**：利用 C 全局变量自动零初始化，避免显式循环初始化池
- **从 push 变量到 pull 指针**：用返回值代替传入地址——调用方只需保存指针，不再关心 TCB 是静态还是动态分配的

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
