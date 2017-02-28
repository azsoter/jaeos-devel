#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H
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

#if (defined(__VFP_FP__) && !defined(__SOFTFP__))
#define RTOS_TARGET_HAS_FPU 1
#endif

#define RTOS_INTERRUPT_CONTEXT_TRACKED_BY_HARDWARE_ONLY

// Type to store saved flags e.g. when we enter and exit critical state.
typedef uint32_t RTOS_Critical_State;

typedef uint32_t RTOS_StackItem_t;

struct rtos_m3_hardware_saved_registers
{
	RTOS_StackItem_t r0;
	RTOS_StackItem_t r1;
	RTOS_StackItem_t r2;
	RTOS_StackItem_t r3;
	RTOS_StackItem_t r12;
	RTOS_StackItem_t lr_app;
	RTOS_StackItem_t pc;
	RTOS_StackItem_t psr;
};

struct rtos_m3_software_saved_registers
{
	RTOS_StackItem_t basepri;
	RTOS_StackItem_t r4;
	RTOS_StackItem_t r5;
	RTOS_StackItem_t r6;
	RTOS_StackItem_t r7;
	RTOS_StackItem_t r8;
	RTOS_StackItem_t r9;
	RTOS_StackItem_t r10;
	RTOS_StackItem_t r11;
	RTOS_StackItem_t lr_int;
};

struct rtos_StackFrame
{
	struct rtos_m3_software_saved_registers swsaved;
	struct rtos_m3_hardware_saved_registers hwsaved;
};

typedef struct rtos_StackFrame rtos_StackFrame;

#if defined(RTOS_TARGET_HAS_FPU)
struct rtos_m4_hardware_saved_fpu_registers
{
	RTOS_StackItem_t s0;
	RTOS_StackItem_t s1;
	RTOS_StackItem_t s2;
	RTOS_StackItem_t s3;
	RTOS_StackItem_t s4;
	RTOS_StackItem_t s5;
	RTOS_StackItem_t s6;
	RTOS_StackItem_t s7;
	RTOS_StackItem_t s8;
	RTOS_StackItem_t s9;
	RTOS_StackItem_t s10;
	RTOS_StackItem_t s11;
	RTOS_StackItem_t s12;
	RTOS_StackItem_t s13;
	RTOS_StackItem_t s14;
	RTOS_StackItem_t s15;
	RTOS_StackItem_t fpscr;
};

struct rtos_m4_software_saved_fpu_registers
{
	RTOS_StackItem_t s16;
	RTOS_StackItem_t s17;
	RTOS_StackItem_t s18;
	RTOS_StackItem_t s19;
	RTOS_StackItem_t s20;
	RTOS_StackItem_t s21;
	RTOS_StackItem_t s22;
	RTOS_StackItem_t s23;
	RTOS_StackItem_t s24;
	RTOS_StackItem_t s25;
	RTOS_StackItem_t s26;
	RTOS_StackItem_t s27;
	RTOS_StackItem_t s28;
	RTOS_StackItem_t s29;
	RTOS_StackItem_t s30;
	RTOS_StackItem_t s31;
};

struct rtos_StackFrameWithFPU
{

	struct rtos_m3_software_saved_registers swsaved;
	struct rtos_m4_software_saved_fpu_registers swsavedFPU;
	struct rtos_m3_hardware_saved_registers hwsaved;
	struct rtos_m4_hardware_saved_fpu_registers hwsavedFPU;
};

typedef struct rtos_StackFrameWithFPU rtos_StackFrameWithFPU;
#endif

// This is needed when we create stack frames for tasks that haven't started yet.
#define RTOS_INITIAL_STACK_DEPTH (sizeof(rtos_StackFrame) + 8)


#if ((RTOS_Priority_Highest) > 31)
#	define RTOS_TASKSET_TYPE uint64_t
#	define RTOS_HIGHEST_SUPPORTED_TASK_PRIORITY 63
#	define RTOS_FIND_HIGHEST(X) (63 - __builtin_clzll((uint64_t)(X)))
#		define RTOS_TaskSet_NumberOfMembers(X) __builtin_popcountll((uint64_t)(X))
#else
#	if !defined(RTOS_FIND_HIGHEST)
#		define RTOS_FIND_HIGHEST(X) (31 - rtos_CLZ(X))
#		define RTOS_TaskSet_NumberOfMembers(X) __builtin_popcountl(X)
#	endif
#endif

#define RTOS_MIN_STACK_SIZE 512 /* What is the real number? */

#endif

