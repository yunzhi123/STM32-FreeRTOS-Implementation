#ifndef INC_FREERTOS_H
#define INC_FREERTOS_H

#include "FreeRTOSConfig.h"
#include "projdefs.h"
#include "portable.h"
#include "list.h"

typedef struct tskTackControlBlock {
	volatile StackType_t *pxTopOfStack;
	ListItem_t xStateListItem;
	StackType_t *pxStack;
	char pcTaskName[configMAX_TASK_NAME_LEN];
} tskTCB;
typedef tskTCB TCB_t;

#endif 
