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

// Start up code for the RaspberryPI target.
//
// A lot of this file was created based on ideas in various tutorials, and other examples and reference material from ARM and RaspberryPI
// resources on the Internet.
// I have included various references to the original sources.
// 
// References:
// http://www.raspberrypi.org/forums/viewtopic.php?t=60277&p=449903
// https://geekwentfreak-raviteja.rhcloud.com/blog/2011/01/16/writing-interrupt-routines-using-gcc-for-arm/

.extern rtos_Isr_Handler
.extern rtos_Invoke_Scheduler
.extern rtos_RunTask
.extern main
.extern __bss_start
.extern __bss_end
.section .init
.globl _start
 
_start:
	// Vector table / jump table.	
	ldr pc,reset_handler
	ldr pc,undefined_handler
    	ldr pc,swi_handler		// SWI also known as SVC handler. System call from user mode.
    	ldr pc,prefetch_handler
    	ldr pc,data_handler
    	ldr pc,not_used_handler
    	ldr pc,irq_handler
    	ldr pc,fiq_handler

// The actual table where the LDR instruction above load from.
reset_handler:
	.word Reset
undefined_handler:
	.word Undefined_instruction
swi_handler:
	.word rtos_Invoke_Scheduler
prefetch_handler:
	.word Prefetch_abort
data_handler:
	.word Data_abort
not_used_handler:
	.word Not_used
irq_handler:
	.word rtos_Isr_Handler
fiq_handler:
	.word Fiq_handler

Reset:

	// Set up stacks for for different CPU modes
    	// IRQ Mode.
	mov r1, #0x8000000
	mov r2, #4
	sub r1, r1, r2
	mov r2, #0x4000
    	mov r0,#0xD2
    	msr cpsr_c,r0
    	mov sp,r1

    	// FIQ Mode.
	sub r1,r1,r2
    	mov r0,#0xD1
    	msr cpsr_c,r0
    	mov sp,r1

    	// Supervisor Mode.
	sub r1,r1,r2
    	mov r0,#0xD3
    	msr cpsr_c,r0
    	mov sp,r1

	// System Mode.
	sub r1,r1,r2
    	mov r0,#0xDF
    	msr cpsr_c,r0
	mov sp,r1

	// Copy the vector tables from 0x8000 to 0x0000.
	// The entire table is 16 dwords, so multi-registers loads and stores
	// are used to copy it using only a few instructions.
	mov r0,#0x8000
    	mov r1,#0x0000
    	ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    	stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}
    	ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    	stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}

	// C programs set all uninitialized variables to 0s.
	// These are placed to the section .bss by the linker.
	// Here the start up code zeroes out .bss thus making sure that 
	// these variables do in fact have an initial value of 0.
	ldr r0, =__bss_start
	ldr r1, =__bss_end
	mov r2, #0
.Loop:
	cmp 	r0,r1
	strlo	r2,[r0], #4
	blo	.Loop
	nop
	nop
	nop

	// Flush everything.
	// http://www.raspberrypi.org/forums/viewtopic.php?f=72&t=13959
	MCR   p15, 0, ip, c7, c5, 0      // Invalidate I cache.
   	MCR   p15, 0, ip, c7, c5, 6      // Invalidate BTB.
   	MCR   p15, 0, ip, c7, c10, 4     // Drain write buffer.
   	MCR   p15, 0, ip, c7, c5, 4      // Prefetch flush.

	// Now that the run-time environment has been set up call the application's main() function.	
	b main

.section .text

Undefined_instruction:
	b Undefined_instruction

Prefetch_abort:
	b Prefetch_abort

Data_abort:
	b Data_abort

Not_used:
	B Not_used

Fiq_handler:
	b Fiq_handler

// The RTOS's task entry point.
.global rtos_TaskEntryPoint
rtos_TaskEntryPoint:
    bl	rtos_RunTask
.L01:
    b .L01

