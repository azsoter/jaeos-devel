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
#include <rtos.h>
#include <rtos_internals.h>
#include <board.h>
#include <kbd.h>
#include <vga.h>
#include "interrupts.h"

#undef DEBUG_INTERRUPTS

extern void PrintHex(uint32_t);
extern void PrintHexNoCr(uint32_t);

extern uint32_t _which_int;
extern uint16_t _idt[];

void Patch_IDT_Entry(int int_vector, uint32_t handler)
{
// Not using a struct for IDT entry, in case the compiler has a different idea about padding than humans do.
// Just patch the handler offset bytes.
	uint16_t *entry = &(_idt[int_vector * 4]);
	entry[0] = (uint16_t)(0xffff & handler);
	entry[3] = (uint16_t)(0xffff & (handler >> 16));

}

void GenericInterruptHandler(void) 
{
	VGA_Cls();
	Board_Puts("Interrupt:");
	PrintHexNoCr((unsigned char)(0xff & _which_int));
	MAGIC_BREAKPOINT();
	for(;;);
	// outb(0x20, 0x20);
}

// select: 0 -- Scheduler
// select: 1 -- Yield
void Invoke_Scheduler_Handler(uint32_t select) 
{
#if defined(DEBUG_INTERRUPTS)
	static uint32_t cnt = 0;
#endif
	RTOS.InterruptNesting++;
#if defined(DEBUG_INTERRUPTS)
	Board_Putc('\r');
	Board_Putc('\n');
	Board_Putc('>');
	PrintHexNoCr(cnt++);
	Board_Putc('#');
	PrintHexNoCr(select);
	Board_Putc('(');
	PrintHexNoCr(RTOS.ReadyToRunTasks);
	Board_Putc(')');
#endif
	if (0 == select)
	{
		rtos_Scheduler();
	}
	else
	{
		if (1 == select)
		{
			rtos_SchedulerForYield();
		}
	}
#if defined(DEBUG_INTERRUPTS)
	Board_Putc(':');
	Board_Puts(RTOS.CurrentTask->TaskName);
	Board_Putc(' ');
	PrintHexNoCr(RTOS.CurrentTask->SP);
	Board_Putc(' ');
	PrintHexNoCr( RTOS_TASK_EXEC_LOCATION(RTOS.CurrentTask));
	Board_Putc(' ');
	Board_Putc('I');
	PrintHex(RTOS.InterruptNesting);
	// PrintHex(RTOS.CurrentTask);
#endif
	RTOS.InterruptNesting--;
}

extern void Invoke_Scheduler_Wrapper(void);
__asm__(
	"Invoke_Scheduler_Wrapper:\n"
	"cli\n"
	"pusha\n"
	"movl (RTOS), %ebx\n"
	"movl %esp, (%ebx)\n"
	"pushl %eax\n"
	"call Invoke_Scheduler_Handler\n"
	"movl (RTOS), %ebx\n"
	"movl (%ebx), %esp\n"
	"popa\n"
	"iret\n"
);

#define INT_WRAPPER(WRAPPER_NAME, WRAPPED_FUNCTION)			\
	extern void (WRAPPER_NAME)(void);				\
	__asm__ (							\
	#WRAPPER_NAME ":\n"						\
	"cli\n"								\
	"pusha\n"							\
	"movl (RTOS), %ebx\n"						\
	"movl %esp, (%ebx)\n"						\
	"call " #WRAPPED_FUNCTION "\n"					\
	"movl (RTOS), %ebx\n"						\
	"movl (%ebx), %esp\n"						\
	"popa\n"							\
	"iret\n"							\
)

// Handle timer interrupts.
void Timer_Interrupt_Handler(void) 
{
	RTOS.InterruptNesting++;
	rtos_TimerTick();
	rtos_Scheduler();
#if defined(DEBUG_INTERRUPTS)
	Board_Putc('+');
	Board_Putc(' ');
	Board_Putc('i');
	PrintHex(RTOS.InterruptNesting);
#endif
	outb(0x20, 0x20);
	RTOS.InterruptNesting--;
}

// extern void Board_KeyboardHandler(KBD_Event_t event);

extern void Board_KeyboardHandler(KBD_Event_t event);

KBD_Event_t KBD_Handler(void)
{
	KBD_Event_t event;
	unsigned char code;
	unsigned char scancode;

	scancode = inb(0x60);

	event = KBD_ProcessScancode(scancode);
	code = (unsigned char)(event & 0xff);

#if defined(DEBUG_INTERRUPTS)
	if (0 != code)
	{
		if ('\n' == code)
		{
        		Board_Putc('\r');
        		Board_Putc('\n');
		}
		else
		{
        		Board_Putc(code);
		}
	}
#endif

	return event;
}

// Handle Keyboard Interrupts.
void Kbd_Interrupt_Handler(void)
{
	KBD_Event_t event;
	event = KBD_Handler();
	Board_KeyboardHandler(event);
	
#if 0
	if (('c' == (event & 0xFF)) || ('C' == (event & 0xFF)))
	{
		VGA_Cls();
	}
#endif
	rtos_Scheduler();
	outb(0x20, 0x20);
}

// Divide by zero -- it seems to have happened, so watch for it.
void Divide_by_Zero_Handler(void)
{
	Board_Puts("Division by Zero@"); PrintHex(RTOS_TASK_EXEC_LOCATION(RTOS.CurrentTask));
#if defined(RTOS_TASK_NAME_LENGTH)
	Board_Puts((const char *)(RTOS.CurrentTask->TaskName));
#else
	PrintHex((uint32_t)(RTOS.CurrentTask));
#endif
	Board_Putc(' ');
	Board_Putc('I');
	PrintHex(RTOS.InterruptNesting);
	MAGIC_BREAKPOINT();
	while(1);
}

INT_WRAPPER(Timer_Interrupt_Wrapper, Timer_Interrupt_Handler);
INT_WRAPPER(Kbd_Interrupt_Wrapper, Kbd_Interrupt_Handler);
INT_WRAPPER(Divide_by_Zero_Wrapper, Divide_by_Zero_Handler);

void InitInterrupts(void)
{
	Patch_IDT_Entry(0x00, (uint32_t)&Divide_by_Zero_Wrapper);
	Patch_IDT_Entry(0x40, (uint32_t)&Timer_Interrupt_Wrapper);
	Patch_IDT_Entry(0x41, (uint32_t)&Kbd_Interrupt_Wrapper);
	Patch_IDT_Entry(0x60, (uint32_t)&Invoke_Scheduler_Wrapper);

	// ---------------------------------------------------------
	outb(0x11, 0x20);	// ICW1 -- Begin initialization.
	outb(0x11, 0xa0);

	outb(0x40, 0x21);	// ICW2 -- Set Interrupt numbers.
	outb(0x48, 0xa1);

	outb(0x0, 0x21);	// ICW3 -- Cascading (?).
	outb(0x0, 0xa1);

	outb(0x01, 0x21);	// ICW4 -- Environment.
	outb(0x01, 0xa1);

	outb(0xff, 0x21);	// ICW0 -- Mask Interrupts.
	outb(0xff, 0xa1);
	// ---------------------------------------------------------
	outb(0xfc, 0x21);
	outb(0xff, 0xa1);
	// asm("sti");
}


