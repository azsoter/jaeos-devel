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

.extern RTOS
.extern rtos_Scheduler
.extern rtos_SchedulerForYield
.globl _interrupt_handler
.globl rtos_restore_context
.globl rtos_Isr_Handler
.globl rtos_sysrequest
.global rtos_RunTask
.globl rtos_TaskEntryPoint

_interrupt_handler:
// Save context
	ADDIK	R1,  R1, -0x80			/* Update stack pointer. */
	SWI		R31, R1, 0x7C		/* PUSH general purpose registers. */
	SWI		R30, R1, 0x78		/* PUSH general purpose registers. */
	SWI		R29, R1, 0x74		/* PUSH general purpose registers. */
	SWI		R28, R1, 0x70		/* PUSH general purpose registers. */
	SWI		R27, R1, 0x6C		/* PUSH general purpose registers. */
	SWI		R26, R1, 0x68		/* PUSH general purpose registers. */
	SWI		R25, R1, 0x64		/* PUSH general purpose registers. */
	SWI		R24, R1, 0x60		/* PUSH general purpose registers. */
	SWI		R23, R1, 0x5C		/* PUSH general purpose registers. */
	SWI		R22, R1, 0x58		/* PUSH general purpose registers. */
	SWI		R21, R1, 0x54		/* PUSH general purpose registers. */
	SWI		R20, R1, 0x50		/* PUSH general purpose registers. */
	SWI		R19, R1, 0x4C		/* PUSH general purpose registers. */
	SWI		R18, R1, 0x48		/* PUSH general purpose registers. */
	SWI		R17, R1, 0x44		/* PUSH general purpose registers. */
//	SWI		R16, R1, 0x40		/* PUSH general purpose registers. */
	SWI		R15, R1, 0x3C		/* PUSH general purpose registers. */
	SWI		R14, R1, 0x38		/* PUSH general purpose registers. */
	SWI		R13, R1, 0x34		/* PUSH general purpose registers. */
	SWI		R12, R1, 0x30		/* PUSH general purpose registers. */
	SWI		R11, R1, 0x2C		/* PUSH general purpose registers. */
	SWI		R10, R1, 0x28		/* PUSH general purpose registers. */
	SWI		R9,  R1, 0x24		/* PUSH general purpose registers. */
	SWI		R8,  R1, 0x20		/* PUSH general purpose registers. */
	SWI		R7,  R1, 0x1C		/* PUSH general purpose registers. */
	SWI		R6,  R1, 0x18		/* PUSH general purpose registers. */
	SWI		R5,  R1, 0x14		/* PUSH general purpose registers. */
	SWI		R4,  R1, 0x10		/* PUSH general purpose registers. */
	SWI		R3,  R1, 0x0C		/* PUSH general purpose registers. */
	SWI		R2,  R1, 0x08		/* PUSH general purpose registers. */

	MFS		R3, RMSR			/* Get RMSR. */
	ORI		R3, R3,	0x02		/* Set EI so that restore will know what to to. */
	SWI		R3, R1, 0x00		/* PUSH RMSR */
	// ANDNI	R3,  R3,  0x02			/* Disable interrupts (clear IE). */
	LWI		R3, R0, RTOS		/* The address of the RTOS structure.*/
	SW		R1, R3, R0		/* The first field in the task structure is the stack pointer. */

	BRLID	R15, rtos_Isr_Handler
	OR		R0, R0, R0
rtos_restore_context:
	LWI		R3, R0, RTOS		/* The address of the RTOS structure.*/
	LW		R1, R0, R3			/* The first field in the task structure is the stack pointer. */
	LWI		R31, R1, 0x7C		/* POP general purpose registers. */
	LWI		R30, R1, 0x78		/* POP general purpose registers. */
	LWI		R29, R1, 0x74		/* POP general purpose registers. */
	LWI		R28, R1, 0x70		/* POP general purpose registers. */
	LWI		R27, R1, 0x6C		/* POP general purpose registers. */
	LWI		R26, R1, 0x68		/* POP general purpose registers. */
	LWI		R25, R1, 0x64		/* POP general purpose registers. */
	LWI		R24, R1, 0x60		/* POP general purpose registers. */
	LWI		R23, R1, 0x5C		/* POP general purpose registers. */
	LWI		R22, R1, 0x58		/* POP general purpose registers. */
	LWI		R21, R1, 0x54		/* POP general purpose registers. */
	LWI		R20, R1, 0x50		/* POP general purpose registers. */
	LWI		R19, R1, 0x4C		/* POP general purpose registers. */
	LWI		R18, R1, 0x48		/* POP general purpose registers. */
	LWI		R17, R1, 0x44		/* POP general purpose registers. */
//	LWI		R17, R1, 0x40		/* POP general purpose registers. */
	LWI		R15, R1, 0x3C		/* POP general purpose registers. */
	LWI		R14, R1, 0x38		/* POP general purpose registers. */
	LWI		R13, R1, 0x34		/* POP general purpose registers. */
	LWI		R12, R1, 0x30		/* POP general purpose registers. */
	LWI		R11, R1, 0x2C		/* POP general purpose registers. */
	LWI		R10, R1, 0x28		/* POP general purpose registers. */
	LWI		R9,  R1, 0x24		/* POP general purpose registers. */
	LWI		R8,  R1, 0x20		/* POP general purpose registers. */
	LWI		R7,  R1, 0x1C		/* POP general purpose registers. */
	LWI		R6,  R1, 0x18		/* POP general purpose registers. */
	LWI		R5,  R1, 0x14		/* POP general purpose registers. */
	LWI		R4,  R1, 0x10		/* POP general purpose registers. */

	LWI		R2,  R1, 0x00		/* POP RMSR. */
	ANDNI	R3,  R2, 0x02		/* Clear IE. */
	ANDI	R2,	R2,	0x02
	BEQI	R2, .return_from_softrequest
	OR		R0,	R0,	R0
.return_from_interrupt:
	MTS		RMSR, R3			/* Update RMSR. */
	LWI		R3,  R1, 0x0C		/* POP general purpose registers. */
	LWI		R2,  R1, 0x08		/* POP general purpose registers. */
	ADDIK	R1, R1, 0x80		/* Update stack pointer. */
	RTID	R14, 0				/* Return from Interrupt. */
	OR		R0, R0, R0

.return_from_softrequest:
	MTS		RMSR, R3			/* Update RMSR. */
	LWI		R3,  R1, 0x0C		/* POP general purpose registers. */
	LWI		R2,  R1, 0x08		/* POP general purpose registers. */

	ADDIK	R1, R1, 0x80		/* Update stack pointer. */
	RTSD	R15, 8				/* Return from function. */
	OR		R0, R0, R0

rtos_sysrequest:
// Save context
	ADDIK	R1, R1, -0x80			/* Update stack pointer. */
	SWI		R31, R1, 0x7C		/* PUSH general purpose registers. */
	SWI		R30, R1, 0x78		/* PUSH general purpose registers. */
	SWI		R29, R1, 0x74		/* PUSH general purpose registers. */
	SWI		R28, R1, 0x70		/* PUSH general purpose registers. */
	SWI		R27, R1, 0x6C		/* PUSH general purpose registers. */
	SWI		R26, R1, 0x68		/* PUSH general purpose registers. */
	SWI		R25, R1, 0x64		/* PUSH general purpose registers. */
	SWI		R24, R1, 0x60		/* PUSH general purpose registers. */
	SWI		R23, R1, 0x5C		/* PUSH general purpose registers. */
	SWI		R22, R1, 0x58		/* PUSH general purpose registers. */
	SWI		R21, R1, 0x54		/* PUSH general purpose registers. */
	SWI		R20, R1, 0x50		/* PUSH general purpose registers. */
	SWI		R19, R1, 0x4C		/* PUSH general purpose registers. */
	SWI		R18, R1, 0x48		/* PUSH general purpose registers. */
	SWI		R17, R1, 0x44		/* PUSH general purpose registers. */
//	SWI		R16, R1, 0x40		/* PUSH general purpose registers. */
	SWI		R15, R1, 0x3C		/* PUSH general purpose registers. */
	SWI		R14, R1, 0x38		/* PUSH general purpose registers. */
	SWI		R13, R1, 0x34		/* PUSH general purpose registers. */
	SWI		R12, R1, 0x30		/* PUSH general purpose registers. */
	SWI		R11, R1, 0x2C		/* PUSH general purpose registers. */
	SWI		R10, R1, 0x28		/* PUSH general purpose registers. */
	SWI		R9,  R1, 0x24		/* PUSH general purpose registers. */
	SWI		R8,  R1, 0x20		/* PUSH general purpose registers. */
	SWI		R7,  R1, 0x1C		/* PUSH general purpose registers. */
	SWI		R6,  R1, 0x18		/* PUSH general purpose registers. */
	SWI		R5,  R1, 0x14		/* PUSH general purpose registers. */
	SWI		R4,  R1, 0x10		/* PUSH general purpose registers. */
	SWI		R3,  R1, 0x0C		/* PUSH general purpose registers. */
	SWI		R2,  R1, 0x08		/* PUSH general purpose registers. */

	MFS		R3, RMSR			/* Get RMSR. */
	SWI		R3,  R1, 0x00		/* PUSH RMSR */

	LWI		R3, R0, RTOS		/* The address of the RTOS structure.*/
	SW		R1, R3, R0			/* The first field in the task structure is the stack pointer. */

	OR		R5, R5, R5
	BNEI	R5, .yield			/* Is the parameter 0? (call scheduler)? */
	OR		R0,	R0,	R0

	BRLID	R15, rtos_Scheduler
	OR		R0,	R0,	R0
	BRI		rtos_restore_context
	OR		R0,	R0,	R0
.yield:
	XORI	R5, R5, 1			/* Is the parameter 1? (call Yield Scheduler)? */
	BNEI	R5, .neither
	OR		R0,	R0,	R0

	BRLID	R15, rtos_SchedulerForYield
	OR		R0,	R0,	R0
	BRI		rtos_restore_context
	OR		R0,	R0,	R0
.neither:
	BRI		rtos_restore_context
	OR		R0,	R0,	R0

rtos_TaskEntryPoint:
	BRLID	R15, rtos_RunTask
	OR		R0,	R0,	R0
.L1:
	OR		R0,	R0,	R0
	BRI		.L1
	OR		R0,	R0,	R0
