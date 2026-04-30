# 实验 6：临界区保护（关/开中断）

**日期**：2026-04-30
**状态**：✅ 已完成

---

## 目标

保护内核关键数据结构不被中断破坏。当 `scheduler()` 正在修改 `current_tcb` 时，SysTick 中断不应在此期间访问 TCB。

---

## 为什么需要临界区

以 `scheduler()` 为例：

```
scheduler() 读取 current_tcb 时
  → SysTick 中断发生
    → SysTick_Handler 修改 delay_ticks
  → 返回后 current_tcb 可能已过时

或者更糟：
  current_tcb 写入一半时被中断
    → 写入操作被分成多条指令（非原子）
    → ISR 读到半写状态的数据
```

临界区通过暂时关中断来保证操作的原子性。

---

## 实现

```c
uint32_t enter_critical(void)
{
    uint32_t primask = __get_PRIMASK();  // 读取当前中断状态
    __disable_irq();                      // 关全局中断
    return primask;                       // 返回旧状态，交给调用者
}

void exit_critical(uint32_t primask)
{
    __set_PRIMASK(primask);               // 恢复到旧状态
}
```

**调度器中使用**：

```c
void scheduler(void)
{
    // ... 遍历选任务 ...
    uint32_t primask = enter_critical();
    current_tcb = next;          // 受保护的修改
    exit_critical(primask);
}
```

---

## 方案对比

| 方案 | 嵌套支持 | 全局变量 | ISR 安全 |
|------|----------|----------|----------|
| 计数嵌套 (`static int irq_count`) | ✅ 需手动计数 | 有 | ❌ ISR 内调用后会错误开中断 |
| 保存/恢复 PRIMASK（最终采用） | ✅ 自动 | 无 | ✅ 恢复到调用前的状态 |

---

## 关键理解

- **保护范围要最小**：临界区只保护 `current_tcb = next` 一行，for 遍历在外面——如果在整个 for 循环上包临界区，中断被关的时间太长
- **PRIMASK vs BASEPRI**：PRIMASK 关所有中断（简单粗暴），BASEPRI 可以只屏蔽低优先级中断而保留高优先级——后续可升级
- **无可见副作用 = 正确**：临界区是"防御性"代码，当前无反例触发，行为和之前一样

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
