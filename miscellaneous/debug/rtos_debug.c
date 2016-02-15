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
#include <rtos_internals.h>
#include "board.h"
#include "rtos_debug.h"

void rtos_debug_putchar(char c)
{
	if ('\n' == c)
	{
		Board_Putc('\r');
	}
	Board_Putc(c);

}

void rtos_debug_PrintStr(const char *s)
{
	char c;
	while(0 != (c = *(s++)))
	{
		rtos_debug_putchar(c);
	}
}

void rtos_debug_PrintStrPadded(const char *s, int width)
{
	char c;

	while(width--)
	{
		c = *s;
		if (0 != c)
		{
			rtos_debug_putchar(c);
			s++;
		}
		else
		{
			rtos_debug_putchar(' ');
		}
	}
}

void rtos_debug_PrintHex(uint32_t x, int cr)
{
	static char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	char buff[10];
	int i;
	if (cr)
	{
		buff[8] = '\n';
		buff[9] = '\0';
	}
	else
	{
		buff[8] = ' ';
		buff[9] = '\0';
	}
	for (i = 0; i < 8; i++)
	{
		buff[7-i] = hexDigits[x & 0x0f];
		x >>= 4;
	}

	rtos_debug_PrintStr(buff);
}

#define RTOS_FIELD_WIDTH 32

void rtos_debug_PrintTask(const RTOS_Task *task)
{
	if (0 == task)
	{
		rtos_debug_PrintStr("Task: 0\n----------------------------------\n");
		return;
	}

	rtos_debug_PrintStrPadded("Task:", RTOS_FIELD_WIDTH);
	rtos_debug_putchar('(');
	rtos_debug_PrintHex((uint32_t)task, 0);
	rtos_debug_putchar(')');
	rtos_debug_putchar(' ');
#if defined(RTOS_TASK_NAME_LENGTH)
	rtos_debug_PrintStr((const char *)(task->TaskName));
#endif
	rtos_debug_putchar('\n');

	rtos_debug_PrintStrPadded("SP:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex((uint32_t)(task->SP), 0);
	rtos_debug_PrintStr(" Ret: "); rtos_debug_PrintHex(RTOS_TASK_EXEC_LOCATION(task), 1);
	rtos_debug_PrintStrPadded("SP0:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex((uint32_t)(task->SP0), 1);
	rtos_debug_PrintStrPadded("Priority:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(task->Priority, 1);

	rtos_debug_PrintStrPadded("WaitFor:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex((uint32_t)(task->WaitFor), 1);
	rtos_debug_PrintStrPadded("WakeUpTime:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(task->WakeUpTime, 1);


	// Structure representing a thread of execution (known as a task in RTOS parlance).

//		void            	(*Action)(void *);		// The main loop of the task.
//		void            	*Parameter;			// Parameter to Function().
#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_debug_PrintStrPadded("TicksToRun:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(task->TicksToRun, 1);
	// rtos_debug_PrintStrPadded("TimeSliceTicks:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(task->TimeSliceTicks, 1);

//		RTOS_Time		TimeWatermark;			// A time when the task was last seen running.
//		RTOS_Task_DLLink	FcfsLists[RTOS_LIST_WHICH_COUNT];
//		RTOS_RegInt		IsTimeshared;			// Is this task time sliced.
#endif
#if defined(RTOS_SMP)
	rtos_debug_PrintStrPadded("Cpu:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(task->Cpu, 1);
#endif
	rtos_debug_PrintStrPadded("Status:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(task->Status, 1);
#if defined(RTOS_TARGET_SPECIFIC_TASK_DATA)
//	RTOS_TARGET_SPECIFIC_TASK_DATA
#endif
	rtos_debug_PrintStr("----------------------------------\n");
}

void rtos_debug_PrintAllTasks(void)
{
	RTOS_TaskPriority pr;
	RTOS_Task *task;
	for (pr = RTOS_Priority_Idle; pr <= RTOS_Priority_Highest; pr++)
	{
		task = rtos_TaskFromPriority(pr);
		if (0 != task)
		{
			rtos_debug_PrintTask(task);
		}
	}
}

void rtos_debug_PrintOS(void)
{
#if defined(RTOS_SMP)
	int i;
#endif

	rtos_debug_PrintStr("\nRTOS:\n");

	rtos_debug_PrintStrPadded("RTOS.InterruptNesting:",RTOS_FIELD_WIDTH);

#if defined(RTOS_SMP)
	for (i = 0; i < (RTOS_SMP_CPU_CORES); i++)
	{
		rtos_debug_PrintHex(RTOS.InterruptNesting[i], 0);
	}
	rtos_debug_putchar('\n');
#else
	rtos_debug_PrintHex(RTOS.InterruptNesting, 1);
#endif
#if defined(RTOS_SMP)
	rtos_debug_PrintStrPadded("TasksAllowed:",RTOS_FIELD_WIDTH);
	for (i = 0; i < (RTOS_SMP_CPU_CORES); i++)
	{
		rtos_debug_PrintHex(RTOS.TasksAllowed[i], 0);
	}
	rtos_debug_putchar('\n');

	rtos_debug_PrintStrPadded("OSLock:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.OSLock, 1);
	rtos_debug_PrintStrPadded("RunningTasks:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.RunningTasks, 1);
	rtos_debug_PrintStrPadded("Cpus:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.Cpus, 1);
	rtos_debug_PrintStrPadded("CpuHoldingPen:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.CpuHoldingPen, 1);
#endif
#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
//	RTOS_RegUInt		SchedulerLocked;						// Is the scheduler locked.
#endif
//	RTOS_RegInt			IsRunning;								// Is the OS running?
	rtos_debug_PrintStrPadded("Time:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.Time, 1);
	rtos_debug_PrintStrPadded("ReadyToRunTasks:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.ReadyToRunTasks, 1);

#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
//	RTOS_TaskSet		SuspendedTasks;							// Suspended tasks.
#endif
#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_debug_PrintStrPadded("TimeshareTasks:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.TimeshareTasks, 1);
	rtos_debug_PrintStrPadded("PreemptedTasks:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.PreemptedTasks, 1);
//	RTOS_Task_DLList	PreemptedList;
#if defined(RTOS_SMP)
	rtos_debug_PrintStrPadded("TimeShareCpus:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.TimeShareCpus, 1);
	rtos_debug_PrintStrPadded("TimeshareParallelAllowed:",RTOS_FIELD_WIDTH); rtos_debug_PrintHex(RTOS.TimeshareParallelAllowed, 1);
#endif
#endif
//	RTOS_Task     		*TaskList[(RTOS_Priority_Highest) + 1];	// A list (really an array) of pointers to all the task structures.
#if defined(RTOS_SUPPORT_SLEEP)
//	RTOS_Task     		*Sleepers[(RTOS_Priority_Highest) + 1]; // All the sleeping tasks.
#endif
	rtos_debug_PrintStr("CurrentTask:\n");
#if defined(RTOS_SMP)
	for (i = 0; i < (RTOS_SMP_CPU_CORES); i++)
	{
		rtos_debug_PrintTask(RTOS.CurrentTasks[i]);
	}
#else
	rtos_debug_PrintTask(RTOS.CurrentTask);
#endif
}

void rtos_Debug(void)
{
	rtos_debug_PrintOS();
}
