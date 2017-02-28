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

#include <stdint.h>
#include <rtos.h>
#include <rtos_internals.h>

#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
RTOS_RegInt RTOS_SuspendTask(RTOS_Task *task)
{
	RTOS_RegInt result = RTOS_ERROR_FAILED;
	RTOS_TaskPriority priority;
	RTOS_SavedCriticalState(saved_state);
	RTOS_Task *currentTask;

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == task)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	RTOS_EnterCriticalSection(saved_state);

	currentTask = RTOS_CURRENT_TASK();

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
    	if ((RTOS_SchedulerIsLocked()) && (task == currentTask))
	{
		// Cannot Suspend the current task is the scheduler is locked.
		RTOS_ExitCriticalSection(saved_state);
		return RTOS_ERROR_FAILED;
	}
#endif

	priority = task->Priority;

	if (RTOS_TaskSet_IsMember(RTOS.SuspendedTasks, priority))
	{
		result = RTOS_OK;
	}

#if defined(RTOS_SMP)
	// If the task is running on this CPU it is OK to suspend it, but if it is running on another CPU we cannot suspend it.
	if (RTOS_TaskSet_IsMember(RTOS.RunningTasks, priority) && (task != currentTask))
	{
		RTOS_ExitCriticalSection(saved_state);
		return RTOS_ERROR_FAILED;
	}
#endif

#if defined(RTOS_SUPPORT_SLEEP)
	if ((RTOS_TASK_STATUS_WAITING == task->Status) || (RTOS_TASK_STATUS_SLEEPING == task->Status))
	{
		rtos_WakeupTask(task);
	}
#endif

	if (RTOS_TaskSet_IsMember(RTOS.ReadyToRunTasks, priority))
	{
		RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, priority);
		RTOS_TaskSet_AddMember(RTOS.SuspendedTasks, priority);
		task->Status |= RTOS_TASK_STATUS_SUSPENDED_FLAG;
		result = RTOS_OK;
	}

#if defined(RTOS_SUPPORT_TIMESHARE)
	if (RTOS_TaskSet_IsMember(RTOS.PreemptedTasks, priority))
	{
		RTOS_TaskSet_RemoveMember(RTOS.PreemptedTasks, priority);
		RTOS_TaskSet_AddMember(RTOS.SuspendedTasks, priority);
		task->Status = (task->Status & ~(RTOS_TASK_STATUS_PREEMPTED_FLAG)) | RTOS_TASK_STATUS_SUSPENDED_FLAG;
		rtos_RemoveTaskFromDLList(&(RTOS.PreemptedList), task);
		result = RTOS_OK;
	}
#endif

	RTOS_ExitCriticalSection(saved_state);

	RTOS_REQUEST_RESCHEDULING();

	return result;
}
#endif

