/*
* Copyright (c) Andras Zsoter 2014.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*/

#include <stdint.h>
#include <rtos.h>
#include <rtos_internals.h>
// #include <board.h>

#define RTOS_RESTORE_CONTEXT()	\
{ \
	__asm__ volatile ("LDR		R0, =RTOS");		/* The address of the RTOS structure.*/ 	\
	__asm__ volatile ("LDR		R0, [R0]");		/* The first address is a pointer to the Current Task structure. */ \
	__asm__ volatile ("LDR		LR, [R0]");		/* The first field in the task structure is the stack pointer. */ \
	__asm__ volatile ("LDMFD	LR!, {R0}");		/* POP SPSR value to R0. */ \
	__asm__ volatile ("MSR		SPSR_cxsf, R0");	/* Restore SPSR. */ \
	__asm__ volatile ("LDMFD	LR, {R0-R14}^");	/* POP general purpose registers. */ \
	__asm__ volatile ("LDR		LR, [LR, #+60]");	/* Load return address to LR. */ \
	__asm__ volatile ("SUBS		PC, LR, #4");		/* Update program counter. */ \
}

#define RTOS_SAVE_CONTEXT()	\
{ \
	__asm__ volatile ("STMDB	SP!, {R0}");		/* Push R0. */ \
	__asm__ volatile ("STMDB	SP,{SP}^");		/* [SP - 4] <-- 'user mode SP'. */ \
	__asm__ volatile ("LDR		R0, [SP, #-4]");	/* R0 <-- [SP - 4] */ \
	__asm__ volatile ("STMDB	R0!, {LR}");		/* Push the return address.  */ \
	__asm__ volatile ("MOV		LR, R0");		/* LR <- R0 ('user mode SP')*/ \
	__asm__ volatile ("LDMIA	SP!, {R0}");		/* Pop R0. */ \
	__asm__ volatile ("STMDB	LR,{R0-LR}^");		/* Push all user mode registers. */ \
	__asm__ volatile ("SUB		LR, LR, #60");		/* Adjust LR. */ \
	__asm__ volatile ("MRS		R0, SPSR");		/* Read SPSR. */ \
	__asm__ volatile ("STMDB	LR!, {R0}");		/* Push the SPSR value. */ \
	__asm__ volatile ("LDR		R0, =RTOS");		/* The address of the RTOS structure. */ \
	__asm__ volatile ("LDR		R0, [R0]");		/* The first field points to the Current task structure.*/ \
	__asm__ volatile ("STR		LR, [R0]");		/* Store the stack pointer in the task structure. */ \
}

void rtos_DispatchScheduler(void)
{
	uint32_t code = *(uint32_t *)(((rtos_StackFrame *)(RTOS.CurrentTask->SP))->regs[15] - 8); // The SWI instruction.
	code = code & 0x00FFFFFF;
	switch(code)
	{
		case 0:	
			rtos_Scheduler();		// Run the scheduler.
			break;
		case 1:
			rtos_SchedulerForYield();	// Yield.
			break;
		default:
			break;
	}
}

void rtos_Invoke_Scheduler(void) __attribute__((interrupt("SWI"), naked));

void rtos_Invoke_Scheduler(void)
{
	// The return address after an SVC instruction is off by one instruction
	// compared to where it woudl be after an IRQ, adjust for that.
	__asm volatile ( "ADD		LR, LR, #4" );
	RTOS_SAVE_CONTEXT();
	__asm__ volatile ("bl rtos_DispatchScheduler");
	RTOS_RESTORE_CONTEXT();	
}

void rtos_Isr_Handler(void) __attribute__((naked));

void rtos_Isr_Handler(void)
{
	RTOS_SAVE_CONTEXT();
	RTOS.InterruptNesting++;
	__asm volatile( "bl IRQInterrupt" );
	rtos_Scheduler();
	RTOS.InterruptNesting--;
	RTOS_RESTORE_CONTEXT();	
}
// -------------------------------------------------------------------------------------------------------------------------------
void RTOS_DefaultIdleFunction(void *p)
{
	while(1)
	{
		__asm__ volatile ("wfi");
	}
	(void)p;
}
// ------------------------------------------------------ Some initialization stuff. ---------------------------------------------
extern void rtos_TaskEntryPoint(void);

void rtos_TargetInitializeTask(RTOS_Task *task, unsigned long stackCapacity)
{
	rtos_StackFrame *sp;
	
	// The stack pointer can legitimately be 0 at this point if the task is initialized as the 'current thread of execution'.
	if (0 != task->SP0)
	{
		task->SP = (void *)(((char *)task->SP0) + (sizeof(RTOS_StackItem_t) * stackCapacity) - (RTOS_INITIAL_STACK_DEPTH));
		sp = (rtos_StackFrame *)(task->SP);
		sp->spsr = 0x1F;
		sp->regs[15] = ((RTOS_StackItem_t)rtos_TaskEntryPoint) + 4;
		sp->regs[14] = 0x14; 
		sp->regs[13] = ((RTOS_StackItem_t)(task->SP)) + sizeof(rtos_StackFrame); 
		sp->regs[12] = 0x12; 
		sp->regs[11] = 0x11; 
		sp->regs[10] = 0x10; 
		sp->regs[9] = 0x9; 
		sp->regs[8] = 0x8; 
		sp->regs[7] = 0x7; 
		sp->regs[6] = 0x6; 
		sp->regs[5] = 0x5; 
		sp->regs[4] = 0x4; 
		sp->regs[3] = 0x3; 
		sp->regs[2] = 0x2; 
		sp->regs[1] = 0x1; 
		sp->regs[0] = 0x0;
	}
}

int RTOS_StartMultitasking(void)
{
	RTOS_ASSERT(0 != RTOS.TaskList[RTOS_Priority_Idle]);

	RTOS.CurrentTask =  RTOS.TaskList[RTOS_Priority_Idle];	// Default to the Idle task.
	rtos_Scheduler();					// Let the scheduler pick a higher priority task.
	// BTW: There is no need to enable interrupts here, 
	// the task will have it enabled in their saved SPSR.
	RTOS.IsRunning = 1;					// Indicate that the OS is up and running.
	__asm__ volatile ("LDR R0, =0x1D3");			// Set SVC Mode.
	__asm__ volatile ("MSR CPSR, R0");		
	RTOS_RESTORE_CONTEXT();					// Restore the context of the selected task.
	return RTOS_ERROR_FAILED;				// We should never actually get here!
}

