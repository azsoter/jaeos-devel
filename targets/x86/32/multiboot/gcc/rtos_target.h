#ifndef RTOS_TARGET_H
#define RTOS_TARGET_H

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

#include <rtos_types.h>

#define rtos_enableInterrupts() __asm__ volatile ("sti")

#define rtos_disableInterrupts(SAVED) 								\
	__asm__ volatile ( 									\
		"\tpushf			\n"	 /* Push flags		*/		\
		"\tpop\t%0			\n"	 /* Pop Register	*/		\
		"\tcli\t			\n"	 /* Disable Interrupts	*/		\
	 : "=r" (SAVED) : : "cc" )

#define rtos_restoreInterrupts(SAVED) 						\
	__asm__ volatile ( 							\
		"\tpush\t%0 			\n"				\
		"\tpopf 			\n"				\
	 : : "r" (SAVED) : "cc")


#define RTOS_SavedCriticalState(X) 	RTOS_Critical_State X

#define RTOS_EnterCriticalSection(X)	rtos_disableInterrupts(X)
#define RTOS_ExitCriticalSection(X)	rtos_restoreInterrupts(X)
#define RTOS_EnableInterrupts()		rtos_enableInterrupts()
#define RTOS_DisableInterrupts()	do { RTOS_Critical_State rtos_tmp_saved; rtos_disableInterrupts(rtos_tmp_saved); } while(0)
#define rtos_CLZ(X)			__builtin_clzl(X)

#define RTOS_INLINE static __inline__

#if 1
#define RTOS_INVOKE_SCHEDULER() __asm__ volatile ( "movl $0,%%eax\n int $0x60\n" : : : "eax","cc" ) 
#define RTOS_INVOKE_YIELD()     __asm__ volatile ( "movl $1,%%eax\n int $0x60\n" : : : "eax","cc" ) 
#else
#define RTOS_INVOKE_SCHEDULER() __asm__ volatile ( "pushf\n pushl %eax\n movl $0,%eax\n int $0x60\n popl %eax\n popf\n" ) 
#define RTOS_INVOKE_YIELD() __asm__ volatile ( "pushf\n pushl %eax\n movl $1,%eax\n int $0x60\n popl %eax\n popf\n" ) 
#endif

#define RTOS_TASK_EXEC_LOCATION(TASK) ((rtos_StackFrame *)((TASK)->SP))->eip

// Utility functions.
#define RTOS_DEFAULT_IDLE_FUNCTION RTOS_DefaultIdleFunction
extern void RTOS_DefaultIdleFunction(void *p);

extern int RTOS_StartMultitasking(void);

#if 0
#define MAGIC_BREAKPOINT() __asm__ volatile ("xchgw %bx, %bx\n"); /* Magic Breakpoint for Bochs. -- http://bochs.sourceforge.net/ */
#else
#define MAGIC_BREAKPOINT() (void)0
#endif

#endif

