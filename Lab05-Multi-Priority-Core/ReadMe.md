## Description

This lab continues from `Lab04-IdleTask-Delay-Core`. Lab04 added `vTaskDelay()`, SysTick tick updates, and an idle task, but its scheduler still used fixed Task1/Task2 selection logic.

Lab05 adds a simplified multi-priority scheduler. Tasks now store their own priority in the TCB, task creation accepts a priority argument, and the scheduler selects the highest-priority ready task instead of switching between hard-coded task names.

This lab does not yet implement real delayed task lists. Delayed tasks are handled by clearing and setting priority bits in a ready-priority bitmap.

## What This Lab Adds After Lab04

- `uxPriority` inside each TCB.
- Priority argument in `xTaskCreateStatic()`.
- Automatic insertion of new tasks into the correct ready list.
- `uxTopReadyPriority` for tracking ready priorities.
- Priority recording and clearing macros.
- Highest-priority task selection.
- Optional port-optimized task selection using bit operations.
- Scheduler logic that no longer depends on hard-coded Task1/Task2 checks.

The main new ideas are:

```c
UBaseType_t uxPriority;
static volatile UBaseType_t uxTopReadyPriority;
```

## Main Idea

Ready tasks are still stored in priority-based ready lists:

```c
List_t pxReadyTasksLists[ configMAX_PRIORITIES ];
```

Each task belongs to one ready list according to its priority:

```text
pxReadyTasksLists[0] -> idle priority
pxReadyTasksLists[1] -> Task1
pxReadyTasksLists[2] -> Task2
```

The scheduler no longer asks:

```text
Should I run Task1 or Task2?
```

Instead, it asks:

```text
What is the highest priority that is currently ready?
```

Then it selects the next TCB from that priority's ready list.

## Ready Priority Bitmap

When `configUSE_PORT_OPTIMISED_TASK_SELECTION` is enabled, this lab uses a bitmap to record which priorities are ready.

Setting a priority ready:

```c
uxTopReadyPriority |= ( 1UL << uxPriority );
```

Clearing a priority:

```c
uxTopReadyPriority &= ~( 1UL << uxPriority );
```

Finding the highest ready priority:

```c
uxTopPriority = 31UL - __clz( uxTopReadyPriority );
```

This makes highest-priority selection faster than scanning every ready list from top to bottom.

## Key Flow

Task creation:

```text
xTaskCreateStatic()
  -> use user-provided TCB and stack
  -> initialize task name and list item
  -> store uxPriority in the TCB
  -> build initial stack frame
  -> prvAddNewTaskToReadyList()
      -> record the task priority as ready
      -> insert task into pxReadyTasksLists[ uxPriority ]
```

Scheduler startup:

```text
main()
  -> initialize ready lists
  -> create Task1 with priority 1
  -> create Task2 with priority 2
  -> vTaskStartScheduler()
      -> create idle task with priority 0
      -> select highest-priority ready task
      -> xPortStartScheduler()
```

Context switch decision:

```text
vTaskSwitchContext()
  -> taskSELECT_HIGHEST_PRIORITY_TASK()
  -> update pxCurrentTCB
```

Task delay:

```text
vTaskDelay( ticks )
  -> set current TCB xTicksToDelay
  -> clear the current task priority bit
  -> taskYIELD()
```

SysTick update:

```text
SysTick_Handler
  -> xTaskIncrementTick()
      -> decrease xTicksToDelay
      -> if delay reaches 0, set the task priority bit again
      -> portYIELD()
```

## Test Scenario

`User/main.c` creates two user tasks:

- `Task1_Entry()` toggles `flag1` and uses priority `1`.
- `Task2_Entry()` toggles `flag2` and uses priority `2`.

Because Task2 has the higher priority, the scheduler should choose Task2 whenever it is ready. When Task2 calls `vTaskDelay()`, its priority bit is cleared, allowing Task1 or the idle task to run.

## Current Limitations

This lab does not yet move delayed tasks into a real delayed list. A delayed task remains inside its ready list, but its priority bit is cleared from the ready-priority bitmap.

Because of this simplification, this lab currently assumes:

```text
one task per priority
```

If multiple tasks share the same priority, delaying one task would clear the whole priority bit and could incorrectly hide the other ready tasks at the same priority.

The focus of this lab is:

```text
TCB priority + ready-priority bitmap + highest-priority task selection
```

Real delayed lists and multiple tasks at the same priority can be added in a later lab.

## Demo

![Lab05 Demo](image.png)
