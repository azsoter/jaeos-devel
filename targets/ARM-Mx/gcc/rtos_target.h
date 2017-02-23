#ifndef RTOS_TARGET_H
#define RTOS_TARGET_H

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

#include <rtos_arm_mx.h>
#include <rtos_types.h>


#if !defined(RTOS_SYS_INTERRUPT_PRIO)
#define RTOS_SYS_INTERRUPT_PRIO_bits ARM_M3_LOWEST_INTERRUPT_PRIORITY_bits
#endif

#if !defined(RTOS_DEFAULT_INT_PRIORITY)
#define RTOS_DEFAULT_INT_PRIORITY 2
#endif

// We need to be able to issue an SVC even with other interrupts disabled.
// Just make it a higher priority than other interrupts.
// BTW: SVC is not called from other interrupt handlers, it is strictly for being able to invoke SVC from inside
// a critical section.
#define RTOS_SVC_INT_PRIORITY 0

#if ((RTOS_DEFAULT_INT_PRIORITY) <= (RTOS_SVC_INT_PRIORITY))
#error RTOS_DEFAULT_INT_PRIORITY number must be higher than the SVC priority number.
#endif

#define RTOS_DEFAULT_INT_PRIORITY_bits ARM_M3_INTERRUPT_PRIORITY_TO_bits(RTOS_DEFAULT_INT_PRIORITY)
#define RTOS_SVC_INT_PRIORITY_bits ARM_M3_INTERRUPT_PRIORITY_TO_bits(RTOS_SVC_INT_PRIORITY)
#define RTOS_INTERRUPTS_DISABLED_PRIORITY_bits ARM_M3_INTERRUPT_PRIORITY_TO_bits((RTOS_SVC_INT_PRIORITY + 1))

#define rtos_disableInterrupts(SAVED, NEW) 								\
	__asm__ __volatile__ ( 												\
		"\tMRS\t%0, BASEPRI			\n"	 /* SAVED <- BASEPRI.	*/		\
		"\tMSR\tBASEPRI, %1			\n"	 /* BASEPRI <- NEW	*/			\
		"ISB						\n"									\
	 : "=&r" (SAVED) : "r" (NEW):)

#define rtos_restoreInterrupts(SAVED) 								\
	__asm__ __volatile__ ( 											\
		"\tMSR\tBASEPRI, %0 \n"	/*  BASEPRI <- saved_CPSR. */		\
		"ISB	\n"													\
	 : : "r" (SAVED):)

#define rtos_readCONTROL() ({ unsigned long rtos_tmp;  asm ("MRS\t%0, CONTROL": "=r" (rtos_tmp)); rtos_tmp; })
#define rtos_readBASEPRI() ({ unsigned long rtos_tmp;  asm ("MRS\t%0, BASEPRI": "=r" (rtos_tmp)); rtos_tmp; })

#define rtos_readPSR() ({ unsigned long rtos_tmp;  asm ("MRS\t%0, PSR": "=r" (rtos_tmp)); rtos_tmp; })
#define RTOS_IsInsideIsr() (0 != ((ARM_M3_PSR_ISR_NUMBER_mask) & rtos_readPSR()))

#define RTOS_SavedCriticalState(X) 	RTOS_Critical_State X

#define RTOS_EnterCriticalSection(X)	rtos_disableInterrupts(X, RTOS_INTERRUPTS_DISABLED_PRIORITY_bits)
#define RTOS_ExitCriticalSection(X)	rtos_restoreInterrupts(X)

#define RTOS_EnableInterrupts() rtos_restoreInterrupts(0)
#define RTOS_DisableInterrupts() do { RTOS_Critical_State rtos_tmp_saved; rtos_disableInterrupts(rtos_tmp_saved,  RTOS_INTERRUPTS_DISABLED_PRIORITY_bits); } while(0)


#define RTOS_INLINE static inline

#define rtos_CLZ(X) __builtin_clzl(X)

#define RTOS_INVOKE_SCHEDULER() __asm__ __volatile__ ("SVC #0")
#define RTOS_INVOKE_YIELD() __asm__ __volatile__ ("SVC #1")
#define RTOS_SIGNAL_SCHEDULER_FROM_INTERRUPT() do { *((uint32_t *)(ARM_M3_REG_ICSR)) = (ARM_M3_REG_ICSR_bit_PENDV); } while(0)

#if defined(RTOS_TARGET_HAS_FPU)

#define RTOS_TASK_EXEC_LOCATION(TASK) \
		((0 != (0x10 & ((rtos_StackFrame *)((TASK)->SP))->swsaved.lr_int)) ? \
		((rtos_StackFrame *)((TASK)->SP))->hwsaved.pc : ((rtos_StackFrameWithFPU *)((TASK)->SP))->hwsaved.pc)
#else

#define RTOS_TASK_EXEC_LOCATION(TASK) ((rtos_StackFrame *)((TASK)->SP))->hwsaved.pc

#endif

// Utility functions.
#define RTOS_DEFAULT_IDLE_FUNCTION RTOS_DefaultIdleFunction
extern void RTOS_DefaultIdleFunction(void *p);

extern int RTOS_StartMultitasking(void);

#endif

