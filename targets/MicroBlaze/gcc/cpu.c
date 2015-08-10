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
#include <board.h>

extern void rtos_sysrequest(uint32_t);

void rtos_InvokeScheduler(void)
{
	RTOS_SavedCriticalState(saved_state);

	RTOS_EnterCriticalSection(saved_state);
	rtos_sysrequest(0);
	RTOS_ExitCriticalSection(saved_state);
}

void rtos_InvokeYield(void)
{
	RTOS_SavedCriticalState(saved_state);

	RTOS_EnterCriticalSection(saved_state);
	rtos_sysrequest(1);
	RTOS_ExitCriticalSection(saved_state);
}

void rtos_Isr_Handler(void)
{
	RTOS.InterruptNesting++;
	board_HandleIRQ();
	rtos_Scheduler();
	RTOS.InterruptNesting--;
}

void RTOS_DefaultIdleFunction(void *p)
{
	while(1)
	{
		__asm__ volatile ("nop");
	}
	(void)p;
}
// ------------------------------------------------------ Some initialization stuff. ---------------------------------------------
extern void rtos_TaskEntryPoint(void);

extern uint8_t _SDA_BASE_[];
extern uint8_t _SDA2_BASE_[];

void rtos_TargetInitializeTask(RTOS_Task *task, unsigned long stackCapacity)
{
	uint32_t msr;
	int i;
	rtos_StackFrame *sp;

	if (0 != task->SP0)	// The stack pointer can legitimately be 0 at this point if the task is initialized as the 'current thread of execution'.
	{
		task->SP = (void *)(((char *)task->SP0) + (sizeof(RTOS_StackItem_t) * stackCapacity) - (RTOS_INITIAL_STACK_DEPTH));
		sp = (rtos_StackFrame *)(task->SP);

		for (i = 0; i < 32; i++)
		{
			sp->regs[i] = i;
		}

	    msr = mfmsr() | 2;

		sp->regs[0] = msr;									// MSR
		sp->regs[2] = (uint32_t)&_SDA2_BASE_;				// R2
		sp->regs[14] = ((uint32_t)rtos_TaskEntryPoint); 	// R14
		sp->regs[13] = (uint32_t)&_SDA_BASE_; 				// R13
	}
}

int RTOS_StartMultitasking(void)
{
	RTOS_ASSERT(0 != RTOS.TaskList[RTOS_Priority_Idle]);
	RTOS.CurrentTask =  RTOS.TaskList[RTOS_Priority_Idle];	// Default to the Idle task.
	rtos_Scheduler();										// Let the scheduler pick a higher priority task.
	// BTW: There is no need to enable interrupts here,
	// the task will have it enabled in their saved SPSR.
	RTOS.IsRunning = 1;										// Indicate that the OS is up and running.
	__asm__ volatile ("BRLID R15, rtos_restore_context");
	__asm__ volatile ("OR R0, R0, R0");
	return RTOS_ERROR_FAILED;								// We should never actually get here!
}
