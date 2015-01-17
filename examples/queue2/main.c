/*
* Copyright (c) Andras Zsoter 2014, 2015.
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
#include <rtos_queue.h>
#include <board.h>
#include "../utility.h"

RTOS_StackItem_t stack1[512];
RTOS_StackItem_t stack2[512];
RTOS_StackItem_t stack3[512];
RTOS_StackItem_t stackIdle[RTOS_MIN_STACK_SIZE];

RTOS_Task task1;
RTOS_Task task2;
RTOS_Task task3;
RTOS_Task task_idle;

RTOS_Semaphore s;

char c = '*';

RTOS_Queue q0;
RTOS_Queue q1;

// Take an item off a queue.
// Print it and push it into another queue is a LIFO manner.
void Consumer1(void *p)
{
    RTOS_RegInt res;
    void *goodie;
    (void)p;
    
    while(1) 
    {
	res = RTOS_Dequeue(&q0, &goodie, RTOS_TIMEOUT_FOREVER);
	// PrintHex(res);
	if (RTOS_OK == res)
	{
		Board_Puts(RTOS_GetTaskName(RTOS_CURRENT_TASK()));
		Board_Puts(":0x");
		PrintHex((uint32_t)goodie);
		Board_Putc('\r');
		Board_Putc('\n');
		while (RTOS_OK != RTOS_PrependQueue(&q1, (void *)(goodie), 0))
		{
			RTOS_Delay(10);
			// RTOS_YieldPriority();
		}
	}  
    }
}

// Take an item off a queue.
// Just print it.
void Consumer2(void *p)
{
    RTOS_RegInt res;
    void *goodie;
    (void)p;
    
    while(1) 
    {
	res = RTOS_Dequeue(&q1, &goodie, RTOS_TIMEOUT_FOREVER);
	// PrintHex(res);
	if (RTOS_OK == res)
	{
		Board_Puts(RTOS_GetTaskName(RTOS_CURRENT_TASK()));
		Board_Puts(":0x");
		PrintHex((uint32_t)goodie);
		Board_Putc('\r');
		Board_Putc('\n');
		// RTOS_YieldPriority();
	}  
    }
}

#define BUFFER_SIZE 5
void *buff0[BUFFER_SIZE];
void *buff1[BUFFER_SIZE];

// Place an item into the queue.
void Producer(void *p)
{
    uint32_t stuff = 0;
    (void)p;
    RTOS_CreateQueue(&q0, buff0, BUFFER_SIZE);
    RTOS_CreateQueue(&q1, buff1, BUFFER_SIZE);

    while(1) 
    {
	stuff++;
	Board_Puts(RTOS_GetTaskName(RTOS_CURRENT_TASK()));
	Board_Puts(":0x");
	PrintHex(stuff);
	Board_Putc('\r');
	Board_Putc('\n');
	RTOS_Enqueue(&q0, (void *)(stuff), RTOS_TIMEOUT_FOREVER);
    }
}

extern void HardwareInit(void);


int main()
{
    	RTOS_CreateTask(&task1,     RTOS_Priority_Producer, stack1, 512, &Producer, 0);
	RTOS_SetTaskName(&task1, "Producer");
    	RTOS_CreateTask(&task2,     RTOS_Priority_Consumer1, stack2, 512, &Consumer1, 0);
	RTOS_SetTaskName(&task2, "Consumer1");
    	RTOS_CreateTask(&task3,     RTOS_Priority_Consumer2, stack3, 512,  &Consumer2, 0);
	RTOS_SetTaskName(&task3, "Consumer2");
    	RTOS_CreateTask(&task_idle, RTOS_Priority_Idle,  stackIdle, RTOS_MIN_STACK_SIZE, &RTOS_DefaultIdleFunction, 0);
	RTOS_SetTaskName(&task_idle, "Idle");

	Board_HardwareInit();
	RTOS_StartMultitasking();
	Board_Puts("Something has gone seriously wrong!");
    	while(1);
    	return 0;	// Unreachable.
}

