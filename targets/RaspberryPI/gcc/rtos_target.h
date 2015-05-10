#ifndef RTOS_TARGET_H
#define RTOS_TARGET_H

/*
* Copyright (c) Andras Zsoter 2014, 2015.
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

#include <rtos_types.h>

#define rtos_enableInterrupts() do { 							\
	__asm__ volatile ("STMDB	SP!, {R0}");		/* Push R0.    */	\
	__asm__ volatile ("MRS		R0, CPSR");		/* R0 <- CPSR. */	\
	__asm__ volatile ("BIC		R0, R0, #0x80");	/* Enable IRQ. */	\
	__asm__ volatile ("MSR		CPSR, R0");		/* CPSR <- R0  */	\
	__asm__ volatile ("LDMIA	SP!, {R0}" );		/* Pop R0.     */	\
} while(0)

#define rtos_disableInterrupts(SAVED) 								\
	__asm__ volatile ( 									\
		"\tMRS\tR1, CPSR			\n"	 /* R1 <- CPSR.	*/		\
		"\tMOV\t%0, R1				\n"	 /* Save CPSR.	*/		\
		"\tORR\tR1, R1, #0xC0			\n"	 /* Disable IRQ, FIQ. */	\
		"\tMSR\tCPSR, R1			\n"	 /* CPSR <- R1	*/		\
	 : "=r" (SAVED) : : "r1","cc")

#define rtos_restoreInterrupts(SAVED) 								\
	__asm__ volatile ( 									\
		"\tMSR\tCPSR, %0 \n"	/*  CPSR <- saved_CPSR. */				\
	 : : "r" (SAVED) : "cc")


#define RTOS_SavedCriticalState(X) 	RTOS_Critical_State X

#if defined(__thumb__)
extern RTOS_Critical_State rtos_arm_disableInterrupts(void);
extern void rtos_arm_enableInterrupts(void);
extern void rtos_arm_restoreInterrupts(RTOS_Critical_State saved_state);
extern uint32_t rtos_arm_CLZ(uint32_t x);
#define RTOS_EnterCriticalSection(X)	(X) = rtos_arm_disableInterrupts()
#define RTOS_ExitCriticalSection(X)	rtos_arm_restoreInterrupts(X)
#define RTOS_DisableInterrupts()	(void)rtos_arm_disableInterrupts()
#define RTOS_EnableInterrupts()		rtos_arm_enableInterrupts()
#define rtos_CLZ(X)			rtos_arm_CLZ(X)
#else
#define RTOS_EnterCriticalSection(X)	rtos_disableInterrupts(X)
#define RTOS_ExitCriticalSection(X)	rtos_restoreInterrupts(X)
#define RTOS_EnableInterrupts()		rtos_enableInterrupts()
#define RTOS_DisableInterrupts()	do { RTOS_Critical_State rtos_tmp_saved; rtos_disableInterrupts(rtos_tmp_saved); } while(0)
#define rtos_CLZ(X)			__builtin_clzl(X)
#endif



// extern int _write(int fd, void *prt, unsigned int count);

#define RTOS_INLINE static inline

#define RTOS_INVOKE_SCHEDULER() __asm volatile ( "SWI 0" ) 
#define RTOS_INVOKE_YIELD() __asm volatile ( "SWI 1" ) 

// For debugging:
#define rsp()   ({ unsigned long rtos_tmp;  asm ("MOV\t%0, SP":   "=r" (rtos_tmp)); rtos_tmp; })
#define rcpsr() ({ unsigned long rtos_tmp;  asm ("MRS\t%0, CPSR": "=r" (rtos_tmp)); rtos_tmp; })

#define RTOS_TASK_EXEC_LOCATION(TASK) ((rtos_StackFrame *)((TASK)->SP))->regs[15]

// Utility functions.
#define RTOS_DEFAULT_IDLE_FUNCTION RTOS_DefaultIdleFunction
extern void RTOS_DefaultIdleFunction(void *p);

extern int RTOS_StartMultitasking(void);

#endif

