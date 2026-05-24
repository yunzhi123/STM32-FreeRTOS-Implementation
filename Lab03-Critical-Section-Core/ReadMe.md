
## Description

This lab continues from `Lab02-Task-Core`. Lab02 already has static task creation, SVC startup, PendSV context switching, and cooperative switching through `taskYIELD()`.

Lab03 adds the first critical section mechanism. The goal is to understand how FreeRTOS prevents kernel data structures from being interrupted while they are being modified.

This lab focuses on the core API and Cortex-M3 port-layer mechanism. It does not yet apply critical sections everywhere in the scheduler.

## What This Lab Adds After Lab02

- Task-level critical section macros.
- ISR-safe critical section macros.
- `BASEPRI` interrupt masking on ARM Cortex-M3.
- A nesting counter for task critical sections.
- Helper functions for raising, clearing, saving, and restoring the interrupt mask.

The main APIs are:

```c
taskENTER_CRITICAL();
taskEXIT_CRITICAL();

taskENTER_CRITICAL_FROM_ISR();
taskEXIT_CRITICAL_FROM_ISR( x );
```

## Main Idea

A critical section is a short region of code that should not be interrupted by kernel-aware interrupts. In a kernel, this is important when modifying shared objects such as ready lists, TCBs, or scheduler state.

This lab uses `BASEPRI` instead of disabling every interrupt globally. `BASEPRI` masks interrupts at or below `configMAX_SYSCALL_INTERRUPT_PRIORITY`, while still allowing higher-priority interrupts to run.

```c
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 191
```

## Key Flow

Task critical section:

```text
taskENTER_CRITICAL()
  -> portENTER_CRITICAL()
  -> vPortEnterCritical()
  -> vPortRaiseBASEPRI()
  -> uxCriticalNesting++
```

```text
taskEXIT_CRITICAL()
  -> portEXIT_CRITICAL()
  -> vPortExitCritical()
  -> uxCriticalNesting--
  -> if nesting is 0, clear BASEPRI
```

ISR critical section:

```text
taskENTER_CRITICAL_FROM_ISR()
  -> ulPortRaiseBASEPRI()
  -> return old BASEPRI value
```

```text
taskEXIT_CRITICAL_FROM_ISR( oldValue )
  -> vPortSetBASEPRI( oldValue )
```

The difference is that task critical sections use `uxCriticalNesting`, while ISR critical sections save and restore the previous `BASEPRI` value directly.

## Main Files

- `task.h`: exposes the critical section macros.
- `portmacro.h`: maps the macros to Cortex-M3 port functions.
- `port.c`: implements `vPortEnterCritical()` and `vPortExitCritical()`.

## Current Limitations

This lab only builds the critical section foundation. Later labs can use it around scheduler operations, ready-list changes, task state updates, and interrupt-driven kernel behavior.
