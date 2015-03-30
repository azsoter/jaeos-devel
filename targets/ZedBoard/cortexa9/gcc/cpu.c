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

#include <stdint.h>
#include <rtos.h>
#include <rtos_internals.h>
#include <xil_cache.h>

extern void rtos_Debug(void);

#if defined(RTOS_SMP)
#if (RTOS_SMP_CPU_CORES) > 2
#error This target only has two CPUs. RTOS_SMP_CPU_CORES must be <= 2.
#endif
#endif

// -------------------------------------------------------------------------------------------------------------------------------
// MULTI-CORE

#if defined(RTOS_SMP)
#define ARM_REG_ICCICR		0x0100
#define ARM_REG_ICCPMR		0x0104		/* (ICCPMR/ICCIPMR) */
#define ARM_REG_ICCIAR		0x010C
#define ARM_REG_ICCEOIR		0x0110

#define ARM_REG_ICDDCR		0x1000
#define ARM_REG_ICDISER		0x1100
#define ARM_REG_ICDSGIR		0x1F00

RTOS_INLINE void rtos_arm_WritePeripheralReg(uint32_t reg, uint32_t value)
{
	__asm__ __volatile__(
				"MRC     p15, 4, r1, c15, c0, 0\n"	/*   ; Peripheral base address */
				"ADD r1, r1, %0\n"
				"STR %1,[r1]\n" : : "r" (reg), "r" (value): "r1"
	);
}

RTOS_INLINE uint32_t rtos_arm_ReadPeripheralReg(uint32_t reg)
{
	uint32_t value;
	__asm__ __volatile__(
				"MRC     p15, 4, r1, c15, c0, 0\n"	/*   ; Peripheral base address */
				"ADD r1, r1, %1\n"
				"LDR %0,[r1]\n" : "=r" (value): "r" (reg): "r1"
	);

	return value;
}

void rtos_SignalAllCpus(RTOS_CpuMask cpus)
{
	uint32_t icdsgir;

	if (0 != cpus)
	{
		icdsgir = 0x00000001 | (((uint32_t) (0xFF & cpus)) << 16); // All CPUs specified.
		rtos_arm_WritePeripheralReg(ARM_REG_ICDSGIR, icdsgir);
	}
}

void rtos_SignalCpus(RTOS_CpuMask cpus)
{
	uint32_t icdsgir;

	if (0 != cpus)
	{
		icdsgir = 0x01000001 | (((uint32_t) (0xFF & cpus)) << 16); // All but the requester.
		rtos_arm_WritePeripheralReg(ARM_REG_ICDSGIR, icdsgir);
	}
}

void rtos_SignalCpu(RTOS_CpuId cpu)
{
	if (RTOS_CPUID_NO_CPU != cpu)
	{
		rtos_SignalCpus(1UL << cpu);
	}
}

#if 0
inline uint32_t rtos_LDREX(volatile uint32_t *address)
{
	uint32_t value;
	__asm__ __volatile__ ("LDREX   %0, [%1]" : "=r" (value) : "r" (address) :);
	return value;
}

inline uint32_t rtos_STREX(volatile uint32_t *address, uint32_t value)
{
	uint32_t res;
	__asm__ __volatile__ ("STREX   %0, %2, [%1]" : "=&r" (res) : "r" (address), "r" (value) :);
	return res;
}

// MRC p15, 0, <Rt>, c10, c2, 0
#define readPRRR() ({ uint32_t prrr_reg;	\
	__asm__ __volatile__ ("\tMRC     p15, 0, %0, c10, c2, 0\n": "=r" (prrr_reg) : : "cc"); \
	prrr_reg; \
})
#endif

#define LOCK_ID_MARKER 0x00008000

// Lock a mutex that prevents other CPUs from accessing a resource.
RTOS_RegInt volatile rtos_LockCpuMutex(volatile RTOS_CpuMutex *lock)
{
	RTOS_CpuId cpu = RTOS_CurrentCpu();
	uint32_t id = (LOCK_ID_MARKER) | cpu;

	__asm__ __volatile__(
	"1:"
			"LDREX r2, [%0]\n"
			"CMP r2, #0\n"
			"STREXEQ r2, %1, [%0]\n"
			"CMPEQ r2, #0\n"
			"WFENE\n"
			"BNE 1b\n"
			"DMB \n"
			: : "r" (lock), "r" (id) : "r2", "cc");

	return 0;
}

// Unlock a mutex that prevents other CPUs from accessing a resource.
RTOS_RegInt volatile rtos_UnlockCpuMutex(volatile RTOS_CpuMutex *lock)
{
	RTOS_CpuId cpu = RTOS_CurrentCpu();
	uint32_t id = (LOCK_ID_MARKER) | cpu;

	// xil_printf("UnlockMutex: lock=0x%x cpu=%d task=%d\r\n",*lock, cpu, RTOS_CURRENT_TASK()->Priority);
	if (*lock != id)
	{
		// outbyte('u');
		return RTOS_ERROR_FAILED;
	}

	__asm__ __volatile__ ("DMB");

	*lock = 0;

	__asm__ __volatile__ ("CLREX");
	__asm__ __volatile__ ("DSB");
	__asm__ __volatile__ ("SEV");

	return RTOS_OK;
}

#define RTOS_RESTORE_CONTEXT()	\
{ \
	__asm__ volatile ("LDR		R0, =RTOS");		/* The address of the RTOS structure.*/ 	\
	__asm__ volatile ("MRC		P15, 0, R1, C0, C0, 5"); /* Get CPU ID. */ \
	__asm__ volatile ("AND		R1, R1, #0x0f");	/* Mask out bits. */ \
	__asm__ volatile ("LDR		R0, [R0, R1, LSL #2]");		/* The Current Task structure pointer per CPU. */ \
	__asm__ volatile ("LDR		LR, [R0]");		/* The first field in the task structure is the stack pointer. */ \
	__asm__ volatile ("LDMFD	LR!, {R0}");		/* POP SPSR value to R0. */ \
	__asm__ volatile ("MSR		SPSR_cxsf, R0");	/* Restore SPSR. */ \
	__asm__ volatile ("LDMFD	LR, {R0-R14}^");	/* POP general purpose registers. */ \
	__asm__ volatile ("LDR		LR, [LR, #+60]");	/* Load return address to LR. */ \
	__asm__ volatile ("SUBS		PC, LR, #4");		/* Update program counter. */ \
}

#define RTOS_SAVE_CONTEXT()	\
{ \
	__asm__ volatile ("STMDB	SP!, {R0}");		/* Push R0. */ \
	__asm__ volatile ("STMDB	SP,{SP}^");		/* [SP - 4] <-- 'user mode SP'. */ \
	__asm__ volatile ("LDR		R0, [SP, #-4]");	/* R0 <-- [SP - 4] */ \
	__asm__ volatile ("STMDB	R0!, {LR}");		/* Push the return address.  */ \
	__asm__ volatile ("MOV		LR, R0");		/* LR <- R0 ('user mode SP')*/ \
	__asm__ volatile ("LDMIA	SP!, {R0}");		/* Pop R0. */ \
	__asm__ volatile ("STMDB	LR,{R0-LR}^");		/* Push all user mode registers. */ \
	__asm__ volatile ("SUB		LR, LR, #60");		/* Adjust LR. */ \
	__asm__ volatile ("MRS		R0, SPSR");		/* Read SPSR. */ \
	__asm__ volatile ("STMDB	LR!, {R0}");		/* Push the SPSR value. */ \
	__asm__ volatile ("LDR		R0, =RTOS");		/* The address of the RTOS structure. */ \
	__asm__ volatile ("MRC		P15, 0, R1, C0, C0, 5"); /* Get CPU ID. */ \
	__asm__ volatile ("AND		R1, R1, #0x0f");	/* Mask out bits. */ \
	__asm__ volatile ("LDR		R0, [R0, R1, LSL #2]");		/* The Current Task structure pointer per CPU. */ \
	__asm__ volatile ("STR		LR, [R0]");		/* Store the stack pointer in the task structure. */ \
}

// Hold CPUs that have nothing to do.
// CPU's that don't process real interrupts (not an SGI to trigger the scheduler) can sit here and wait for work.
static void rtos_CpuHoldingPenLoop(void)
{
	RTOS_CpuId cpu = RTOS_CurrentCpu();

	if (0 != RTOS.CurrentTasks[cpu])
	{
		return;
	}

	RTOS.CpuHoldingPen = RTOS_CpuMask_AddCpu(RTOS.CpuHoldingPen, cpu);

	while(0 == RTOS.CurrentTasks[cpu])
	{
		rtos_UnlockCpuMutex(RTOS_OS_LOCK);
		__asm__ __volatile__ ("WFE");		// Consume flag from SEV in the preceeding rtos_UnlockCpuMutex().
		__asm__ __volatile__ ("NOP");
		// outbyte(cpu ? 'l' : 'L');
		__asm__ __volatile__ ("WFE");		// Wait for an event coming from another CPU.
		rtos_LockCpuMutex(RTOS_OS_LOCK);
		rtos_Scheduler();
	}

	RTOS.CpuHoldingPen = RTOS_CpuMask_RemoveCpu(RTOS.CpuHoldingPen, cpu);
}


extern void rtos_InitCpu(void);
// #define COMM_VAL  (*(volatile unsigned long *)(0xFFFF0000))
extern void Xil_DCacheFlushLine(unsigned int adr);

#define CPU1_START_ADDR 0xfffffff0

void rtos_StartCpu()
{
	*((volatile uint32_t *)CPU1_START_ADDR) = (uint32_t)(&rtos_InitCpu);
	__asm__ __volatile__ ("DSB");
	Xil_DCacheFlushLine(CPU1_START_ADDR); // OCM is still cacheable!
	__asm__ __volatile__ ("SEV");

	while(!RTOS_CpuMask_IsCpuIncluded(RTOS.Cpus, 1))
	{
		__asm__ __volatile__ ("WFE");
		__asm__ __volatile__ ("DMB");
	}
}

void rtos_CPUxIsr(void)
{
	rtos_arm_WritePeripheralReg(ARM_REG_ICCEOIR, rtos_arm_ReadPeripheralReg(ARM_REG_ICCIAR));
}

void rtos_SecondaryCpu(void)
{
	RTOS_CpuId thisCpu = RTOS_CurrentCpu();
	rtos_StackFrame *currentStackFrame;
	uint32_t spsr;

	RTOS_DisableInterrupts();

	// Set up Software Generated Interrup (SGI) handling.
	rtos_arm_WritePeripheralReg(ARM_REG_ICCPMR, 0xF0);
	rtos_arm_WritePeripheralReg(ARM_REG_ICCICR, 0x07);
	rtos_arm_WritePeripheralReg(ARM_REG_ICDISER, 1);

	RTOS.Cpus = RTOS_CpuMask_AddCpu(RTOS.Cpus, thisCpu);

	__asm__ __volatile__ ("DSB");
	__asm__ __volatile__ ("SEV");
	__asm__ __volatile__ ("LDR R0, =0x1D3");			// Set SVC Mode.
	__asm__ __volatile__ ("MSR CPSR, R0");

	while (!RTOS.IsRunning)
	{
		__asm__ __volatile__ ("WFE");
		__asm__ __volatile__ ("DMB");
	}

	rtos_LockCpuMutex(RTOS_OS_LOCK);
	RTOS_CURRENT_TASK() =  0;
	rtos_CpuHoldingPenLoop();

	currentStackFrame =  (rtos_StackFrame *)(RTOS_CURRENT_TASK()->SP);
	spsr = currentStackFrame->spsr;

	if (0 == (0x80 & spsr))
	{
		rtos_UnlockCpuMutex(RTOS_OS_LOCK);	// Start executing task.
	}

	RTOS_RESTORE_CONTEXT();

	while(1) // Should never get here.
	{
		__asm__ __volatile__ ("WFI");
	}
}
// -------------------------------------------------------------------------------------------------------------------------------
#else /* RTOS_SMP not defined. */
#define RTOS_RESTORE_CONTEXT()	\
{ \
	__asm__ volatile ("LDR		R0, =RTOS");		/* The address of the RTOS structure.*/ 	\
	__asm__ volatile ("LDR		R0, [R0]");		/* The first address is a pointer to the Current Task structure. */ \
	__asm__ volatile ("LDR		LR, [R0]");		/* The first field in the task structure is the stack pointer. */ \
	__asm__ volatile ("LDMFD	LR!, {R0}");		/* POP SPSR value to R0. */ \
	__asm__ volatile ("MSR		SPSR_cxsf, R0");	/* Restore SPSR. */ \
	__asm__ volatile ("LDMFD	LR, {R0-R14}^");	/* POP general purpose registers. */ \
	__asm__ volatile ("LDR		LR, [LR, #+60]");	/* Load return address to LR. */ \
	__asm__ volatile ("SUBS		PC, LR, #4");		/* Update program counter. */ \
}

#define RTOS_SAVE_CONTEXT()	\
{ \
	__asm__ volatile ("STMDB	SP!, {R0}");		/* Push R0. */ \
	__asm__ volatile ("STMDB	SP,{SP}^");		/* [SP - 4] <-- 'user mode SP'. */ \
	__asm__ volatile ("LDR		R0, [SP, #-4]");	/* R0 <-- [SP - 4] */ \
	__asm__ volatile ("STMDB	R0!, {LR}");		/* Push the return address.  */ \
	__asm__ volatile ("MOV		LR, R0");		/* LR <- R0 ('user mode SP')*/ \
	__asm__ volatile ("LDMIA	SP!, {R0}");		/* Pop R0. */ \
	__asm__ volatile ("STMDB	LR,{R0-LR}^");		/* Push all user mode registers. */ \
	__asm__ volatile ("SUB		LR, LR, #60");		/* Adjust LR. */ \
	__asm__ volatile ("MRS		R0, SPSR");		/* Read SPSR. */ \
	__asm__ volatile ("STMDB	LR!, {R0}");		/* Push the SPSR value. */ \
	__asm__ volatile ("LDR		R0, =RTOS");		/* The address of the RTOS structure. */ \
	__asm__ volatile ("LDR		R0, [R0]");		/* The first field points to the Current task structure.*/ \
	__asm__ volatile ("STR		LR, [R0]");		/* Store the stack pointer in the task structure. */ \
}
#endif /* RTOS_SMP */

#if defined(RTOS_SMP)
void rtos_DispatchScheduler(void)
{
	RTOS_CpuId cpu = RTOS_CurrentCpu();
	rtos_StackFrame *currentStackFrame =  (rtos_StackFrame *)(RTOS.CurrentTasks[cpu]->SP);
	uint32_t code = *(uint32_t *)(currentStackFrame->regs[15] - 8); // The SWI instruction.


	RTOS.InterruptNesting[cpu]++;
	__asm__ __volatile__ ("DSB");

	uint32_t spsr = currentStackFrame->spsr;

	if (0 == (0x80 & spsr))
	{
		rtos_LockCpuMutex(RTOS_OS_LOCK);
	}

	// rtos_Debug();

	code = code & 0x00FFFFFF;

	switch(code)
	{
		case 0:	
			rtos_Scheduler();		// Run the scheduler.

			while (0 == RTOS.CurrentTasks[cpu])
			{
				rtos_CpuHoldingPenLoop();
			}

			RTOS_CpuMask otherCpus = RTOS_CpuMask_RemoveCpu(RTOS.Cpus, cpu);

			if (0 != otherCpus)
			{
				rtos_SignalCpus(otherCpus);
			}
			break;

		case 1:
			rtos_SchedulerForYield();	// Yield.

			while (0 == RTOS.CurrentTasks[cpu])
			{
				rtos_CpuHoldingPenLoop();
			}
			break;
		default:
			break;
	}

	currentStackFrame =  (rtos_StackFrame *)(RTOS.CurrentTasks[cpu]->SP);
	spsr = currentStackFrame->spsr;

	if (0 == (0x80 & spsr))
	{
		rtos_UnlockCpuMutex(RTOS_OS_LOCK);
	}

	RTOS.InterruptNesting[cpu]--;

	__asm__ __volatile__ ("DSB");
}

#else

void rtos_DispatchScheduler(void)
{
	rtos_StackFrame *currentStackFrame =  (rtos_StackFrame *)(RTOS_CURRENT_TASK()->SP);
	uint32_t code = *(uint32_t *)(currentStackFrame->regs[15] - 8); // The SWI instruction.

	RTOS.InterruptNesting++;

	code = code & 0x00FFFFFF;

	switch(code)
	{
		case 0:
			rtos_Scheduler();		// Run the scheduler.
			break;
		case 1:
			rtos_SchedulerForYield();	// Yield.
			break;
		default:
			break;
	}

	RTOS.InterruptNesting--;
}
#endif

void rtos_Invoke_Scheduler(void) __attribute__((interrupt("SWI"), naked));

void rtos_Invoke_Scheduler(void)
{
	// The return address after an SVC instruction is off by one instruction
	// compared to where it would be after an IRQ, adjust for that.
	__asm__ __volatile__ ( "ADD		LR, LR, #4" );
	RTOS_SAVE_CONTEXT();
	__asm__ __volatile__ ("CLREX");
	__asm__ __volatile__ ("bl rtos_DispatchScheduler");
	__asm__ __volatile__ ("CLREX");
	RTOS_RESTORE_CONTEXT();	
}

extern void IRQInterrupt(void);

#if defined(RTOS_SMP)
void rtos_Isr(void)
{
	RTOS_CpuId cpu = RTOS_CurrentCpu();
	RTOS_CpuMask otherCpus = 0;
	RTOS_TaskSet runnableTasks;
	rtos_StackFrame *currentStackFrame;
	uint32_t spsr;

	RTOS.InterruptNesting[cpu]++;

	__asm__ __volatile__ ("DSB");

	rtos_LockCpuMutex(RTOS_OS_LOCK);

	runnableTasks = RTOS.ReadyToRunTasks;

	// rtos_Debug();

	if (0 == cpu)
	{
		IRQInterrupt();
	}
	else
	{
		rtos_CPUxIsr();
	}

	rtos_Scheduler();

	while (0 == RTOS.CurrentTasks[cpu])
	{
		rtos_CpuHoldingPenLoop();
	}

	if (runnableTasks != RTOS.ReadyToRunTasks)
	{
		otherCpus = RTOS_CpuMask_RemoveCpu(RTOS.Cpus, cpu);		// All CPUs except the current one.
	}

	currentStackFrame =  (rtos_StackFrame *)(RTOS_CURRENT_TASK()->SP);
	spsr = currentStackFrame->spsr;

	if (0 == (0x80 & spsr))
	{
		rtos_UnlockCpuMutex(RTOS_OS_LOCK);
	}

	RTOS.InterruptNesting[cpu]--;
	__asm__ __volatile__ ("DSB");

	if (0 != otherCpus)
	{
		rtos_SignalCpus(otherCpus);
	}
}

#else
void rtos_Isr(void)
{
	RTOS.InterruptNesting++;
	IRQInterrupt();
	rtos_Scheduler();
	RTOS.InterruptNesting--;
}
#endif

void rtos_Isr_Handler(void) __attribute__((naked));

void rtos_Isr_Handler(void)
{
	RTOS_SAVE_CONTEXT();
	__asm__ __volatile__ ("CLREX");
	__asm volatile( "bl rtos_Isr" );
	__asm__ __volatile__ ("CLREX");
	RTOS_RESTORE_CONTEXT();	
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
extern void rtos_TaskEntryPoint(void);

void rtos_TargetInitializeTask(RTOS_Task *task, unsigned long stackCapacity)
{
	rtos_StackFrame *sp;
	
	// The stack pointer can legitimately be 0 at this point if the task is initialized as the 'current thread of execution'.
	if (0 != task->SP0)
	{
		task->SP = (void *)(((char *)task->SP0) + (sizeof(RTOS_StackItem_t) * stackCapacity) - (RTOS_INITIAL_STACK_DEPTH));
		sp = (rtos_StackFrame *)(task->SP);
		sp->spsr = 0x1F;
		sp->regs[15] = ((RTOS_StackItem_t)rtos_TaskEntryPoint) + 4;
		sp->regs[14] = 0x14; 
		sp->regs[13] = ((RTOS_StackItem_t)(task->SP)) + sizeof(rtos_StackFrame); 
		sp->regs[12] = 0x12; 
		sp->regs[11] = 0x11; 
		sp->regs[10] = 0x10; 
		sp->regs[9] = 0x9; 
		sp->regs[8] = 0x8; 
		sp->regs[7] = 0x7; 
		sp->regs[6] = 0x6; 
		sp->regs[5] = 0x5; 
		sp->regs[4] = 0x4; 
		sp->regs[3] = 0x3; 
		sp->regs[2] = 0x2; 
		sp->regs[1] = 0x1; 
		sp->regs[0] = 0x0;
	}
}

int RTOS_StartMultitasking(void)
{
	RTOS_ASSERT(0 != RTOS.TaskList[RTOS_Priority_Idle]);
#if defined(RTOS_SMP)
	 RTOS_CpuId thisCpu = RTOS_CurrentCpu();
	 RTOS.Cpus = RTOS_CpuMask_AddCpu(0, thisCpu);
	 // xil_printf("[%d] PRRR=%x\r\n", thisCpu, readPRRR());
	 *(RTOS_OS_LOCK) = 0;
	 // xil_printf("TA0 = %x TA1=%x\r\n", RTOS.TasksAllowed[0], RTOS.TasksAllowed[1]);
	 RTOS_RestrictTaskToCpus(RTOS.TaskList[RTOS_Priority_Idle], (1UL << thisCpu));
	 rtos_PrepareToStart();
	 __asm__ __volatile__ ("DSB");

#if (RTOS_SMP_CPU_CORES) > 1
	 RTOS_ASSERT(0 == thisCpu);
	 rtos_StartCpu();
#endif
#endif

	 RTOS_CURRENT_TASK() =  RTOS.TaskList[RTOS_Priority_Idle];	// Default to the Idle task.

#if defined(RTOS_SMP)
	 RTOS_TaskSet_AddMember(RTOS.RunningTasks, RTOS_Priority_Idle);
	 RTOS_CURRENT_TASK()->Cpu = thisCpu;
#endif

	// BTW: There is no need to enable interrupts here,
	// the task will have it enabled in their saved SPSR.
	RTOS.IsRunning = 1;								// Indicate that the OS is up and running.
	__asm__ volatile ("LDR R0, =0x1D3");			// Set SVC Mode.
	__asm__ volatile ("MSR CPSR, R0");
	// xil_printf("SCU_CONTROL_REGISTER=%x SCU_CONFIGURATION_REGISTER=%x\r\n", *(uint32_t *)0xF8F00000, *(uint32_t *)0xF8F00004);
#if defined(RTOS_SMP)
	__asm__ __volatile__ ("CLREX");
	__asm__ __volatile__ ("DSB");
	__asm__ __volatile__ ("SEV");
	rtos_SignalAllCpus(RTOS.Cpus);
#else
	rtos_Scheduler();								// Let the scheduler pick a higher priority task.
#endif

	RTOS_RESTORE_CONTEXT();					// Start running current task.
	return RTOS_ERROR_FAILED;				// We should never actually get here!
}

