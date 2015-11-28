#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H
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

// Type to store saved flags e.g. when we enter and exit critical state.
typedef uint32_t RTOS_Critical_State;

typedef uint32_t RTOS_StackItem_t;

struct rtos_StackFrame
{
	RTOS_StackItem_t edi;
	RTOS_StackItem_t esi;
	RTOS_StackItem_t ebp;
	RTOS_StackItem_t esp;
	RTOS_StackItem_t ebx;
	RTOS_StackItem_t edx;
	RTOS_StackItem_t ecx;
	RTOS_StackItem_t eax;
	RTOS_StackItem_t eip;
	RTOS_StackItem_t cs;
	RTOS_StackItem_t eflags;
};

typedef struct rtos_StackFrame rtos_StackFrame;

// This is needed when we create stack frames for tasks that haven't started yet.
#define RTOS_INITIAL_STACK_DEPTH (sizeof(rtos_StackFrame) + 8)


#if ((RTOS_Priority_Highest) > 31)
#	define RTOS_TASKSET_TYPE uint64_t
#	define RTOS_HIGHEST_SUPPORTED_TASK_PRIORITY 63
#	define RTOS_FIND_HIGHEST(X) ((0 == (X)) ? ~(uint64_t)0 : (63 - __builtin_clzll((uint64_t)(X))))
#else
#	if !defined( RTOS_FIND_HIGHEST)
#		define RTOS_FIND_HIGHEST(X) ((0 == (X)) ? ~(uint32_t)0 : (31 - rtos_CLZ(X)))
#	endif
#endif

#define RTOS_MIN_STACK_SIZE 512 /* What is the real number? */

#endif

