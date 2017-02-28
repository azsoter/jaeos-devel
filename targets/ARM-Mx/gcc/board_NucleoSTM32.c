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



#include <string.h>
#include <rtos_arm_mx.h>
#include <rtos.h>
#include <rtos_internals.h>
#include <board.h>

#include <usart.h>

#define BOARD_STDOUT &huart2

// Print something on the serial port that shows up when you plug in the Nucleo board
// to a PC through USB.
// These aren't a very efficient way to use the UART.
// Also, technically these are inappropriate to use from an ISR, but they do work.
// The main purpose of Board_Putc() and Board_Puts() is to provide some basic printing capability
// to be able to bring up the OS, so they need to be simple and rely on as few thing working as possible.
// For real life application one may want to create something more sophisticated and efficient.
int Board_Putc(char c)
{
	HAL_UART_Transmit(BOARD_STDOUT, &c, 1, 1000);
	return 0;
}

int Board_Puts(const char* str)
{
	HAL_UART_Transmit(BOARD_STDOUT, str, strlen(str), 1000);
	return 0;
}

// -------------------------------------------
static int uart_init(void)
{
	// It is already initialized at this point, so nothing to do here.
	 return 0;
}

// -----------------------------------------------------------------------------------------------------------
#if !defined(RTOS_USE_DEFAULT_ARM_Mx_SYSTICK_HANDLER)
// Replacement timer tick handler with HAL calls.
// In this setup HAL and the RTOS share the SysTick timer interrupt.
// BTW: On the ARM-Mx the use of a separate timer task is recommended.
extern void SysTick_Handler(void) __attribute__ ((isr));

void SysTick_Handler(void)
{
	RTOS_SavedCriticalState(saved_state);

	// Currently rtos_TimerTick() assumes that it will not be interrupted.
	// So run it with interrupts disabled.
	// To kep it short, use a timer task.
	RTOS_EnterCriticalSection(saved_state);
		rtos_TimerTick();
	RTOS_ExitCriticalSection(saved_state);
	__asm__ __volatile__ ("isb");

//	RTOS_EnterCriticalSection(saved_state); // Normally the HAL call should run with interrupts enabled.
		HAL_IncTick();
		HAL_SYSTICK_IRQHandler();
//	RTOS_ExitCriticalSection(saved_state);

	RTOS_SIGNAL_SCHEDULER_FROM_INTERRUPT();
}
#endif
// -----------------------------------------------------------------------------------------------------------
extern int rtos_CpuInit(void);
extern void SystemClock_Config(void);

// This needs to be defined in the application.
// Mostly stuff that CubeMX outputs to main().
// BTW: Initialization steps that need a working timer tick for time outs must be called
// later from a task after the RTOS is up and running.
extern int hal_hwinit(void);


int Board_HardwareInit(void)
{
	int res;

	__asm__ __volatile__ ("CPSID I");	// Master Disable interrupts.
	SystemClock_Config();
	res = rtos_CpuInit();
	hal_hwinit();
	res |= uart_init();
	return res;
}
