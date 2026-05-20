# Lab03 - Critical Section Core

## Description

This lab continues from `Lab02-Task-Core`. Lab02 introduced the basic task model, static task creation, task stacks, SVC startup, PendSV context switching, and cooperative switching through `taskYIELD()`.

Lab03 adds the first critical section support. The goal is to understand how FreeRTOS protects kernel data structures from being interrupted while they are being modified.

This lab does not yet apply critical sections throughout the scheduler. Instead, it focuses on building the core API and port-layer mechanism that later labs can use around ready lists, task state changes, and scheduler operations.

## What This Lab Adds After Lab02

Lab03 adds:

- Task-level critical section macros.
- ISR-safe critical section macros.
- BASEPRI-based interrupt masking for ARM Cortex-M3.
- A critical nesting counter for task context.
- Helper functions for raising and restoring the interrupt mask.

The important new user-facing APIs are:

```c
taskENTER_CRITICAL();
taskEXIT_CRITICAL();

taskENTER_CRITICAL_FROM_ISR();
taskEXIT_CRITICAL_FROM_ISR( x );
```

## Main Idea

A critical section is a region of code that must not be interrupted by kernel-aware interrupts. In an RTOS, this is important when modifying shared kernel objects such as ready lists, task control blocks, or scheduler state.

On ARM Cortex-M, this lab uses the `BASEPRI` register instead of globally disabling all interrupts. `BASEPRI` masks interrupts at or below a configured priority level, while still allowing higher-priority interrupts to run.

The interrupt mask level is configured in `FreeRTOSConfig.h`:

```c
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 191
```

When entering a critical section, the port layer writes this value into `BASEPRI`.

## Task Context Flow

Task-level critical sections start from the normal task API:

```c
taskENTER_CRITICAL();
```

The call flow is:

```text
taskENTER_CRITICAL()
  -> portENTER_CRITICAL()
  -> vPortEnterCritical()
  -> portDISABLE_INTERRUPTS()
  -> vPortRaiseBASEPRI()
  -> write configMAX_SYSCALL_INTERRUPT_PRIORITY to BASEPRI
```

Leaving the critical section follows the reverse path:

```text
taskEXIT_CRITICAL()
  -> portEXIT_CRITICAL()
  -> vPortExitCritical()
  -> decrease uxCriticalNesting
  -> if nesting becomes 0, clear BASEPRI
```

The task-side implementation uses:

```c
static UBaseType_t uxCriticalNesting;
```

This counter allows nested critical sections. For example:

```c
taskENTER_CRITICAL();  /* nesting = 1 */
taskENTER_CRITICAL();  /* nesting = 2 */

taskEXIT_CRITICAL();   /* nesting = 1, interrupts still masked */
taskEXIT_CRITICAL();   /* nesting = 0, interrupts can be unmasked */
```

This prevents an inner function from accidentally re-enabling interrupts while an outer critical section is still active.

## ISR Context Flow

Interrupt handlers use a different API:

```c
uint32_t ulSavedMask;

ulSavedMask = taskENTER_CRITICAL_FROM_ISR();

/* ISR critical section */

taskEXIT_CRITICAL_FROM_ISR( ulSavedMask );
```

The enter flow is:

```text
taskENTER_CRITICAL_FROM_ISR()
  -> portSET_INTERRUPT_MASK_FROM_ISR()
  -> ulPortRaiseBASEPRI()
  -> save current BASEPRI
  -> write configMAX_SYSCALL_INTERRUPT_PRIORITY to BASEPRI
  -> return the old BASEPRI value
```

The exit flow is:

```text
taskEXIT_CRITICAL_FROM_ISR( oldValue )
  -> portCLEAR_INTERRUPT_MASK_FROM_ISR( oldValue )
  -> vPortSetBASEPRI( oldValue )
```

ISR critical sections restore the previous interrupt mask instead of using the task nesting counter. This is important because an ISR may interrupt a task that was already inside a critical section. The ISR must restore the exact interrupt mask state that existed before it changed `BASEPRI`.

## Task vs ISR Critical Sections

Task context and ISR context use different designs:

```text
Task context:
  Uses uxCriticalNesting.
  Supports nested critical sections.
  Clears BASEPRI only when the outermost critical section exits.

ISR context:
  Saves the old BASEPRI value.
  Raises BASEPRI during the ISR critical section.
  Restores the saved BASEPRI value when leaving.
```

In short, task code manages critical section depth, while ISR code preserves and restores the interrupt mask state it interrupted.

## Important Port-Layer Functions

The main implementation is split between `task.h`, `portmacro.h`, and `port.c`.

`task.h` provides the user-facing macros:

```c
#define taskENTER_CRITICAL()           portENTER_CRITICAL()
#define taskEXIT_CRITICAL()            portEXIT_CRITICAL()

#define taskENTER_CRITICAL_FROM_ISR()  portSET_INTERRUPT_MASK_FROM_ISR()
#define taskEXIT_CRITICAL_FROM_ISR(x)  portCLEAR_INTERRUPT_MASK_FROM_ISR(x)
```

`portmacro.h` maps those macros to Cortex-M-specific operations:

```c
#define portDISABLE_INTERRUPTS()       vPortRaiseBASEPRI()
#define portENABLE_INTERRUPT()         vPortSetBASEPRI( 0 )

#define portENTER_CRITICAL()           vPortEnterCritical()
#define portEXIT_CRITICAL()            vPortExitCritical()

#define portSET_INTERRUPT_MASK_FROM_ISR()      ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)   vPortSetBASEPRI( x )
```

`port.c` implements the task-level nesting logic:

```c
void vPortEnterCritical( void );
void vPortExitCritical( void );
```