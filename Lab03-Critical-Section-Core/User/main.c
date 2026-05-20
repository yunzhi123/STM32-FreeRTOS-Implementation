#include "FreeRTOS.h"
#include "task.h"

volatile portCHAR flag1;
volatile portCHAR flag2;

extern List_t pxReadyTasksLists[ configMAX_PRIORITIES ];

TaskHandle_t Task1_Handle;
#define TASK1_STACK_SIZE 20
StackType_t Task1Stack[TASK1_STACK_SIZE];
TCB_t Task1TCB;

TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE 20
StackType_t Task2Stack[TASK2_STACK_SIZE];
TCB_t Task2TCB;

void delay(uint32_t count) {
	for (int i=count; i>=0; i--) {}
}

void Task1_Entry( void *p_arg ) {
	while (1) {
		flag1 = 1;
		delay(100);
		flag1 = 0;
		delay(100);
		taskYIELD();
	}
}

void Task2_Entry( void *p_arg ) {
	while (1) {
		flag2 = 1;
		delay(100);
		flag2 = 0;
		delay(100);
		taskYIELD();
	}
}
int main() {
	prvInitialiseTaskLists();
	
	Task1_Handle = xTaskCreateStatic((TaskFunction_t)Task1_Entry,
																	 (char*)"Task1",
																		(uint32_t)TASK1_STACK_SIZE,
																	  (void*) NULL,
																		(StackType_t*)Task1Stack,
																		(TCB_t*)&Task1TCB);
	vListInsertEnd(&(pxReadyTasksLists[1]), &( ((TCB_t *)(&Task1TCB))->xStateListItem ));

	Task2_Handle = xTaskCreateStatic( (TaskFunction_t)Task2_Entry, 
													(char *)"Task2",              
													(uint32_t)TASK2_STACK_SIZE ,  
													(void *) NULL,              
													(StackType_t *)Task2Stack,    
													(TCB_t *)&Task2TCB );         
	vListInsertEnd( &( pxReadyTasksLists[2] ), &( ((TCB_t *)(&Task2TCB))->xStateListItem ) );
    
	vTaskStartScheduler();
	while (1);
}
