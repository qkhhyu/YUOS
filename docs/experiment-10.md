# 实验 10：就绪队列与延时队列

**日期**：2026-05-03
**状态**：✅ 已完成

---

## 目标

用双向链表替代 TCB 池遍历，scheduler 从 ready_list 头部 O(1) 选任务，SysTick 遍历 delay_list 递减延时。

## 实现

### TCB 链表指针

```c
struct yuos_tcb {
    // ... 原有字段 ...
    struct yuos_tcb *next;   // 下一个节点
    struct yuos_tcb *prev;   // 上一个节点
};
```

### 全局队列头

```c
struct yuos_tcb *ready_list;  // 就绪队列（按优先级排序）
struct yuos_tcb *delay_list;  // 延时队列（按 delay_ticks 升序）
```

### 链表操作

- `yuos_list_insert_by_priority(tcb, &ready_list)` — 按优先级插入
- `yuos_list_insert_by_delay(tcb, &delay_list)` — 按 delay_ticks 升序插入
- `yuos_list_remove(tcb, &list)` — 从指定队列移除

### 改造后的各函数

| 函数 | 变化 |
|------|------|
| `scheduler()` | 不再遍历 tcb_pool，直接取 `ready_list` 头 |
| `task_create()` | 创建后 `ready_list_insert(tcb)` |
| `task_sleep()` | 从 ready_list 移除 → 入 delay_list |
| `task_yield()` | 入 ready_list（同优先级末尾） |
| `SysTick_Handler` | 遍历 delay_list 递减 → 到期移回 ready_list |

### 队列与状态对应

```
READY 态 → 在 ready_list 中
BLOCKED 态 → 在 delay_list 中
RUNNING 态 → 不在任何队列
UNUSED 态 → 不在任何队列
```

## 关键理解

- **链表 = RTOS 的数据结构骨架**：FreeRTOS 的 `list.c` 也是这个用途
- **任务只在就绪/阻塞态时在队列里**：RUNNING 态不在队列，切换时由 scheduler 统一管理入队/出队
- **延时队列按 delay_ticks 排序**：队头是等待时间最短的任务，但不是增量编码（每次都要遍历递减）

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
