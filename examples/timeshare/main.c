/*
* Copyright (c) Andras Zsoter 2014-2016.
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
#include <board.h>
#include <../utility.h>

volatile uint32_t dummy_var;

// Just burn CPU cycles.
void dly(uint32_t n)
{
	while(n)
	{
		dummy_var = n--;
	}
}

// Just keep printing a different character in each thread of execution and burn CPU cycles in between.
void f(void *p)
{

    while(1) 
    {
	Board_Putc(RTOS_GetCurrentTask()->TaskName[1]);
	dly(50000);
    }
    (void)p;
}

#define TASK_COUNT ((RTOS_Priority_LastPeer) + 1)
#define STACK_SIZE 1024

RTOS_Task tasks[RTOS_Priority_Highest + 1];
RTOS_StackItem_t stacks[RTOS_Priority_Highest + 1][STACK_SIZE];
#define DEFAULT_TIMESLICE 4

void initTasks(void)
{
	int i;
	char name[3] = { 'T', '?', 0 };

#if defined(RTOS_USE_TIMER_TASK)
	RTOS_CreateTask(&(tasks[RTOS_Priority_Timer]), "Timer", RTOS_Priority_Timer, &(stacks[RTOS_Priority_Timer]), STACK_SIZE, &RTOS_DefaultTimerFunction, 0);
#endif

	RTOS_CreateTask(&(tasks[0]), "Idle", RTOS_Priority_Idle, &(stacks[0]), STACK_SIZE, &RTOS_DefaultIdleFunction, 0);

	for (i = 1; i < TASK_COUNT; i++)
	{
		// All tasks in this example are allotted a different sized time slice.
		// As a result each will iterate a different number of times before it gets preempted
		// because its time slice has expired and print a different number of the unique digit in its name.
		name[1] = '0' + i;
    		RTOS_CreateTimeShareTask(&(tasks[i]), name, i, &(stacks[i]), STACK_SIZE, &f, 0, (2 * i) + 1);
	}
}

int main()
{
	initTasks();
	Board_HardwareInit();
	RTOS_StartMultitasking();
	Board_Puts("Something has gone wrong, we should not be here.\r\n");
    	while(1); 

    	return 0;
}

