/*
* Copyright (c) Andras Zsoter 2015-2017.
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

RTOS_RegInt RTOS_ChangePriority(RTOS_Task *task, RTOS_TaskPriority targetPriority)
{

	RTOS_TaskPriority oldPriority;
	RTOS_RegInt result = RTOS_OK;
#if defined(RTOS_SMP)
	int i;
#endif

	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
	RTOS_ASSERT((targetPriority <= RTOS_Priority_Highest) && (RTOS_Priority_Idle != targetPriority));
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
    	if (0 == task)
    	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
    	}
#endif

 	oldPriority = task->Priority;

	if (oldPriority == targetPriority)
	{
		return RTOS_OK;
	}

	if ((targetPriority > RTOS_Priority_Highest) || (RTOS_Priority_Idle == targetPriority))
	{
		return RTOS_ERROR_FAILED;
	}

	RTOS_EnterCriticalSection(saved_state);

	if (0 != RTOS.TaskList[targetPriority])
	{
		result = RTOS_ERROR_PRIORITY_IN_USE;
	}
	else
	{
#if defined(RTOS_SMP)
		if (RTOS_TaskSet_IsMember(RTOS.RunningTasks,  oldPriority))
		{
			RTOS_TaskSet_RemoveMember(RTOS.RunningTasks, oldPriority);
			RTOS_TaskSet_AddMember(RTOS.RunningTasks, targetPriority);
		}

		for (i = 0; i < (RTOS_SMP_CPU_CORES); i++)
		{
			if (RTOS_TaskSet_IsMember(RTOS.TasksAllowed[i], oldPriority))
			{
				RTOS_TaskSet_RemoveMember(RTOS.TasksAllowed[i], oldPriority);
				RTOS_TaskSet_AddMember(RTOS.TasksAllowed[i], targetPriority);
			}
		}
#endif
		if (RTOS_TaskSet_IsMember(RTOS.ReadyToRunTasks,  oldPriority))
		{
			RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, oldPriority);
			RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, targetPriority);
		}
		else
		{
#if defined(RTOS_SUPPORT_EVENTS)
			if (RTOS_TaskSet_IsMember(RTOS.WaitingTasks,  oldPriority))
			{
				RTOS_TaskSet_RemoveMember(RTOS.WaitingTasks, oldPriority);
				RTOS_TaskSet_AddMember(RTOS.WaitingTasks, targetPriority);

				// If we are here, this should always be true.
				if ((0 != task->WaitFor) && (RTOS_TaskSet_IsMember(task->WaitFor->TasksWaiting,  oldPriority)))
				{
					RTOS_TaskSet_RemoveMember(task->WaitFor->TasksWaiting, oldPriority);
					RTOS_TaskSet_AddMember(task->WaitFor->TasksWaiting, targetPriority);
				}
			}
#endif
#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
			if (RTOS_TaskSet_IsMember(RTOS.SuspendedTasks,  oldPriority))
			{
				RTOS_TaskSet_RemoveMember(RTOS.SuspendedTasks, oldPriority);
				RTOS_TaskSet_AddMember(RTOS.SuspendedTasks, targetPriority);
			}
#endif
		}

#if defined(RTOS_SUPPORT_TIMESHARE)
		if (RTOS_TaskSet_IsMember(RTOS.TimeshareTasks,  oldPriority))
		{
			RTOS_TaskSet_RemoveMember(RTOS.TimeshareTasks, oldPriority);
			RTOS_TaskSet_AddMember(RTOS.TimeshareTasks, targetPriority);

			if (RTOS_TaskSet_IsMember(RTOS.PreemptedTasks,  oldPriority))
			{
				RTOS_TaskSet_RemoveMember(RTOS.PreemptedTasks, oldPriority);
				RTOS_TaskSet_AddMember(RTOS.PreemptedTasks, targetPriority);
			}
		}
#endif

#if defined(RTOS_SUPPORT_SLEEP)
		if (task == RTOS.Sleepers[oldPriority])
		{
			RTOS.Sleepers[targetPriority] = task;
			RTOS.Sleepers[oldPriority] = 0;
		}
#endif

		RTOS.TaskList[targetPriority] = task;
		RTOS.TaskList[oldPriority] = 0;
		task->Priority = targetPriority;
	}

	RTOS_ExitCriticalSection(saved_state);

    if ((!RTOS_IsInsideIsr()) && (!RTOS_SchedulerIsLocked()))
    {
		RTOS_INVOKE_SCHEDULER();
	}
	
	return result;
}


