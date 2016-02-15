/*
* Copyright (c) Andras Zsoter 2016.
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

#include "board.h"
#include <rtos.h>
#include <alt_16550_uart.h>
#include <alt_timers.h>
#include <alt_interrupt.h>
#include <alt_globaltmr.h>
// UART

#undef HOSTED

static ALT_16550_HANDLE_t uart_handle;
#define BAUD_RATE      (115200)

int Board_Putc(char c)
{
	ALT_STATUS_CODE status;
	status = alt_16550_fifo_write_safe(&uart_handle, &c, 1, 1);
	return (0 == status) ? 0 : -1;
}

int Board_Puts(const char *s)
{
	while (*s)
	{
		if (0 != Board_Putc(*s))
		{
			return -1;
		}
		s++;
	}

	return 0;
}

char Board_Getc(void)
{
	ALT_STATUS_CODE status;
	uint32_t level;
	char c;

	do
	{
		level = 0;
		status = alt_16550_fifo_level_get_rx(&uart_handle, &level);
	} while (0 == level);


	status = alt_16550_fifo_read(&uart_handle, &c, 1);

	return c;
}

int board_UartInit(void)
{
	ALT_STATUS_CODE status;
	unsigned long loc;

	status = alt_16550_init( ALT_16550_DEVICE_SOCFPGA_UART0, &loc, ALT_CLK_L4_SP, &uart_handle);
	status = alt_16550_baudrate_set(&uart_handle, BAUD_RATE);
    	status = alt_16550_line_config_set(&uart_handle, ALT_16550_DATABITS_8, ALT_16550_PARITY_DISABLE, ALT_16550_STOPBITS_1);
    	status = alt_16550_fifo_enable(&uart_handle);
    	status = alt_16550_enable(&uart_handle);
	return 0;
}

// ---------------------------------------- Timer ------------------------------------------------------------------------
extern void rtos_TimerTick(void);

// typedef void (*alt_int_callback_t)(uint32_t icciar, void * context);

void rtos_TimerTickHandler(uint32_t icciar, void *context)
{
	rtos_TimerTick();
	alt_gpt_int_clear_pending(ALT_GPT_CPU_PRIVATE_TMR);
}

static void board_TimerInit(void)
{
	alt_freq_t freq;

	alt_gpt_all_tmr_init();

	alt_clk_freq_get( ALT_CLK_MPU_PERIPH, &freq);

	alt_gpt_counter_set( ALT_GPT_CPU_PRIVATE_TMR, freq / (RTOS_TICKS_PER_SECOND));
	alt_gpt_time_microsecs_get(ALT_GPT_CPU_PRIVATE_TMR); 
	alt_gpt_mode_set(ALT_GPT_CPU_PRIVATE_TMR, ALT_GPT_RESTART_MODE_PERIODIC);
	alt_gpt_tmr_start(ALT_GPT_CPU_PRIVATE_TMR);

	alt_int_isr_register(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE, &rtos_TimerTickHandler, 0); 
	
	alt_int_dist_enable(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE);
	alt_gpt_int_clear_pending(ALT_GPT_CPU_PRIVATE_TMR);
	alt_gpt_int_enable(ALT_GPT_CPU_PRIVATE_TMR);

}
// ---------------------------------------- Interrupts ------------------------------------------------------------------------

extern void __cs3_interrupt_vector(void);
static void board_InterruptInit(void)
{
	ALT_STATUS_CODE status;

	uint32_t sctrl;
	void *vector_table_address = &__cs3_interrupt_vector;

	status = alt_int_global_init();
	PrintHex(status);

	__asm( "MRC p15, 0, %0, c1, c0, 0" : "=r" (sctrl));
	sctrl &= ~(((uint32_t)1 << 13));
	__asm( "MCR p15, 0, %0, c1, c0, 0" : : "r" (sctrl));
	__asm( "MCR p15, 0, %0, c12, c0, 0" : : "r" (vector_table_address));

	alt_int_cpu_binary_point_set( 0 );
	alt_int_cpu_priority_mask_set(0xff);

	alt_int_cpu_enable();
	alt_int_global_enable();
}
// ----------------------------------------------------------------------------------------------------------------------------
int Board_HardwareInit(void)
{
	int res;
	res = board_UartInit();
	Board_Puts("Initializing Interrupts.\r\n");
	board_InterruptInit();
	Board_Puts("Initializing Timer\r\n");
	board_TimerInit();
	return 0;
}

