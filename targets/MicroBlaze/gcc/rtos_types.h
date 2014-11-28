#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H
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
#include <mb_interface.h>

#define RTOS_INLINE static inline

typedef uint32_t RTOS_Critical_State;

// Stack frame saved by the interrupt prologue code.
typedef uint32_t RTOS_StackItem_t;

struct rtos_StackFrame
{
	RTOS_StackItem_t regs[32];
};

typedef struct rtos_StackFrame rtos_StackFrame;

// This is needed when we create stack frames for tasks that haven't started yet.
#define RTOS_INITIAL_STACK_DEPTH (sizeof(rtos_StackFrame) + 8)


#if ((RTOS_Priority_Highest) > 31)
#	define RTOS_TASKSET_TYPE uint64_t
#	define RTOS_HIGHEST_SUPPORTED_TASK_PRIORITY 63
#	define RTOS_FIND_HIGHEST(X) (63 - __builtin_clzll((uint64_t)(X)))
#else
#	if !defined( RTOS_FIND_HIGHEST)
#		define RTOS_FIND_HIGHEST(X) (31 - rtos_CLZ(X))
#	endif
#endif

#define RTOS_MIN_STACK_SIZE 512 /* What is the real number? */

#endif

