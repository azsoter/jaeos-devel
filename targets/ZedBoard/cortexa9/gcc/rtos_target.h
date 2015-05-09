#ifndef RTOS_TARGET_H
#define RTOS_TARGET_H

/*
* Copyright (c) Andras Zsoter 2014-2015.
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
	__asm__ __volatile__ ( 									\
		"\tMRS\tR1, CPSR			\n"	 /* R1 <- CPSR.	*/		\
		"\tMOV\t%0, R1				\n"	 /* Save CPSR.	*/		\
		"\tORR\tR1, R1, #0xC0			\n"	 /* Disable IRQ, FIQ. */	\
		"\tMSR\tCPSR, R1			\n"	 /* CPSR <- R1	*/		\
	 : "=r" (SAVED) : : "r1","cc")

#define rtos_restoreInterrupts(SAVED) 								\
	__asm__ __volatile__ ( 									\
		"\tMSR\tCPSR, %0 \n"	/*  CPSR <- saved_CPSR. */				\
	 : : "r" (SAVED) : "cc")

#define rtos_readCPSR() ({ unsigned long rtos_tmp;  __asm__ __volatile__ ("MRS\t%0, CPSR": "=r" (rtos_tmp)); rtos_tmp; })

#define RTOS_IsInsideIsr() (0x1f != (0x1f & rtos_readCPSR()))

#define RTOS_SavedCriticalState(X) 	RTOS_Critical_State X

#if defined(RTOS_SMP)
extern RTOS_RegInt volatile rtos_LockCpuMutex(volatile RTOS_CpuMutex *lock);
extern RTOS_RegInt volatile rtos_UnlockCpuMutex(volatile RTOS_CpuMutex *lock);

#define RTOS_OS_LOCK  (&(RTOS.OSLock))
// #define RTOS_OS_LOCK  ((volatile RTOS_CpuMutex *)&(RTOS.OSLock))
// #define RTOS_OS_LOCK ((volatile unsigned long *)(0xFFFF0004))

#if 1
extern RTOS_Critical_State rtos_ARM_SMP_EnterCriticalSection(volatile RTOS_CpuMutex *lock);

#define RTOS_EnterCriticalSection(X) (X) = rtos_ARM_SMP_EnterCriticalSection(RTOS_OS_LOCK)

#else
#define RTOS_EnterCriticalSection(X) \
		do { rtos_disableInterrupts(X); if (0 == (0x80 & (X))) rtos_LockCpuMutex(RTOS_OS_LOCK); } while(0)
#endif

#define RTOS_ExitCriticalSection(X)	\
	do { if (0 == (0x80 & (X))) rtos_UnlockCpuMutex(RTOS_OS_LOCK); rtos_restoreInterrupts(X); } while(0)
#else
#define RTOS_EnterCriticalSection(X)	rtos_disableInterrupts(X)
#define RTOS_ExitCriticalSection(X)	rtos_restoreInterrupts(X)
#endif


#define RTOS_EnableInterrupts() rtos_enableInterrupts()
#define RTOS_DisableInterrupts() do { RTOS_Critical_State rtos_tmp_saved; rtos_disableInterrupts(rtos_tmp_saved); } while(0)


// extern int _write(int fd, void *prt, unsigned int count);

#define RTOS_INLINE static inline

#define rtos_CLZ(X) __builtin_clzl(X)

#if 0
#define rtos_CTZ(X) ({uint32_t res;					\
	__asm__ __volatile__ (							\
			"rsb	r2, %1, #0\n"					\
			"ands	r1,%1,r2\n"						\
			"clz	%0,r1\n"						\
			"rsbne	%0, %0, #31"					\
			:"=r" (res):"r" (X):"r2", "r1", "cc");	\
	res;})
#endif

#define RTOS_INVOKE_SCHEDULER() __asm volatile ( "SWI 0" ) 
#define RTOS_INVOKE_YIELD() __asm volatile ( "SWI 1" ) 

#if defined(RTOS_SMP)

#define RTOS_CurrentCpu() ({ RTOS_CpuId rtos__current_cpuid;	\
	__asm__ __volatile__ ("\tMRC     p15, 0, %0, c0, c0, 5\n"	"\tAND     %0, %0, #0x0f\n" : "=r" (rtos__current_cpuid) : : "cc"); \
	rtos__current_cpuid; \
})

#define RTOS_CpuMask_AddCpu(MASK, CPU) ((MASK) | (1UL << (CPU)))
#define RTOS_CpuMask_RemoveCpu(MASK, CPU) ((MASK) & (~(1UL << (CPU))))
#define RTOS_CpuMask_IsCpuIncluded(MASK, CPU) (0 != ((MASK) & (1UL << (CPU))))

extern void rtos_SignalCpu(RTOS_CpuId cpu);
extern void rtos_SignalCpus(RTOS_CpuMask cpus);
#endif

// For debugging:
#define rsp()   ({ unsigned long rtos_tmp;  asm ("MOV\t%0, SP":   "=r" (rtos_tmp)); rtos_tmp; })
#define rcpsr() ({ unsigned long rtos_tmp;  asm ("MRS\t%0, CPSR": "=r" (rtos_tmp)); rtos_tmp; })

#define RTOS_TASK_EXEC_LOCATION(TASK) ((rtos_StackFrame *)((TASK)->SP))->regs[15]

// Utility functions.
#define RTOS_DEFAULT_IDLE_FUNCTION RTOS_DefaultIdleFunction
extern void RTOS_DefaultIdleFunction(void *p);

extern int RTOS_StartMultitasking(void);

#endif

