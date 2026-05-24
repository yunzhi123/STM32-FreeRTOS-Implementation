
## Description

This lab continues from `Lab01-List-Core`. After building the FreeRTOS-style list module, Lab02 uses it to create a minimal task system.

The goal is to understand how a task is represented by a TCB, how static task creation works, how tasks are linked into ready lists, and how Cortex-M3 starts and switches task contexts.

This is still not a complete scheduler. It is a small cooperative context-switching demo using SVC, PendSV, and `taskYIELD()`.

## What This Lab Adds After Lab01

- `TCB_t`, the task control block.
- Static task creation with `xTaskCreateStatic()`.
- Per-task stack buffers.
- `xStateListItem` inside each TCB.
- Ready lists with `pxReadyTasksLists[]`.
- Initial task stack frame setup in `pxPortInitialiseStack()`.
- First task startup through SVC.
- Context switching through PendSV.
- Cooperative switching through `taskYIELD()`.

## Main Idea

Each task owns a TCB:

```c
typedef struct tskTackControlBlock {
    volatile StackType_t *pxTopOfStack;
    ListItem_t xStateListItem;
    StackType_t *pxStack;
    char pcTaskName[ configMAX_TASK_NAME_LEN ];
} tskTCB;
```

The most important field is `pxTopOfStack`. PendSV saves the current CPU context to the current task stack, stores the stack pointer in this field, then loads the next task's saved stack pointer.

The ready lists are grouped by priority:

```c
List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
```

In this lab, tasks are manually inserted into ready lists from `User/main.c`.

## Key Flow

Task creation:

```text
xTaskCreateStatic()
  -> use user-provided TCB and stack
  -> initialize task name and list item
  -> set list item owner to the TCB
  -> build initial stack frame
  -> save top-of-stack into TCB
```

First task startup:

```text
vTaskStartScheduler()
  -> pxCurrentTCB = Task1
  -> xPortStartScheduler()
  -> prvStartFirstTask()
  -> SVC_Handler
  -> restore Task1 context
```

Cooperative context switch:

```text
Task1
  -> taskYIELD()
  -> PendSV_Handler
  -> vTaskSwitchContext()
  -> Task2
```

## Test Scenario

`User/main.c` creates two tasks:

- `Task1_Entry()` toggles `flag1`.
- `Task2_Entry()` toggles `flag2`.

Both tasks call `taskYIELD()`, so PendSV switches execution between their separate stacks.

The expected Logic Analyzer result is that `flag1` and `flag2` toggle alternately.

## Current Limitations

This lab does not yet implement SysTick, `vTaskDelay()`, idle task creation, automatic ready-list management, or full priority-based scheduling.

The focus is:

```text
TCB + stack frame + SVC startup + PendSV context switch
```

## Demo

![Lab02 Demo](image.png)
