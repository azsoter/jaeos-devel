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
#include <vga.h>
#include <kbd.h>
#include <interrupts.h>

void Board_Putc(char c)
{
	VGA_PutChar(c);
}
 
void Board_Puts(const char *str)
{
	if (0 == str)
	{
		return;
	}
	while (*str)
	{
		Board_Putc(*str++);
	}
}

// Simple KBD Event handler, for realisting programs probably somethign using queues would be more appropriate, but queues are optional.
// We want this to work even if queues are not used.
#define KBD_BUFFER_DEPTH 32
static volatile KBD_Event_t Kbd_Buffer[KBD_BUFFER_DEPTH];
static volatile unsigned int Kbd_BufferHead;
static volatile unsigned int Kbd_BufferTail;

static unsigned int next_ix(unsigned int ix, unsigned int depth)
{
	ix += 1;
	if (ix >= depth)
	{
		ix = 0;
	}

	return ix;
}
 
void Board_KeyboardHandler(KBD_Event_t event)
{
	unsigned char c;
	unsigned int next_head;

	if (0 == (0x8000 & event))			// Only care about 'make' events.
	{
		if (0x53 == (0xFF & (event >> 8)))
		{
			if ((0 != (KBD_BIT_CONTROL_MASK & event)) && (0 != (KBD_BIT_ALT & event)))
			{
				// CTRL-ALT-DEL
				outb(0xfe, 0x64);
				while(1);
			}
		}
		c = (unsigned char)(0xFF & event);
		if (c != 0)				// Don't care about events here that just toggle control, shift and the like.
		{
			next_head = next_ix(Kbd_BufferHead, KBD_BUFFER_DEPTH);

			if (next_head != Kbd_BufferTail)	// If the buffer is already full we just loose events....
			{
				Kbd_Buffer[Kbd_BufferHead] = event;
				Kbd_BufferHead = next_head;
			}
		}
	}
}
 
char Board_Getc(void)
{
	char c;
	VGA_SyncCursor();
	while (Kbd_BufferHead == Kbd_BufferTail)
	{
		__asm__ volatile ("hlt");	// Really there is nothing else to do here, this is supposed to be simple low-level.
						// For any better solution we will need to use queues -- which is optional.
	}

	c = (char)(0xFF & Kbd_Buffer[Kbd_BufferTail]);
	Kbd_BufferTail = next_ix(Kbd_BufferTail, KBD_BUFFER_DEPTH);
	return c;
}

void Board_InitTimer(void)
{
	// Based on the description here:
	// https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interval_Timer 
	uint16_t div_factor = ((uint32_t)1193182) / (RTOS_TICKS_PER_SECOND);
	outb(0x36, 0x43);
	outb((uint8_t)(div_factor & 0xff), 0x40);
	outb((uint8_t)((div_factor >> 8) & 0xff), 0x40);
}

// -------------------------------------------------------------------------------------------------
int Board_HardwareInit(void)
{
	RTOS_DisableInterrupts();
	VGA_Initialize();
	Board_InitTimer();
	Kbd_BufferHead = 0;
	Kbd_BufferTail = 0;
	KBD_Init();
	InitInterrupts();
	return 0;
}

