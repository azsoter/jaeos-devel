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

.org 0
.text
.extern rtos_Isr_Handler
.extern Invoke_Scheduler
.extern RTOS_RunTask
.extern PrefetchAbortInterrupt
.extern DataAbortInterrupt
.extern _boot
.section .vectors
.globl _vector_table

.section .vectors 
_vector_table:
	// Vector table / jump table.
	ldr pc,reset_handler
	ldr pc,undefined_handler
    	ldr pc,swi_handler		// SWI also known as SVC handler. System call from user mode.
    	ldr pc,prefetch_handler
    	ldr pc,data_handler
    	ldr pc,unused_handler
    	ldr pc,irq_handler
    	ldr pc,fiq_handler

// The actual table where the LDR instruction above load from.
reset_handler:
	.word _boot
undefined_handler:
	.word Undefined_instruction
swi_handler:
	.word rtos_Invoke_Scheduler
prefetch_handler:
	.word Prefetch_abort
data_handler:
	.word Data_abort
unused_handler:
	.word Unused
irq_handler:
	.word rtos_Isr_Handler
fiq_handler:
	.word Fiq_Handler

.section .text

Undefined_instruction:
	b Undefined_instruction
	nop

Prefetch_abort:
	dsb
	stmdb	sp!,{r0-r3,r12,lr}
	bl	PrefetchAbortInterrupt
	ldmia	sp!,{r0-r3,r12,lr}
	subs	pc, lr, #4
	nop

Data_abort:
	dsb
	stmdb	sp!,{r0-r3,r12,lr}
	bl	DataAbortInterrupt
	ldmia	sp!,{r0-r3,r12,lr}
	subs	pc, lr, #8	
	nop

Unused:
	B Unused
	nop

Fiq_Handler:
	b Fiq_Handler
	nop

.global rtos_TaskEntryPoint
rtos_TaskEntryPoint:
    bl	rtos_RunTask
.L01:
    b .L01
	nop

