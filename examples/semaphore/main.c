/*
* Copyright (c) Andras Zsoter 2015-2016.
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

#include <rtos.h>
#include <board.h>

#define STACK_SIZE 512
RTOS_StackItem_t stack[2][STACK_SIZE];
RTOS_StackItem_t stack_idle[RTOS_MIN_STACK_SIZE];

RTOS_Task task[2];
RTOS_Task task_idle;

RTOS_Semaphore s;

void high_prio(void *p)
{
	RTOS_RegInt res;

	while(1)
	{
		res = RTOS_GetSemaphore(&s, RTOS_TIMEOUT_FOREVER);
		if (RTOS_OK == res)
		{
			Board_Putc('+');
		}
		else
		{
			Board_Putc('-');
		}
		
	}
	(void)p;	// Pacifier for the compiler.
}

void low_prio(void *p)
{
    while(1) 
    {
	RTOS_PostSemaphore(&s);
	Board_Putc('>');
	RTOS_Delay(RTOS_MS_TO_TICKS(1000));
	
    }
    (void)p;	// Pacifier for the compiler.
}


int main()
{
    	RTOS_CreateTask(&task_idle, "Idle",   RTOS_Priority_Idle,  stack_idle, RTOS_MIN_STACK_SIZE, &RTOS_DefaultIdleFunction, 0);
    	RTOS_CreateTask(&(task[0]), "Task1",  RTOS_Priority_Task1, stack[0], STACK_SIZE, &low_prio, 0);
    	RTOS_CreateTask(&(task[1]), "Task2",  RTOS_Priority_Task2, stack[1], STACK_SIZE, &high_prio, 0);

	RTOS_CreateSemaphore(&s, 0);

	Board_HardwareInit();					// Initialize hardware as appropriate for the system/board.

	RTOS_StartMultitasking();
	Board_Puts("Something has gone seriously wrong!\r\n");	// We should never get here!
    	while(1);
    	return 0;						// Unreachable.
}

