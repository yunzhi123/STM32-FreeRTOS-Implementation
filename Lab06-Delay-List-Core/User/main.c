#include "FreeRTOS.h"
#include "task.h"

volatile portCHAR flag1;
volatile portCHAR flag2;

extern List_t pxReadyTasksLists[ configMAX_PRIORITIES ];

TaskHandle_t Task1_Handle;
#define TASK1_STACK_SIZE 128
StackType_t Task1Stack[TASK1_STACK_SIZE];
TCB_t Task1TCB;

TaskHandle_t Task2_Handle;
#define TASK2_STACK_SIZE 128
StackType_t Task2Stack[TASK2_STACK_SIZE];
TCB_t Task2TCB;

// idle task
#define configMINIMAL_STACK_SIZE 128
StackType_t IdleTaskStack[configMINIMAL_STACK_SIZE];
TCB_t IdleTaskTCB;

void delay(uint32_t count) {
	for (int i=count; i>=0; i--) {}
}

void Task1_Entry( void *p_arg ) {
	while (1) {
		flag1 = 1;
		vTaskDelay(2);
		flag1 = 0;
		vTaskDelay(2);
		taskYIELD();
	}
}

void Task2_Entry( void *p_arg ) {
	while (1) {
		flag2 = 1;
		vTaskDelay(2);
		flag2 = 0;
		vTaskDelay(2);
		taskYIELD();
	}
}

void vApplicationGetIdleTaskMemory(TCB_t** pxIdleTaskTCBBuffer,
								  StackType_t** pxIdleTaskStackBuffer,
								  uint32_t* ulIdleTaskStackSize) {
	*pxIdleTaskTCBBuffer = &IdleTaskTCB;
	*pxIdleTaskStackBuffer = IdleTaskStack;
	*ulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

int main() {
	prvInitialiseTaskLists();
	
	Task1_Handle = xTaskCreateStatic((TaskFunction_t)Task1_Entry,
									(char*)"Task1",
									(uint32_t)TASK1_STACK_SIZE,
									(void*) NULL,
									(UBaseType_t) 1U,
									(StackType_t*)Task1Stack,
									(TCB_t*)&Task1TCB);

	Task2_Handle = xTaskCreateStatic( (TaskFunction_t)Task2_Entry, 
									(char *)"Task2",              
									(uint32_t)TASK2_STACK_SIZE ,  
									(void *) NULL,    
									(UBaseType_t) 2U,          
									(StackType_t *)Task2Stack,    
									(TCB_t *)&Task2TCB );         
    
	vTaskStartScheduler();
	while (1);
}
