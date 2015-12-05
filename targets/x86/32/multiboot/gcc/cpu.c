/*
* Copyright (c) Andras Zsoter 2015.
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

#if defined(RTOS_SMP)
#error This target does not support SMP/multicore configurations.
#endif
extern void rtos_TaskEntryPoint(void);

void rtos_TargetInitializeTask(RTOS_Task *task, unsigned long stackCapacity)
{
	rtos_StackFrame *sp;
	
	// The stack pointer can legitimately be 0 at this point if the task is initialized as the 'current thread of execution'.
	if (0 != task->SP0)
	{
		task->SP = (void *)(((char *)task->SP0) + (sizeof(RTOS_StackItem_t) * stackCapacity) - (RTOS_INITIAL_STACK_DEPTH));
		sp = (rtos_StackFrame *)(task->SP);

		sp->eflags = 0x200;
		// sp->eflags = 0;
		sp->cs = 0x10;
		sp->eip=(RTOS_StackItem_t)(&rtos_TaskEntryPoint);
		sp->esp = ((RTOS_StackItem_t)(&(sp->eax))) + 4;
		sp->ebp = sp->esp;
		sp->eax = 0xaaaaaaaa;
		sp->ebx = 0xbbbbbbbb;
		sp->ecx = 0xcccccccc;
		sp->edx = 0xdddddddd;
		sp->edi = 0x00000000;
		sp->esi = 0x11111111;
	}

}
// -------------------------------------------------------------------------------------------------------------------------------
void RTOS_DefaultIdleFunction(void *p)
{
	while(1)
	{
		__asm__ volatile ("hlt");
	}
	(void)p;
}
// -------------------------------------------------------------------------------------------------------------------------------
void rtos_TaskEntryPoint(void)
{
	rtos_RunTask();
	while(1);
}

int RTOS_StartMultitasking(void)
{
	RTOS_ASSERT(0 != RTOS.TaskList[RTOS_Priority_Idle]);

	RTOS.CurrentTask =  RTOS.TaskList[RTOS_Priority_Idle];	// Default to the Idle task.
	rtos_Scheduler();					// Let the scheduler pick a higher priority task.
	// BTW: There is no need to enable interrupts here, 
	// the task will have it enabled in their saved EFLAGS.
	RTOS.IsRunning = 1;					// Indicate that the OS is up and running.
	__asm__ volatile("movl (RTOS), %ebx");
	__asm__ volatile("movl (%ebx), %esp");
	__asm__ volatile("popa");
	MAGIC_BREAKPOINT();					// Magic breakpoint for Bochs.
	__asm__ volatile("iret");
	return RTOS_ERROR_FAILED;				// We should never actually get here!
}

