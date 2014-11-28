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
#include <rtos.h>
#include <rtos_internals.h>
#include "raspi_peripherals.h"
#include <board.h>

// This file contains the bare minimum code needed to interface with the RaspberryPI hardware.
// Whether on bare metal or using an embedded operating system.
// It is mostly just a straight forward initialization based on data sheets and tutorials on the Internet.
//
// In addition to the permissions given or implied in the copyright notice above, permission is hereby granted to use
// the code in this particular file as a whole or portions of it without restrictions for any project related or unrelated
// to JaeOS with or without attribution to me.
// If you are working on anything that requires bare metal access on the RaspberryPI or any similar board, please feel free
// to cut and paste code from THIS FILE.
// This permission does not extend to other files that might be distributed or bundled together with this one.

// ---------------------------------------------------------------------------------------------------------------------------------------------
// Some rudimentary interrupt handling.
// This is a bare minimum implementation, with everything hard coded just to allow running JaeOS on a RasberryPI board.
// To learn more about how the interrupt controller on the RaspberryPI works consult the the datasheet "BCM2835 ARM Peripherals" from Boardcom.
// ---------------------------------------------------------------------------------------------------------------------------------------------
static int EnableInterruptSource(unsigned int interrupt)
{
	uint32_t enable_regs[] = { ARM_INTC_ENABLE1, ARM_INTC_ENABLE2, ARM_INTC_ENABLE_BASIC };
	uint32_t mask;
	unsigned int ix;

	if (interrupt > 71)
	{
		return -1;
	}

	ix = interrupt / 32;
	mask = ((uint32_t)1) << (interrupt % 32);

	*((volatile uint32_t *)(enable_regs[ix])) = mask;

	return 0;
}

static int DisableInterruptSource(unsigned int interrupt)
{
	uint32_t disable_regs[] = { ARM_INTC_DISABLE1, ARM_INTC_DISABLE2, ARM_INTC_DISABLE_BASIC };
	uint32_t mask;
	unsigned int ix;

	if (interrupt > 71)
	{
		return -1;
	}

	ix = interrupt / 32;
	mask = ((uint32_t)1) << (interrupt % 32);

	*((volatile uint32_t *)(disable_regs[ix])) = mask;

	return 0;
}

// ----------------------------------------------------------------------------------------------------------
// Handle the timer interrupt.
// Consult the datasheet "BCM2835 ARM Peripherals" from Boardcom for an explaination of the timer registers.
// ----------------------------------------------------------------------------------------------------------
static void board_HandleTimerInterrupt(void)
{
	rtos_TimerTick();
 	*(volatile uint32_t *)(TIMER_CLEAR) =  0;
}

// ----------------------------------------------------------------------------------------------------------
// Find out which interrupt line has caused the interrupt and dispatch it to the appropriate handler.
// ----------------------------------------------------------------------------------------------------------
void board_HandleIRQ(void)
{
	uint32_t pendingBits = *(volatile uint32_t *)(ARM_INTC_BASIC);
	uint32_t irq;

	if (0 != (0xFF & pendingBits))
	{
		irq = (64 + 31) - rtos_CLZ(pendingBits);
	}
	else if (0 != (0x100 & pendingBits))
	{
		irq = 31 - rtos_CLZ((*(volatile uint32_t *)(ARM_INTC_PENDING1)));
	}
	else if (0 != (0x200 & pendingBits))
	{
		irq = (32 + 31) - rtos_CLZ((*(volatile uint32_t *)(ARM_INTC_PENDING2)));
	}
	else
	{
		return;
	}

	switch(irq)
	{
		case ARM_IRQ_ID_TIMER_0:
			board_HandleTimerInterrupt();
			break;

		default:
			DisableInterruptSource(irq);
			break;
	}
}

// ----------------------------------------------------------------------------------------------------------
// Set up the timer to generate interrupts.
// Consult the datasheet "BCM2835 ARM Peripherals" from Boardcom for an explaination of the timer registers.
// ----------------------------------------------------------------------------------------------------------
static void TimerInit(void)
{
	*(volatile uint32_t *)(TIMER_CONTROL) =  0x003E0000;
 	*(volatile uint32_t *)(TIMER_LOAD) =  (1000000UL / (RTOS_TICKS_PER_SECOND));
 	*(volatile uint32_t *)(TIMER_RELOAD) =  (1000000UL / (RTOS_TICKS_PER_SECOND));
 	*(volatile uint32_t *)(TIMER_DIVIDER) =  0xF9UL;
 	*(volatile uint32_t *)(TIMER_CLEAR) =  0;
 	*(volatile uint32_t *)(TIMER_CONTROL) =  0x003E00A2;

	EnableInterruptSource(ARM_IRQ_ID_TIMER_0);
}
 
// ----------------------------------------------------------------------------------------------------------
// Some rudimentary initialization of the UART.
// Initialization code was created by following examples in tutorials on the Internet,
// and the rudimentary code here it does not use the full capabilities of the RaspberryPI board.
// For a source of information visit this website:
// http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C
// ----------------------------------------------------------------------------------------------------------

static void UartInit() {
    *(volatile uint32_t *)(UART0_CR) = 0;
    *(volatile uint32_t *)(GPPUD)= 0;
    *(volatile uint32_t *)(GPPUDCLK0) = 0xC000;
    *(volatile uint32_t *)(GPPUDCLK0) = 0;
    *(volatile uint32_t *)(UART0_ICR) = 0x7FF;
    *(volatile uint32_t *)(UART0_IBRD) = 1;
    *(volatile uint32_t *)(UART0_FBRD) = 40;
    *(volatile uint32_t *)(UART0_LCRH) = 0x70;
    *(volatile uint32_t *)(UART0_IMSC) = 0x07F2;
    *(volatile uint32_t *)(UART0_CR) = 0x0301;
}
 
void Board_Putc(char c)
{
	unsigned char byte = (unsigned char)c;
	// Wait for ready to transmit.
	while (0 != (((*(volatile uint32_t *)(UART0_FR)) & 0x20)));
	*(volatile uint32_t *)(UART0_DR) = byte;
}
 
void Board_Puts(const char *str)
{
	while (*str)
	{
		Board_Putc(*str++);
	}
}

char Board_Getc(void)
{

	// Wait for receive.
	while (0 != (((*(volatile uint32_t *)(UART0_FR)) & 0x10)));
	return (char)(*(volatile uint32_t *)(UART0_DR));
}

// -------------------------------------------------------------------------------------------------
int Board_HardwareInit(void)
{
	RTOS_DisableInterrupts();
	TimerInit();
	UartInit();
	return 0;
}

