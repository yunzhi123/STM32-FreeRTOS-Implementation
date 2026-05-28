## Description

This lab continues from `Lab05-Multi-Priority-Core`. Lab05 added priority-based ready lists and highest-priority task selection, but delayed tasks were still handled by clearing and setting ready-priority bits.

Lab06 replaces that simplified delay mechanism with real delayed task lists. When a task calls `vTaskDelay()`, it is removed from the ready list and inserted into a delayed list according to its wake-up tick. SysTick increments the global tick count and moves expired tasks back to the ready list.

This is closer to the real FreeRTOS scheduling model: a task that is delayed is no longer considered ready.

## What This Lab Adds After Lab05

- Global tick counter `xTickCount`.
- Two delayed task lists:
  - `xDelayedTaskList1`
  - `xDelayedTaskList2`
- Delayed list pointers:
  - `pxDelayedTaskList`
  - `pxOverflowDelayedTaskList`
- `xNextTaskUnblockTime` for tracking the earliest wake-up tick.
- `xNumOfOverflows` for counting tick counter overflows.
- Real movement of tasks between ready lists and delayed lists.
- Wake-up time stored in each task's `xStateListItem.xItemValue`.
- Tick overflow handling by switching delayed list pointers.

The main new objects are:

```c
static volatile TickType_t xTickCount;

static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t * volatile pxDelayedTaskList;
static List_t * volatile pxOverflowDelayedTaskList;

static volatile TickType_t xNextTaskUnblockTime;
static volatile BaseType_t xNumOfOverflows;
```

## Main Idea

Lab05 kept delayed tasks inside the ready list and used `xTicksToDelay` as a countdown value.

Lab06 changes the model:

```text
ready task    -> stored in pxReadyTasksLists[ priority ]
delayed task  -> stored in pxDelayedTaskList or pxOverflowDelayedTaskList
```

When a task delays, the kernel calculates an absolute wake-up time:

```c
xTimeToWake = xTickCount + xTicksToWait;
```

That value is stored inside the task's state list item:

```c
listSET_LIST_ITEM_VALUE( &( pxCurrentTCB->xStateListItem ), xTimeToWake );
```

The delayed list is sorted by this item value, so the head of the list is always the task that should wake first.

## Delayed Lists

The active delayed list stores tasks whose wake-up time is still within the current tick-count cycle:

```c
vListInsert( pxDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
```

If the wake-up time wraps around the tick counter, the task is inserted into the overflow delayed list:

```c
vListInsert( pxOverflowDelayedTaskList, &( pxCurrentTCB->xStateListItem ) );
```

For example, with a 32-bit tick counter:

```text
xTickCount = 0xFFFFFFFE
vTaskDelay( 5 )
xTimeToWake = 0x00000003
```

The wake-up time looks smaller than the current tick, but it is actually after tick overflow. That task must wait in the overflow delayed list until `xTickCount` wraps back to `0`.

When overflow happens, the two delayed list pointers are swapped:

```c
taskSWITCH_DELAYED_LISTS();
```

## xNextTaskUnblockTime

`xNextTaskUnblockTime` records the earliest wake-up tick among delayed tasks.

This avoids scanning the delayed list on every tick. If the current tick has not reached `xNextTaskUnblockTime`, there is no delayed task to wake.

```text
xTickCount < xNextTaskUnblockTime
  -> no delayed task is ready

xTickCount >= xNextTaskUnblockTime
  -> check delayed list and unblock expired tasks
```

If the delayed list becomes empty, `xNextTaskUnblockTime` is set to `portMAX_DELAY`.

## Key Flow

Scheduler startup:

```text
main()
  -> initialize ready lists and delayed lists
  -> create Task1 with priority 1
  -> create Task2 with priority 2
  -> vTaskStartScheduler()
      -> create idle task with priority 0
      -> set xNextTaskUnblockTime to portMAX_DELAY
      -> reset xTickCount to 0
      -> select highest-priority ready task
      -> xPortStartScheduler()
```

Task delay:

```text
Task1_Entry() or Task2_Entry()
  -> vTaskDelay( ticks )
      -> remove current task from its ready list
      -> calculate xTimeToWake = xTickCount + ticks
      -> store xTimeToWake in xStateListItem.xItemValue
      -> insert task into delayed list or overflow delayed list
      -> update xNextTaskUnblockTime if needed
      -> taskYIELD()
```

SysTick update:

```text
SysTick_Handler
  -> xTaskIncrementTick()
      -> increment xTickCount
      -> if xTickCount wrapped to 0, switch delayed lists
      -> if xTickCount reached xNextTaskUnblockTime:
          -> remove expired tasks from delayed list
          -> insert them back into ready lists
          -> update xNextTaskUnblockTime
      -> portYIELD()
```

Context switch decision:

```text
vTaskSwitchContext()
  -> taskSELECT_HIGHEST_PRIORITY_TASK()
  -> choose the highest-priority task currently in a ready list
```

## Test Scenario

`User/main.c` creates two user tasks:

- `Task1_Entry()` toggles `flag1` and uses priority `1`.
- `Task2_Entry()` toggles `flag2` and uses priority `2`.

Because Task2 has the higher priority, the scheduler chooses Task2 whenever it is ready. When Task2 calls `vTaskDelay( 2 )`, it is removed from the ready list and placed into the delayed list. Task1 can then run until Task2's wake-up tick arrives.

When the wake-up tick arrives, SysTick moves Task2 back into its ready list. Since Task2 has higher priority, the next context switch selects Task2 again.

## Current Limitations

This lab implements the core delayed-list idea, but it is still a simplified FreeRTOS kernel.

Current simplifications include:

- `vTaskDelay()` does not yet wrap the ready-list and delayed-list updates in a critical section.
- `xTaskIncrementTick()` always requests PendSV through `portYIELD()`, even if no higher-priority task was unblocked.
- The idle task is minimal and does not perform cleanup or low-power behavior.
- The old `xTicksToDelay` field still exists in the TCB, but Lab06 delay handling now uses the task list item's wake-up value instead.

The focus of this lab is:

```text
xTickCount + delayed lists + wake-up time ordering + moving tasks back to ready lists
```
