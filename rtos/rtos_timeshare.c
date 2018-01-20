#include <rtos.h>
#include <rtos_internals.h>

/*
* Copyright (c) Andras Zsoter 2014-2018.
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

#if defined(RTOS_SUPPORT_TIMESHARE)
void rtos_AppendTaskToDLList(volatile RTOS_Task_DLList *list, RTOS_Task *task)
{
	if (0 == list->Tail)
	{
		list->Head = task;
		list->Tail = task;
		task->Link.Previous = 0;
		task->Link.Next = 0;
	}
	else
	{
		task->Link.Previous = list->Tail;
		task->Link.Next = 0;
		list->Tail->Link.Next = task;
		list->Tail = task;
	}
}

void rtos_RemoveTaskFromDLList(volatile RTOS_Task_DLList *list, RTOS_Task *task)
{

	if (task == list->Head)
	{
		list->Head = task->Link.Next;
	}

	if (task == list->Tail)
	{
		list->Tail = task->Link.Previous;
	}

	if (0 != task->Link.Previous)
	{
		task->Link.Previous->Link.Next = task->Link.Next;
	}

	if (0 != task->Link.Next)
	{
		task->Link.Next->Link.Previous = task->Link.Previous;
	}

	task->Link.Previous = 0;
	task->Link.Next = 0;
}

RTOS_Task *rtos_RemoveFirstTaskFromDLList(volatile RTOS_Task_DLList *list)
{
	RTOS_Task *task = 0;

	if (0 != list->Head)
	{
		task = list->Head;
		list->Head = task->Link.Next;

		if (0 == list->Head)
		{
			list->Tail = 0;
		}
		else
		{
			list->Head->Link.Previous = 0;
		}

		task->Link.Previous = 0;
		task->Link.Next = 0;
	}

	return task;

}
// ----------
void rtos_MakeTimeshared(RTOS_Task *task, RTOS_Time slice)
{
	task->IsTimeshared = 1;
	task->TicksToRun = slice;
	task->TimeSliceTicks = slice;
}

// This function can be called during initialization before the RTOS is fully functional.
RTOS_RegInt RTOS_CreateTimeShareTask(RTOS_Task *task, const char *name, RTOS_TaskPriority priority, void *sp0, unsigned long stackCapacity, void (*f)(void *), void *param, RTOS_Time slice)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state = 0);

	result = rtos_CreateTask(task, sp0, stackCapacity, f, param);
#if defined(RTOS_TASK_NAME_LENGTH)
	RTOS_SetTaskName(task, name);
#else
	(void)name;
#endif
	if (RTOS.IsRunning)
	{
		RTOS_EnterCriticalSection(saved_state);
	}

	if (RTOS_OK == result)
	{
		rtos_MakeTimeshared(task, slice);
		result = rtos_RegisterTask(task, priority);
	}

	if (RTOS.IsRunning)
	{
		RTOS_ExitCriticalSection(saved_state);
	}

	return result;
}

// This function should only be called by OS components inside a critical section.
// Unschedule the task whose and place it on the list of the tasks whose time slice has expired.
void rtos_PreemptTask(RTOS_Task *task)
{
	RTOS_TaskPriority priority;

	if (RTOS_SchedulerIsLocked())
	{
		return;
	}

	priority = task->Priority;

	if (RTOS_TaskSet_IsMember(RTOS.ReadyToRunTasks, priority))
	{
		RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, priority);
		RTOS_TaskSet_AddMember(RTOS.PreemptedTasks, priority);
		rtos_AppendTaskToDLList(&(RTOS.PreemptedList), task);
	}
}

// This function should only be called by OS components inside a critical section.
// Schedule a task inside the peer group. The task was previously unscheduled because its time slice has expired.
void rtos_SchedulePeer(void)
{
	RTOS_Task *task;
	RTOS_TaskSet peersRunning;
	RTOS_TaskPriority priority;
	peersRunning = RTOS_TaskSet_Intersection(RTOS.ReadyToRunTasks, RTOS.TimeshareTasks);

#if defined(RTOS_SMP)
	if (RTOS_TaskSet_NumberOfMembers(peersRunning) < RTOS.TimeshareParallelAllowed)
#else
	if (RTOS_TaskSet_IsEmpty(peersRunning))
#endif
	{
		task = rtos_RemoveFirstTaskFromDLList(&(RTOS.PreemptedList));

		if (0 != task)
		{
			priority = task->Priority;
			RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, priority);
			RTOS_TaskSet_RemoveMember(RTOS.PreemptedTasks, priority);
			task->TicksToRun = task->TimeSliceTicks;
			task->TimeWatermark = RTOS.Time;
		}
	}
}

void RTOS_YieldTimeSlice(void)
{
	volatile RTOS_Task *thisTask;

	RTOS_SavedCriticalState(saved_state);

	thisTask = RTOS_CURRENT_TASK();

	if ((!RTOS_IsInsideIsr()) && (!RTOS_SchedulerIsLocked()) && (thisTask->IsTimeshared))
	{
		RTOS_EnterCriticalSection(saved_state);
		if (0 != RTOS.PreemptedList.Head)
		{
			rtos_PreemptTask(thisTask);
			rtos_SchedulePeer();
		}
		RTOS_ExitCriticalSection(saved_state);
		RTOS_INVOKE_SCHEDULER();
	}
}

void rtos_DeductTick(RTOS_Task *task)
{
	if ((0 != task->TicksToRun) && ((RTOS_TIMEOUT_FOREVER) != task->TicksToRun) && (task->TimeWatermark != RTOS.Time))
	{
		task->TicksToRun--;
		task->TimeWatermark = RTOS.Time;
	}
}

void rtos_ManageTimeshared(RTOS_Task *task)
{
	if (task)
	{
		if (task->IsTimeshared)
		{
			rtos_DeductTick(task);

			if (0 == task->TicksToRun)
			{
				if (RTOS_SchedulerIsLocked())
				{
					return;
				}

				rtos_PreemptTask(task);
			}
		}
	}

	rtos_SchedulePeer();
}

#endif

