#ifndef PORTMACRO_H 
#define PORTMACRO_H 
#include "stdint.h" 
#include "stddef.h" 
	
	
#define portCHAR       char 
#define portFLOAT       float 
#define portDOUBLE       double 
#define portLONG       long 
#define portSHORT       short 
#define portSTACK_TYPE uint32_t 
#define portBASE_TYPE  long 

typedef portSTACK_TYPE StackType_t; 
typedef long BaseType_t; 
typedef unsigned long UBaseType_t; 
	
#if( configUSE_16_BIT_TICKS == 1)
	typedef uint16_t TickType_t; 
	#define portMAX_DELAY ( TickType_t ) 0xffff 
#else 
	typedef uint32_t TickType_t; 
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL 
#endif 

#define portNVIC_INT_CTRL_REG		( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT		( 1UL << 28UL )
#define portSY_FULL_READ_WRITE		( 15 )

#define portYIELD()																\
{																				\
	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;								\
	__dsb( portSY_FULL_READ_WRITE );											\
	__isb( portSY_FULL_READ_WRITE );											\
}

/* Critical Section Handler*/
extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);

#define portDISABLE_INTERRUPTS() vPortRaiseBASEPRI()
#define portENABLE_INTERRUPT() vPortSetBASEPRI(0)

#define portENTER_CRITICAL() vPortEnterCritical()
#define portEXIT_CRITICAL() vPortExitCritical()

#define portSET_INTERRUPT_MASK_FROM_ISR() ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x) vPortSetBASEPRI(x)

#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )

#define portINLINE __inline

#ifndef portFORCE_INLINE
	#define portFORCE_INLINE __forceinline
#endif

static portFORCE_INLINE void vPortSetBASEPRI(uint32_t ulBASEPRI) {
	__asm {
		msr basepri, ulBASEPRI
	}
}

/*----------------------------------------------*/
static portFORCE_INLINE void vPortRaiseBASEPRI(void) {
	uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY; // 191
	__asm {
		msr basepri, ulNewBASEPRI
		dsb
		isb
	}
}

static portFORCE_INLINE void vPortClearBASEPRIFromISR(void) {
	__asm {
		msr basepri, #0
	}
}

/*----------------------------------------------*/
static portFORCE_INLINE uint32_t ulPortRaiseBASEPRI(void) {
	uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
	__asm {
		mrs ulReturn, basepri
		msr basepri, ulNewBASEPRI
		dsb
		isb 
	}
	return ulReturn;
}

#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
	#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#endif

#define portRECORD_READY_PRIORITY(uxPriority, uxReadyPriorities)\
	(uxReadyPriorities) |= (1UL << uxPriority)

#define portRESET_READY_PRIORITY(uxPriority, uxReadyPriorities)\
	(uxReadyPriorities) &= ~(1UL << uxPriority);

#define portGET_HIGHEST_PRIORITY(uxTopPriority, uxTopReadyPriorities)\
	uxTopPriority = (31UL - (uint32_t)__clz((uxTopReadyPriorities)))
#endif
