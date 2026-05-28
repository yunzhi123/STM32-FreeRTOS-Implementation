#include "FreeRTOS.h"
#include "task.h"

TCB_t* volatile pxCurrentTCB = NULL;

List_t pxReadyTasksLists[ configMAX_PRIORITIES ];

static TaskHandle_t xIdleTaskHandle	= NULL;

static volatile UBaseType_t uxCurrentNumberOfTasks = (UBaseType_t)0U;
static volatile TickType_t xTickCount = (TickType_t) 0U;

static portTASK_FUNCTION( prvIdleTask, pvParameters );

static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t* volatile pxDelayedTaskList;
static List_t* volatile pxOverflowDelayedTaskList;

static volatile TickType_t xNextTaskUnblockTime = (TickType_t) 0U;
static volatile BaseType_t xNumOfOverflows = (BaseType_t) 0U;

static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,
									const char * const pcName,             
									const uint32_t ulStackDepth,
									void * const pvParameters,
									UBaseType_t uxpriority,
									TaskHandle_t * const pxCreatedTask,     
									TCB_t *pxNewTCB );

static volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;
									
static void prvAddNewTaskToReadyList(TCB_t* pxNewTCB);
static void prvAddCurrentTaskToDelayList(TickType_t xTicksToWait);
static void prvResetNextTaskUnblockTime(void);

// macro define

#define taskSWITCH_DELAYED_LISTS() {\
	List_t* pxTemp;\
	pxTemp = pxDelayedTaskList;\
	pxDelayedTaskList = pxOverflowDelayedTaskList;\
	pxOverflowDelayedTaskList = pxTemp;\
	xNumOfOverflows ++;\
	prvResetNextTaskUnblockTime();\
}

// general method to find maximum priority task
#if (configUSE_PORT_OPTIMISED_TASK_SELECTION == 0) 
	#define taskRECORD_READY_PRIORITY(uxPriority) {\
		if ((uxPriority) > uxTopReadyPriority) {\
			uxTopReadyPriority = (uxPriority);\
		}\
	}

	#define taskSELECT_HIGHEST_PRIORITY_TASK() {\
		UBaseType_t uxTopPriority = uxTopReadyPriority;\
		while (listLIST_IS_EMPTY(&(pxReadyTasksLists[uxTopPriority]))) uxTopPriority --;\
		\
		\
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) );\
		uxTopReadyPriority = uxTopPriority;\
	}
	#define taskRESET_READY_PRIORITY( uxPriority )

#else 
	#define taskRECORD_READY_PRIORITY(uxPriority) portRECORD_READY_PRIORITY(uxPriority, uxTopReadyPriority)
	#define taskSELECT_HIGHEST_PRIORITY_TASK() {\
		UBaseType_t uxTopPriority;\
		portGET_HIGHEST_PRIORITY(uxTopPriority, uxTopReadyPriority);\
		listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &(pxReadyTasksLists[uxTopPriority]));\
	}
	#define taskRESET_READY_PRIORITY( uxPriority ) {\
		if (listCURRENT_LIST_LENGTH(&(pxReadyTasksLists[(uxPriority)])) == (UBaseType_t 0U)) {\
			portRESET_READY_PRIORITY(uxPriority, uxTopReadyPriority)\
		}\
	}
#endif

#define prvAddTaskToReadyList(pxTCB)\
	taskRECORD_READY_PRIORITY((pxTCB)->uxPriority);\
	vListInsertEnd(&(pxReadyTasksLists[(pxTCB)->uxPriority]), &((pxTCB)->xStateListItem));

// static create task
#if(configSUPPORT_STATIC_ALLOCATION == 1)
															
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,
								const char *const pcName,
								const uint32_t ulStackDepth,
								void* const pvParameters,
								UBaseType_t uxPriority,
								StackType_t* const puxStackBuffer,
								TCB_t* const pxTaskBuffer) {
	TCB_t* pxNewTCB;
	TaskHandle_t xReturn;
																	
	if((pxTaskBuffer != NULL) && (puxStackBuffer != NULL)) {
		pxNewTCB = (TCB_t*)pxTaskBuffer;
		pxNewTCB->pxStack = (StackType_t*)puxStackBuffer;
		
		prvInitialiseNewTask(pxTaskCode,
							pcName,
							ulStackDepth,
							pvParameters,
							uxPriority,
							&xReturn,
							pxNewTCB);

		prvAddNewTaskToReadyList(pxNewTCB);
	} else {
		xReturn = NULL;
	}
	return xReturn;
}

#endif

static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,
									const char * const pcName,             
									const uint32_t ulStackDepth,
									void * const pvParameters,   
									UBaseType_t uxPriority,       
									TaskHandle_t * const pxCreatedTask,     
									TCB_t *pxNewTCB ) {
	StackType_t* pxTopOfStack;
	UBaseType_t x;
	
	pxTopOfStack = pxNewTCB->pxStack + (ulStackDepth-(uint32_t)1);
	pxTopOfStack = (StackType_t*)(((uint32_t)pxTopOfStack) & (~((uint32_t)(0x0007))));
	
	for (x = (UBaseType_t)0; x < (UBaseType_t)configMAX_TASK_NAME_LEN; x++) {
		pxNewTCB->pcTaskName[x] = pcName[x];
		if (pcName[x] == 0x00) break;
	}
	
	pxNewTCB->pcTaskName[configMAX_TASK_NAME_LEN-1] = '\0';
	
	vListInitialiseItem(&(pxNewTCB->xStateListItem));
	listSET_LIST_ITEM_OWNER(&(pxNewTCB->xStateListItem), pxNewTCB);
	if (uxPriority >= (UBaseType_t)configMAX_PRIORITIES) {
		uxPriority = (UBaseType_t)configMAX_PRIORITIES - 1U;
	}
	pxNewTCB->uxPriority = uxPriority;
	pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters );   
	
	if ((void*)pxCreatedTask != NULL) {
		*pxCreatedTask = (TaskHandle_t)pxNewTCB;
	}
}

void prvInitialiseTaskLists() {
	UBaseType_t uxPriority;
	
	for (uxPriority = (UBaseType_t)0U; uxPriority<(UBaseType_t)configMAX_PRIORITIES; uxPriority++) {
		vListInitialise(&(pxReadyTasksLists[uxPriority]));
	}

	vListInitialise(&xDelayedTaskList1);
	vListInitialise(&xDelayedTaskList2);

	pxDelayedTaskList = &xDelayedTaskList1;
	pxOverflowDelayedTaskList = &xDelayedTaskList2;
}
		
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;
extern TCB_t IdleTaskTCB;
void vApplicationGetIdleTaskMemory(TCB_t** pxIdleTaskTCBBuffer,
								  StackType_t** pxIdleTaskStackBuffer,
								  uint32_t *ulIdleTaskStackSize);
void vTaskStartScheduler() {
	TCB_t* pxIdleTaskTCBBuffer = NULL;
	StackType_t* pxIdleTaskStackBuffer = NULL;
	uint32_t ulIdleTaskStackSize;

	vApplicationGetIdleTaskMemory(&pxIdleTaskTCBBuffer,
								  &pxIdleTaskStackBuffer,
								  &ulIdleTaskStackSize);
	
	xIdleTaskHandle = xTaskCreateStatic((TaskFunction_t)prvIdleTask,
					  (char*)"IDLE",
					  (uint32_t)ulIdleTaskStackSize,
					  (void*)NULL,
					  (UBaseType_t) tskIDLE_PRIORITY,
					  (StackType_t*)pxIdleTaskStackBuffer,
					  (TCB_t*)pxIdleTaskTCBBuffer);
	
	xNextTaskUnblockTime = portMAX_DELAY;
	xTickCount = (TickType_t) 0U;
	(void)xIdleTaskHandle;
	

	taskSELECT_HIGHEST_PRIORITY_TASK();

	if (xPortStartScheduler() != pdFALSE) {

	}
}

static portTASK_FUNCTION( prvIdleTask, pvParameters )
{
	( void ) pvParameters;
    
    for(;;);
}

void vTaskSwitchContext() {
	taskSELECT_HIGHEST_PRIORITY_TASK();
}

void vTaskDelay(const TickType_t xTicksToDelay) {
	prvAddCurrentTaskToDelayList(xTicksToDelay);
	taskYIELD();
}

void xTaskIncrementTick(void) {
	TCB_t* pxTCB = NULL;
	TickType_t xItemValue;

	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;

	if (xConstTickCount == (TickType_t) 0U) {
		taskSWITCH_DELAYED_LISTS();
	}

	if (xConstTickCount >= xNextTaskUnblockTime) {
		while (1) {
			if (listLIST_IS_EMPTY(pxDelayedTaskList) != pdFALSE) {
				xNextTaskUnblockTime = portMAX_DELAY;
				break;
			} else {
				pxTCB = (TCB_t*) listGET_OWNER_OF_HEAD_ENTRY(pxDelayedTaskList);
				xItemValue = listGET_LIST_ITEM_VALUE(&(pxTCB->xStateListItem));
  
				if (xConstTickCount < xItemValue) {
					xNextTaskUnblockTime = xItemValue;
					break;
				}
				(void) uxListRemove(&(pxTCB->xStateListItem));
				prvAddTaskToReadyList(pxTCB);
			}
		}
	}

	portYIELD();
}

static void prvAddNewTaskToReadyList(TCB_t* pxNewTCB) {
	taskENTER_CRITICAL();

	uxCurrentNumberOfTasks ++;
	if (pxCurrentTCB == NULL) {
		pxCurrentTCB = pxNewTCB;
		if (uxCurrentNumberOfTasks == 1) prvInitialiseTaskLists();
	}  else {
		if (pxCurrentTCB->uxPriority < pxNewTCB->uxPriority) pxCurrentTCB = pxNewTCB;
	}
	prvAddTaskToReadyList(pxNewTCB);

	taskEXIT_CRITICAL();
}

static void prvAddCurrentTaskToDelayList(TickType_t xTicksToWait) {
	TickType_t xTimeToWake;
	const TickType_t xConstTickCount = xTickCount;

	if (uxListRemove(&(pxCurrentTCB->xStateListItem)) == 0) {
		portRESET_READY_PRIORITY(pxCurrentTCB->uxPriority, uxTopReadyPriority);
	}
	xTimeToWake = xTickCount + xTicksToWait;

	listSET_LIST_ITEM_VALUE(&(pxCurrentTCB->xStateListItem), xTimeToWake);

	if (xTimeToWake < xConstTickCount) {
		vListInsert(pxOverflowDelayedTaskList, &(pxCurrentTCB->xStateListItem));
	} else {
		vListInsert(pxDelayedTaskList, &(pxCurrentTCB->xStateListItem));

		if (xTimeToWake < xNextTaskUnblockTime) {
			xNextTaskUnblockTime = xTimeToWake;
		}
	}
}

static void prvResetNextTaskUnblockTime(void) {
	TCB_t* pxTCB;

	if (listLIST_IS_EMPTY(pxDelayedTaskList) != pdFALSE) {
		xNextTaskUnblockTime = portMAX_DELAY;
	} else {
		(pxTCB) = (TCB_t*) listGET_OWNER_OF_HEAD_ENTRY(pxDelayedTaskList);
		xNextTaskUnblockTime = listGET_LIST_ITEM_VALUE(&(pxTCB->xStateListItem));
	}
}
