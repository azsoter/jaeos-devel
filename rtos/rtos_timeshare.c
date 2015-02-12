#include <rtos.h>
#include <rtos_internals.h>

/*
* Copyright (c) Andras Zsoter 2014-2015.
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
void rtos_AppendToTaskDList(volatile RTOS_Task_DLList *list, RTOS_RegInt which, RTOS_Task *task)
{
	if (0 == list->Tail)
	{
		list->Head = task;
		list->Tail = task;
		task->FcfsLists[which].Previous = 0;
		task->FcfsLists[which].Next = 0;
	}
	else
	{
		task->FcfsLists[which].Previous = list->Tail;
		task->FcfsLists[which].Next = 0;
		list->Tail->FcfsLists[which].Next = task;
		list->Tail = task;
	}
}

void rtos_RemoveFromTaskDList(volatile RTOS_Task_DLList *list, RTOS_RegInt which, RTOS_Task *task)
{

	if (task == list->Head)
	{
		list->Head = task->FcfsLists[which].Next;
	}

	if (task == list->Tail)
	{
		list->Tail = task->FcfsLists[which].Previous;
	}

	if (0 != task->FcfsLists[which].Previous)
	{
		task->FcfsLists[which].Previous->FcfsLists[which].Next = task->FcfsLists[which].Next;
	}

	if (0 != task->FcfsLists[which].Next)
	{
		task->FcfsLists[which].Next->FcfsLists[which].Previous = task->FcfsLists[which].Previous;
	}

	task->FcfsLists[which].Previous = 0;
	task->FcfsLists[which].Next = 0;
}

RTOS_Task *rtos_RemoveFirstTaskFromDList(volatile RTOS_Task_DLList *list, RTOS_RegInt which)
{
	RTOS_Task *task = 0;

	if (0 != list->Head)
	{
		task = list->Head;
		list->Head = task->FcfsLists[which].Next;

		if (0 == list->Head)
		{
			list->Tail = 0;
		}
		else
		{
			list->Head->FcfsLists[which].Previous = 0;
		}

		task->FcfsLists[which].Previous = 0;
		task->FcfsLists[which].Next = 0;
	}

	return task;

}
// ----------
RTOS_RegInt rtos_MakeTimeshared(RTOS_Task *task, RTOS_Time slice)
{
	task->IsTimeshared = 1;
	task->TicksToRun = slice;
	task->TimeSliceTicks = slice;
	RTOS_TaskSet_AddMember(RTOS.TimeshareTasks, task->Priority);
	return RTOS_OK;
}

// This function can be called during initialization before the RTOS is fully functional.
RTOS_RegInt RTOS_CreateTimeShareTask(RTOS_Task *task, RTOS_TaskPriority priority, void *sp0, unsigned long stackCapacity, void (*f)(), void *param, RTOS_Time slice)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state = 0);

	if (RTOS.IsRunning)
	{
		RTOS_EnterCriticalSection(saved_state);
	}

	result = RTOS_CreateTask(task, priority, sp0, stackCapacity, f, param);

	if (RTOS_OK == result)
	{
		result = rtos_MakeTimeshared(task, slice);
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
	if (RTOS_SchedulerIsLocked())
	{
		return;
	}

	if (RTOS_TaskSet_IsMember(RTOS.ReadyToRunTasks, task->Priority))
	{
		RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, task->Priority);
		RTOS_TaskSet_AddMember(RTOS.PreemptedTasks, task->Priority);
		task->Status |= RTOS_TASK_STATUS_PREEMPTED_FLAG;
		rtos_AppendToTaskDList(&(RTOS.PreemptedList),  RTOS_LIST_WHICH_PREEMPTED, task);
	}
}

// This function should only be called by OS components inside a critical section.
// Schedule a task inside the peer group. The task was previously unscheduled because its time slice has expired.
void rtos_SchedulePeer(void)
{
	RTOS_Task *task;
	RTOS_TaskSet peersRunning;
	peersRunning = RTOS_TaskSet_Intersection(RTOS.ReadyToRunTasks, RTOS.TimeshareTasks);

	if (RTOS_SchedulerIsLocked())
	{
		return;
	}

	if (RTOS_TaskSet_IsEmpty(peersRunning))
	{
		task = rtos_RemoveFirstTaskFromDList(&(RTOS.PreemptedList),  RTOS_LIST_WHICH_PREEMPTED);
		if (0 != task)
		{
			RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, task->Priority);
			RTOS_TaskSet_RemoveMember(RTOS.PreemptedTasks, task->Priority);
			task->TicksToRun = task->TimeSliceTicks;
			task->TimeWatermark = RTOS.Time;
			task->Status &= ~((RTOS_RegInt)RTOS_TASK_STATUS_PREEMPTED_FLAG);
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
	if ((0 != task->TicksToRun) && ((RTOS_TIMEOUT_FOREVER) != task->TicksToRun))
	{
		task->TicksToRun--;
		task->TimeWatermark = RTOS.Time;
	}
}

void rtos_ManageTimeshared(RTOS_Task *task)
{
	if (task->IsTimeshared)
	{
		if (task->TimeWatermark != RTOS.Time)
		{
			rtos_DeductTick(task);
		}

		if (0 == task->TicksToRun)
		{
			rtos_PreemptTask(task);
			rtos_SchedulePeer();
		}
	}
}

#endif

