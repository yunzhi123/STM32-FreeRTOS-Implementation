#include "FreeRTOS.h"
#include "task.h"
#include "ARMCM3.h"

#define portINITIAL_XPSR (0x01000000)
#define portSTART_ADDRESS_MASK ((StackType_t)0xfffffffeUL)


#define portNVIC_SYSPRI2_REG (*((volatile uint32_t*)0xe000ed20))

#define portNVIC_PENDSV_PRI (((uint32_t) configKERNEL_INTERRUPT_PRIORITY)<<16UL);
#define portNVIC_SYSTICK_PRI (((uint32_t) configKERNEL_INTERRUPT_PRIORITY ) << 24UL );


void prvStartFirstTask( void );
void vPortSVCHandler( void );
void xPortPendSVHandler( void );


static void prvTaskExitError() {
	while (1);
}

StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, TaskFunction_t pxCode, void* pvParameters) {
	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_XPSR;
	pxTopOfStack--;
	*pxTopOfStack = ((StackType_t)pxCode) & portSTART_ADDRESS_MASK;
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t) prvTaskExitError;
	pxTopOfStack -= 5; // R12 R3 R2 R1 default 0
	*pxTopOfStack = (StackType_t) pvParameters;
	pxTopOfStack -= 8;
	return pxTopOfStack;
}

BaseType_t xPortStartScheduler() {
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

	prvStartFirstTask();

	return 0;
}

__asm void prvStartFirstTask() {
	PRESERVE8 /* ARM cortexM3 alignment*/

	ldr r0, =0xE000ED08 /* SCB_VTOR(vector table offset register)*/
	ldr r0, [r0] /* dereference VTOR, got the address of vector table*/
	ldr r0, [r0] /* dereference vector table, got the first address (must be MSP)*/

	msr msp, r0 /* initialise the main stack pointer*/

	cpsie i /* enable IRQ*/
	cpsie f
	dsb
	isb

	svc 0 /* jump to vPortSVCHandler*/
	nop
	nop
}

__asm void vPortSVCHandler() {
	extern pxCurrentTCB

	PRESERVE8

	ldr r3, =pxCurrentTCB
	ldr r1, [r3]
	ldr r0, [r1]
	ldmia r0!, {r4-r11}
	msr psp, r0
	isb
	mov r0, #0
	msr basepri, r0
	orr r14, #0xd

	bx r14

}

__asm void xPortPendSVHandler() {
	extern pxCurrentTCB
	extern vTaskSwitchContext
	PRESERVE8

	mrs r0, psp
	isb

	ldr r3, =pxCurrentTCB
	ldr r2, [r3]

	stmdb r0!, {r4-r11}
	str r0, [r2]

	stmdb sp!, {r3, r14}

	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r0
	dsb
	isb
	bl vTaskSwitchContext
	mov r0, #0
	msr basepri, r0
	ldmia sp!, {r3, r14}

	ldr r1, [r3]
	ldr r0, [r1]
	ldmia r0!, {r4-r11}
	msr psp, r0
	isb
	bx r14
	
	nop
}
