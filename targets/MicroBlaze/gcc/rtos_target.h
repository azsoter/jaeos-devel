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


#define rtos_enableInterrupts() \
		__asm__ volatile (	\
				"msf r3, rmsr \r\n" \
				"ori   r3,  r3, 2\r\n" \
				"mts rmsr, r3 \r\n" \
							 : : : "r3" )

#define rtos_disableInterrupts() ({ RTOS_Critical_State _saved_; \
		__asm__ volatile (	\
				"mfs %0, rmsr \r\n" \
				"andni   r3,  %0, 2\r\n" \
				"mts rmsr, r3 \r\n" \
							 : "=r" (_saved_): : "r3"); _saved_; })

#define rtos_restoreInterrupts(SAVED)	__asm__ volatile ("mts rmsr, %0\n" : : "r" (SAVED) : "cc")

#define RTOS_SavedCriticalState(X) 	RTOS_Critical_State X
#define RTOS_EnterCriticalSection(X)	(X) = rtos_disableInterrupts()
#define RTOS_ExitCriticalSection(X)	rtos_restoreInterrupts(X)


#define RTOS_EnableInterrupts() rtos_enableInterrupts()
#define RTOS_DisableInterrupts() rtos_disableInterrupts()

#define rtos_CLZ(X) __builtin_clzl(X)

extern void rtos_InvokeScheduler(void);
extern void rtos_InvokeYield(void);
#define RTOS_INVOKE_SCHEDULER() rtos_InvokeScheduler()
#define RTOS_INVOKE_YIELD() rtos_InvokeYield()

#define RTOS_TASK_EXEC_LOCATION(TASK) ((rtos_StackFrame *)((TASK)->SP))->regs[14]

// Utility functions.
extern void RTOS_DefaultIdleFunction(void *p);
#define RTOS_DEFAULT_IDLE_FUNCTION RTOS_DefaultIdleFunction

extern int RTOS_StartMultitasking(void);

#endif

