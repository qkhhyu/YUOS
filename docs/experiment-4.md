# 实验 4：加入优先级

**日期**：2026-04-30
**状态**：✅ 已完成

---

## 目标

高优先级任务就绪时，强行剥夺低优先级任务的 CPU。这是 RTOS 区别于裸机合作式多任务的标志性能力。

---

## 实现步骤

### 第一步：TCB 新增 priority 字段

```c
struct tcb {
    uint32_t *sp;        /* 栈指针 —— 切换任务就是切换这个 */
    uint32_t priority;   /* 任务优先级，数值越小优先级越高 */
};
```

### 第二步：task_create() 新增 priority 参数

```c
void task_create(struct tcb *tcb, uint32_t *stack, int stack_size,
                 uint32_t priority, void (*task_func)(void))
{
    // ... 栈初始化不变（硬件异常帧 + R4-R11） ...
    tcb->priority = priority;
}
```

### 第三步：scheduler() 改为优先级调度

```c
/*
 * scheduler — 优先级调度
 * 当前为两任务简化版：直接比较 taskA/taskB 的优先级，
 * 谁优先级高（数值小）就选谁。
 * 局限性：硬编码全局变量比较，仅支持两个任务。
 */
void scheduler(void)
{
    current_tcb = taska_tcb.priority < taskb_tcb.priority
                  ? &taska_tcb : &taskb_tcb;
}
```

### 第四步：main() 中指定优先级

```c
task_create(&taska_tcb, taska_stack, STACK_SIZE, 1, taska);  // 优先级 1（高）
task_create(&taskb_tcb, taskb_stack, STACK_SIZE, 2, taskb);  // 优先级 2（低）
```

---

## 观察现象

LED 一直快闪——taskA 优先级高（1 < 2），始终被调度选中；taskB 优先级低，永远饿死（饥饿）。

这验证了优先级调度正确生效：当高优先级任务就绪时，低优先级任务永远得不到 CPU。

---

## 当前局限

- `scheduler()` 硬编码了 `taska_tcb` / `taskb_tcb` 全局变量比较，仅支持两个任务
- 还没有 `task_delay()`，低优先级任务永远饿死——这是正确行为，但限制了实际使用
- 后续实验将通过链表管理就绪任务、加入延时字段来解决

---

## 关键理解

- **优先级 = 调度决策的唯一依据**：当前阶段，scheduler 只看优先级，不看其他因素
- **饥饿是正确的**：没有延时机制时，高优先级任务会永远霸占 CPU——这正是优先级调度的预期行为
- **数值小的优先级高**：这是一个约定，后续与 FreeRTOS 等主流 RTOS 保持一致

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
