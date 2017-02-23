/*
* Copyright (c) Andras Zsoter 2016-2017.
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

// -------------------------------------------------------------------------------------------------------------------------------
extern void PendSV_Handler(void) __attribute__ (( naked ));
void PendSV_Handler(void)
{
	__asm__ __volatile__ ("LDR		R2,=rtos_Scheduler");
	__asm__ __volatile__ ("rtos_TaskSwitchRoutine:");
	__asm__ __volatile__ ("MRS		R1,	PSP");			/* Process Stack Pointer. */
	__asm__ __volatile__ ("MRS		R3,	BASEPRI");		/* R3 <- BASEPRI */

#if defined(RTOS_TARGET_HAS_FPU)
	__asm__ __volatile__ ("TST LR, #0x10");
	__asm__ __volatile__ ("IT EQ");
	__asm__ __volatile__ ("VSTMDBEQ R1!, { S16-S31 }");
#endif

	__asm__ __volatile__ ("STMDB	R1!, {R3-R11, LR}");/* PUSH { BASEPRI, R4-R11, LR } */
	// Need to disable interrupts here???
	__asm__ __volatile__ ("LDR		R0, =RTOS");		/* The address of the RTOS structure.*/
	__asm__ __volatile__ ("LDR		R0, [R0]");			/* The first address is a pointer to the Current Task structure. */
	__asm__ __volatile__ ("STR		R1, [R0]");			/* Store stack pointer in the task structure. */

	__asm__ __volatile__ ("MOV		R3, %0"::"I" (RTOS_INTERRUPTS_DISABLED_PRIORITY_bits));
	__asm__ __volatile__ ("MSR		BASEPRI, R3");		/* Disable Interrupts. */

	__asm__ __volatile__ ("BLX		R2");				/* Call the Scheduler */
	__asm__ __volatile__ ("rtos_SwitchToNewTask:");
	__asm__ __volatile__ ("LDR		R0, =RTOS");		/* The address of the RTOS structure.*/
	__asm__ __volatile__ ("LDR		R0, [R0]");			/* The first address is a pointer to the Current Task structure. */
	__asm__ __volatile__ ("LDR		R1, [R0]");			/* The first field in the task structure is the stack pointer. */

	__asm__ __volatile__ ("LDMIA	R1!, {R3-R11, LR}");/* POP { BASEPRI, R4-R11, LR } */

#if defined(RTOS_TARGET_HAS_FPU)
	__asm__ __volatile__ ("TST LR, #0x10");
	__asm__ __volatile__ ("IT EQ");
	__asm__ __volatile__ ("VLDMIAEQ R1!, { S16-S31 }");
#endif

	__asm__ __volatile__ ("MSR		PSP, R1");			/* Restore process stack pointer. */
	__asm__ __volatile__ ("MSR		BASEPRI, R3");		/* Restore BASEPRI */

	__asm__ __volatile__ ("BX		LR");				/* Return */

}

extern void PrintHex(uint32_t);
void rtos_debug_SVC_Check(uint32_t arg)
{
	PrintHex(arg);
	// PrintHex(rpsr());
}

extern void SVC_Handler(void) __attribute__ (( naked ));
void SVC_Handler(void)
{
	__asm__ __volatile__ ("TST LR, #4");							// Recommended by ARM.
	__asm__ __volatile__ ("ITE EQ");								// Need to figure out which stack was used.
	__asm__ __volatile__ ("MRSEQ R0, MSP");
	__asm__ __volatile__ ("MRSNE R0, PSP");
	__asm__ __volatile__ ("NOP");
// -----------------------------------------------------------
	__asm__ __volatile__ ("LDR		R0, [R0, #24]");				// PC
	__asm__ __volatile__ ("LDR		R0, [R0, -2]");					// Instructions including SVC
	__asm__ __volatile__ ("AND		R0, #0xFF");					// Get parameter bits.
	__asm__ __volatile__ ("TEQ		R0, #0x00");					// Run the scheduler.
	__asm__ __volatile__ ("BEQ		PendSV_Handler");
#if defined(RTOS_INVOKE_YIELD)
	__asm__ __volatile__ ("LDR		R2, =rtos_SchedulerForYield");
	__asm__ __volatile__ ("TEQ		R0, #0x01");					// Yield
	__asm__ __volatile__ ("BEQ		rtos_TaskSwitchRoutine");
	__asm__ __volatile__ ("");
#endif
	__asm__ __volatile__ ("TEQ		R0, #0x88");					// Start the first task.
	__asm__ __volatile__ ("BEQ		rtos_SwitchToNewTask");
	__asm__ __volatile__ ("BX		LR");							// None of the above, just return.
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
void rtos_TaskEntryPoint(void)
{
	rtos_RunTask();
	while(1);
}

void rtos_TargetInitializeTask(RTOS_Task *task, unsigned long stackCapacity)
{
	rtos_StackFrame *sp;

	RTOS_ASSERT(0 != task->SP0); // Start up code does not support initializing a s'current thread', so do not allow it.

	task->SP = (void *)(((char *)task->SP0) + (sizeof(RTOS_StackItem_t) * stackCapacity) - (RTOS_INITIAL_STACK_DEPTH));
	sp = (rtos_StackFrame *)(task->SP);
	sp->hwsaved.r0 = 0;
	sp->hwsaved.r1 = 1;
	sp->hwsaved.r2 = 2;
	sp->hwsaved.r3 = 3;
	sp->hwsaved.lr_app = 0xAAAAAAAA; // Just for debugging.
	sp->hwsaved.psr = 0x01000000; // ?????????????????????????????????????
	sp->hwsaved.pc = ((RTOS_StackItem_t)rtos_TaskEntryPoint);

	sp->swsaved.r4 = 4;
	sp->swsaved.r5 = 5;
	sp->swsaved.r6 = 6;
	sp->swsaved.r7 = 7;
	sp->swsaved.r8 = 8;
	sp->swsaved.r9 = 9;
	sp->swsaved.r10 = 10;
	sp->swsaved.r11 = 11;;
	sp->swsaved.lr_int = 0xFFFFFFFD; // This seems to be the correct EXC_RETURN.
	sp->swsaved.basepri = 0;
}

int RTOS_StartMultitasking(void)
{
	RTOS_ASSERT(0 != RTOS.TaskList[RTOS_Priority_Idle]);

#if defined(RTOS_USE_TIMER_TASK)
	RTOS_ASSERT(0 != RTOS.TaskList[RTOS_Priority_Timer]);
#endif

	RTOS_CURRENT_TASK() =  RTOS.TaskList[RTOS_Priority_Idle];	// Default to the Idle task.

	RTOS.IsRunning = 1;								// Indicate that the OS is up and running.

	rtos_Scheduler();								// Let the scheduler pick a higher priority task.

	// There is no need to enable interrupts in BASEPRI here,
	// the task will have it enabled in their saved BASEPRI.
	__asm__ __volatile__ ("CPSIE I");		// Master enable interrupts.
	__asm__ __volatile__ ("SVC #0x88");		// Start the first task.

	return RTOS_ERROR_FAILED;						// We should never actually get here!
}

#if defined(RTOS_USE_DEFAULT_ARM_Mx_SYSTICK_HANDLER)
// On simple ARM-M3/M4 systems this handler may be enough.
// On others you may need to call things in a HAL or other system library layer
// if the RTOS and HAL share a timer.
// There is a more complex version in board_XXXXX.c
// BTW: On the ARM-Mx the use of a separate timer task is recommended.
extern void SysTick_Handler(void) __attribute__ ((isr));

void SysTick_Handler(void)
{
	RTOS_SavedCriticalState(saved_state);

	// Currently rtos_TimerTick() assumes that it will not be interrupted.
	// So run it with interrupts disabled.
	// To kep it short, use a timer task.
	RTOS_EnterCriticalSection(saved_state);
		rtos_TimerTick();
	RTOS_ExitCriticalSection(saved_state);

	RTOS_SIGNAL_SCHEDULER_FROM_INTERRUPT();
}
#endif

static void sysTick_init(void)
{
	*((uint32_t *)(ARM_M3_REG_STRVR)) = ( (SYSTEM_CLOCK_FREQUENCY) / (RTOS_TICKS_PER_SECOND) ) - 1UL;
	*((uint32_t *)(ARM_M3_REG_STCSR)) = ARM_M3_REG_STCSR_bit_CLKSOURCE | ARM_M3_REG_STCSR_bit_TICKINT | ARM_M3_REG_STCSR_bit_ENABLE;

}

// Set up interrupts, SysTick, etc. to some sane configuration.
// On some systems these will be reprogrammed partially or completely by vendor supplied initialization code.
// If you have configuration tools for your microcontroller make sure that the setup is compatible with RTOS operation.
// Recommended:
// SVC should have a priority of 0 (highest priority).
// PendSV and SysTick should have a set to maximum (lowest priority).
// Interrupts coming from hardware devices should be set to something in between.
// We have a default programmed here, but that can be changed.
// Do NOT set them to 0! That is for SVC and system exceptions.
int rtos_CpuInit(void)
{
	int i;
	uint32_t tmp;
	__asm__ __volatile__ ("CPSID I");	// Master Disable interrupts.
	RTOS_DisableInterrupts();			// Disable interrupts in the OS.

	// SysTick Priority -- Systick can run at a low priority.
	ARM_M3_SET_REG(ARM_M3_REG_SHPR3, (*(uint32_t *)(ARM_M3_REG_SHPR3) | ((RTOS_SYS_INTERRUPT_PRIO_bits) << 24)));	// SysTick Priority.

	// PendSV Priority -- Has to be low priority, so that if it is triggered from an interrupt handler it PendSV executes afterwards.
	ARM_M3_SET_REG(ARM_M3_REG_SHPR3, (*(uint32_t *)(ARM_M3_REG_SHPR3)) |  ((RTOS_SYS_INTERRUPT_PRIO_bits) << 16));	// PendSV Priority.

	// SVCall Priority -- must be high priority so that we can do SVCall even with other interrupts disabled.
	ARM_M3_SET_REG(ARM_M3_REG_SHPR2, ((RTOS_SVC_INT_PRIORITY_bits) << 24)); 										// SVC Priority.

	// Initialize interrupt priorities to default.
	// The application code can change it, but these have to be set to something reasonable
	// for the RTOS to work correctly.
	tmp = RTOS_DEFAULT_INT_PRIORITY_bits;
	tmp = (tmp << 8) | RTOS_DEFAULT_INT_PRIORITY_bits;
	tmp = (tmp << 8) | RTOS_DEFAULT_INT_PRIORITY_bits;
	tmp = (tmp << 8) | RTOS_DEFAULT_INT_PRIORITY_bits;

	for (i = 0; i <= 59; i += 4)
	{
		*((uint32_t *)(ARM_M3_REG_IPR0 + i)) = tmp;
	}

	sysTick_init();

	return 0;
}
