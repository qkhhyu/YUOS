# 补强 1：MSP/PSP 分离

**日期**：2026-04-30
**状态**：✅ 已完成

---

## 目标

让内核使用 MSP、任务使用 PSP，符合 Cortex-M RTOS 的标准栈模型。

## 问题

之前的实现中，taskA、taskB 和 PendSV_Handler 都用 MSP——内核和任务共用同一根栈。任务栈溢出会直接冲掉内核数据，系统崩溃无法追溯。

## 改动

### SVC_Handler — 启动第一个任务

```asm
ldr r0, =current_tcb
ldr r1, [r0]
ldr r0, [r1]           ; r0 = 任务栈指针 (tcb->sp)
ldmia r0!, {r4-r11}    ; 从任务栈弹出 R4-R11
msr psp, r0            ; PSP = 任务栈当前位置
mov r0, #2             ; CONTROL[1]=1(PSP) CONTROL[0]=0(特权)
msr CONTROL, r0
isb
bx lr                  ; 异常返回 → 硬件从 PSP 弹异常帧 → 任务启动
```

### PendSV_Handler — 上下文切换

```asm
mrs r0, psp            ; 读当前任务 PSP
stmdb r0!, {r4-r11}    ; R4-R11 存到 PSP
ldr r1, =current_tcb
ldr r2, [r1]
str r0, [r2]           ; PSP → tcb->sp
mov r4, lr
bl scheduler
mov lr, r4
ldr r1, =current_tcb
ldr r2, [r1]
ldr r0, [r2]           ; 新任务栈指针
ldmia r0!, {r4-r11}    ; 从新任务栈恢复 R4-R11
msr psp, r0            ; PSP = 新位置
isb
bx lr                  ; 异常返回
```

### 关键指令变化

| 旧（全用 MSP） | 新（MSP/PSP 分离） | 原因 |
|---|---|---|
| `push {r4-r11}` | `mrs r0, psp` + `stmdb r0!, {r4-r11}` | Handler 模式下 push 操作 MSP；任务数据在 PSP 上 |
| `pop {r4-r11}` | `ldmia r0!, {r4-r11}` + `msr psp, r0` | 同理，pop 操作 MSP |
| 无 | `msr CONTROL, r0` | 显式切到 PSP 模式 |

## 遇到的 Bug

### CONTROL=3 导致任务非特权 → HardFault

**症状**：LED 不亮

**根因**：`mov r0, #3` 设了 `CONTROL[0]=1`（非特权模式）。任务在非特权下访问 GPIO 外设寄存器触发 MemManage Fault。

**修复**：`mov r0, #2`——bit1=PSP, bit0=特权。所有主流 RTOS 默认任务跑特权 + PSP。

## 关键理解

- **MSP/PSP 物理隔离**：内核栈独享 MSP，不受任务栈溢出影响
- **CONTROL 寄存器的两个 bit**：bit1 选栈（SP），bit0 选权限（特权/非特权）——两件事独立
- **非特权模式**需要配套的 SVC 系统调用机制，是进阶内容

---

*详细总览见 [DEVELOPMENT.md](../DEVELOPMENT.md)*
