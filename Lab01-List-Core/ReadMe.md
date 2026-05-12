# Discription

This folder documents the first core module in my journey of rebuilding the FreeRTOS kernel from scratch: the `list` implementation.

FreeRTOS uses this doubly linked list structure as the foundation for ready lists, delayed lists, event lists, and scheduler bookkeeping. The goal of this test is not just to implement a generic linked list, but to reproduce the behavior that later kernel components will depend on.

## What Is Implemented

- `ListItem_t`: A normal list item with a sort value, previous/next pointers, owner pointer, and container pointer.
- `MiniListItem_t`: A smaller list item used as the list end marker.
- `List_t`: The list object, including item count, current index, and the sentinel end marker.
- `vListInitialise()`: Initializes an empty list and its end marker.
- `vListInitialiseItem()`: Initializes a list item before insertion.
- `vListInsert()`: Inserts an item into the list in ascending `xItemValue` order.
- `vListInsertEnd()`: Inserts an item before the current list index.
- `uxListRemove()`: Removes an item from the list it belongs to.
- Common list macros such as `listSET_LIST_ITEM_VALUE()`, `listGET_HEAD_ENTRY()`, and `listCURRENT_LIST_LENGTH()`.



## Test Scenario

`User/main.c` creates one list and three list items:

```c
struct xLIST List_Test;

struct xLIST_ITEM List_item1;
struct xLIST_ITEM List_item2;
struct xLIST_ITEM List_item3;
```

The test flow is:

1. Initialize `List_Test` with `vListInitialise()`.
2. Initialize each list item with `vListInitialiseItem()`.
3. Assign item values:
   - `List_item1 = 1`
   - `List_item2 = 2`
   - `List_item3 = 3`
4. Insert the items in the order `2 -> 1 -> 3`.
5. Use the debugger to verify that the final linked-list order becomes `1 -> 2 -> 3`.
## Test Image
![alt text](image.png)
