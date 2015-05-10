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

#include <stdint.h>
#include <rtos.h>
#include <rtos_internals.h>

volatile RTOS_OS RTOS;

RTOS_RegInt rtos_CreateTask(RTOS_Task *task, RTOS_TaskPriority priority, void *sp0, unsigned long stackCapacity, void (*f)(), void *param)
{
	RTOS_RegInt i;

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
#endif
	if (0 == task)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	if (0 != RTOS.TaskList[priority])
	{
        	return RTOS_ERROR_PRIORITY_IN_USE;
	}

	// sp0 == 0 means that we are initializing a task structure for the 'current thread of execution'.
	// In that case no stack initialization will be done.
	if ((0 != sp0) && (RTOS_INITIAL_STACK_DEPTH) >= stackCapacity)
	{
		return RTOS_ERROR_FAILED;
	}

	task->Priority = priority;
	task->Action = f;
	task->Parameter = param;
	task->SP0 = sp0;

	rtos_TargetInitializeTask(task, stackCapacity);
	
	RTOS.TaskList[priority] = task;

#if defined(RTOS_SUPPORT_TIMESHARE)
	task->TicksToRun = RTOS_TIMEOUT_FOREVER;
	task->TimeSliceTicks = RTOS_TIMEOUT_FOREVER;
	for (i = 0; i < (RTOS_LIST_WHICH_COUNT); i++)
	{
		task->FcfsLists[i].Previous = 0;
		task->FcfsLists[i].Next = 0;
	}
#endif
	RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, priority);
#if defined(RTOS_SMP)
	for (i = 0; i < (RTOS_SMP_CPU_CORES); i++)
	{
		RTOS_TaskSet_AddMember(RTOS.TasksAllowed[i], priority);
	}
#endif
	return RTOS_OK;
}

// This function can be called during initialization before the RTOS is fully functional.
RTOS_RegInt RTOS_CreateTask(RTOS_Task *task, RTOS_TaskPriority priority, void *sp0, unsigned long stackCapacity, void (*f)(), void *param)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state = 0);

	if (RTOS.IsRunning)
	{
		RTOS_EnterCriticalSection(saved_state);
	}

	result = rtos_CreateTask(task, priority, sp0, stackCapacity, f, param);

	if (RTOS.IsRunning)
	{
		RTOS_ExitCriticalSection(saved_state);
	}
	return result;
}

#if defined(RTOS_TASK_NAME_LENGTH)
// This function can be called during initialization before the RTOS is fully functional.
void RTOS_SetTaskName(RTOS_Task *task, const char *name)
{
	RTOS_RegInt i;
	char c;
	RTOS_SavedCriticalState(saved_state = 0);
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
#endif
	
	if (0 == task)
	{
		return;
	}

	if (RTOS.IsRunning)
	{
		RTOS_EnterCriticalSection(saved_state);
	}

	task->TaskName[0] = '\0';
	task->TaskName[(RTOS_TASK_NAME_LENGTH) - 1] = '\0';

	if (0 != name) 
	{
		// We do not necessarily even have a standard C library, so copy stuff manually.
		for (i = 0; i < ((RTOS_TASK_NAME_LENGTH) - 1); i++)
		{
			c = *name++;

			task->TaskName[i] = c;

			if (0 == c)
			{
				break;
			}
		}
	}

	if (RTOS.IsRunning)
	{
		RTOS_ExitCriticalSection(saved_state);
	}
}
#endif
#if defined(RTOS_SMP)
RTOS_RegInt RTOS_RestrictTaskToCpus(RTOS_Task *task, RTOS_CpuMask cpus)
{
	RTOS_CpuId i;
	RTOS_TaskPriority priority;
	RTOS_CpuMask mask;
	RTOS_RegInt isRunning = RTOS.IsRunning;

	RTOS_SavedCriticalState(saved_state);

	if (0 == task)
	{
		return RTOS_ERROR_FAILED;
	}

	if (isRunning)
	{
		RTOS_EnterCriticalSection(saved_state);
	}

	priority = task->Priority;

	for (mask = 1, i = 0; i < (RTOS_SMP_CPU_CORES); i++, mask <<= 1)
	{

		if (0 == (cpus & mask))
		{
			RTOS_TaskSet_RemoveMember(RTOS.TasksAllowed[i], priority);
		}
		else
		{
			RTOS_TaskSet_AddMember(RTOS.TasksAllowed[i], priority);
		}
	}

	if (isRunning)
	{
		RTOS_ExitCriticalSection(saved_state);
	}

	return RTOS_OK;
}
#endif
// -----------------------------------------------------------------------------------
RTOS_Task *RTOS_GetCurrentTask(void)
{
#if defined(RTOS_SMP)
	RTOS_Task *thisTask;
	RTOS_SavedCriticalState(saved_state);

	RTOS_EnterCriticalSection(saved_state);
	thisTask = RTOS_CURRENT_TASK();
	RTOS_ExitCriticalSection(saved_state);

	return thisTask;
#else
	return RTOS.CurrentTask;
#endif
}

void rtos_RunTask(void)
{
	volatile RTOS_Task *thisTask;
	RTOS_TaskPriority priority;

	RTOS_SavedCriticalState(saved_state);

	thisTask = RTOS_GetCurrentTask();

	// Execute the task's function.
	thisTask->Action(thisTask->Parameter);

 	// If the tasks function has returned just kill the task and try to clean up after it as much as possible.
	// There is no real resource management, so the task should first do its own clean up.
	RTOS_EnterCriticalSection(saved_state);

	thisTask = RTOS_CURRENT_TASK();

	priority = thisTask->Priority;

	// Task is removed from the ready to run set.
	// No other sets are touched. The assumption is that the task cannot be sleeping or waiting for some event 
	// since it is obviously executing otherwise we would not be here.
	RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, priority);

#if defined(RTOS_SUPPORT_TIMESHARE)
	// Make sure the task is not in the time share set.
	RTOS_TaskSet_RemoveMember(RTOS.TimeshareTasks, priority);
#endif

	// Remove task from the list of valid tasks and mark it as killed.
	RTOS.TaskList[priority] = 0;
	thisTask->Status = RTOS_TASK_STATUS_KILLED;
	// RTOS_ExitCriticalSection(saved_state); // -- Not actually necessary, could be harmful?
	// Invoke the scheduler to select a different task to run.
	RTOS_INVOKE_SCHEDULER();
	// We truly should never get here.
	while(1);
	RTOS_ExitCriticalSection(saved_state); // No reachable, but pacifies GCC.
}

#if defined(RTOS_FIND_HIGHEST)
RTOS_TaskPriority RTOS_GetHighestPriorityInSet(RTOS_TaskSet taskSet)
{
	return RTOS_FIND_HIGHEST(taskSet);
}
#else
#error RTOS_FIND_HIGHEST must be defined by the target port.
#endif

RTOS_INLINE RTOS_Task *rtos_TaskFromPriority(RTOS_TaskPriority priority)
{
	return (priority > RTOS_Priority_Highest) ? 0 : RTOS.TaskList[priority];
}

// This is an API function wrapper, do not confuse it with the internal (and much faster) version above.
RTOS_Task *RTOS_TaskFromPriority(RTOS_TaskPriority priority)
{
	RTOS_Task *task;
	RTOS_SavedCriticalState(saved_state);

	RTOS_EnterCriticalSection(saved_state);
	task = rtos_TaskFromPriority(priority);
	RTOS_ExitCriticalSection(saved_state);
	return task;
}

#if defined(RTOS_SMP)
// The Scheduler for SMP.
// This scheduler is run on each CPU independently.
// There is no attempt to assign tasks to CPUs in any centralized manner.
void rtos_Scheduler(void)
{
	RTOS_CpuId cpu;
	RTOS_TaskPriority currentPriority;
	RTOS_Task *currentTask;
	RTOS_TaskPriority priority;
	RTOS_TaskSet tasks;
	RTOS_TaskSet runningTasks;
	RTOS_Task *task;

	if (RTOS_SchedulerIsLocked())
	{
		return;
	}

	cpu = RTOS_CurrentCpu();

	currentTask = RTOS.CurrentTasks[cpu];

	if (0 != currentTask)
	{

#if defined(RTOS_SUPPORT_TIMESHARE)
		rtos_ManageTimeshared();
#endif

		currentPriority = currentTask->Priority;
	}

	tasks = RTOS_TaskSet_Intersection(RTOS.ReadyToRunTasks, RTOS.TasksAllowed[cpu]);
	runningTasks = RTOS.RunningTasks;

	if (0 != currentTask)
	{
		RTOS_TaskSet_RemoveMember(runningTasks, currentPriority);
	}

	tasks = RTOS_TaskSet_Difference(tasks, runningTasks);

	priority = RTOS_GetHighestPriorityInSet(tasks);

	task = rtos_TaskFromPriority(priority);

	if (0 != currentTask)
	{
		currentTask->Cpu = RTOS_CPUID_NO_CPU;
	}

	if (0 == task)
	{
		RTOS.TimeShareCpus = RTOS_CpuMask_RemoveCpu(RTOS.TimeShareCpus, cpu);
		RTOS.CurrentTasks[cpu] = 0;
	}
	else
	{
		RTOS_TaskSet_AddMember(runningTasks, priority);
		RTOS.CurrentTasks[cpu] = task;
		task->Cpu = cpu;
#if defined(RTOS_SUPPORT_TIMESHARE)
		if (task->IsTimeshared)
		{
			RTOS.TimeShareCpus = RTOS_CpuMask_AddCpu(RTOS.TimeShareCpus, cpu);
		}
		else
		{
			RTOS.TimeShareCpus = RTOS_CpuMask_RemoveCpu(RTOS.TimeShareCpus, cpu);
		}
#endif
	}
	RTOS.RunningTasks = runningTasks;

}
#else
// The Scheduler must be called from inside an ISR.
void rtos_Scheduler(void)
{
	RTOS_TaskPriority priority;
	RTOS_Task *task;

	if (RTOS_SchedulerIsLocked())
	{
		return;
	}

#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_ManageTimeshared();
#endif

	priority = RTOS_GetHighestPriorityInSet(RTOS.ReadyToRunTasks);
	task = rtos_TaskFromPriority(priority);

	if (0 != task)
	{
		RTOS.CurrentTask = task;
	}
}
#endif

#if defined(RTOS_INVOKE_YIELD)
void rtos_SchedulerForYield(void)
{
	RTOS_TaskPriority currentPriority;
	RTOS_TaskPriority priority;
	RTOS_Task *task;
	RTOS_TaskSet tasks;
	RTOS_TaskSet t;
#if defined(RTOS_SMP)
	RTOS_CpuId cpu;
#endif

	if (RTOS_SchedulerIsLocked())
	{
		return;
	}

#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_ManageTimeshared();
#endif


#if defined(RTOS_SMP)
	cpu = RTOS_CurrentCpu();
	currentPriority = RTOS.CurrentTasks[cpu]->Priority;
	tasks = RTOS_TaskSet_Intersection(RTOS.ReadyToRunTasks, RTOS.TasksAllowed[cpu]);
	tasks = RTOS_TaskSet_Difference(tasks, RTOS.RunningTasks);	// None of the currently running tasks.
#else
	tasks = RTOS.ReadyToRunTasks;
	currentPriority = RTOS_CURRENT_TASK()->Priority;
	RTOS_TaskSet_RemoveMember(tasks, currentPriority);			// Not the current task.
#endif
	RTOS_TaskSet_RemoveMember(tasks, RTOS_Priority_Idle);		// Not the idle task.
	
	t = tasks & ~((~(RTOS_TaskSet)0) << currentPriority);

	priority = RTOS_GetHighestPriorityInSet(t);
	task = rtos_TaskFromPriority(priority);

	if (0 == task)
	{
		priority = RTOS_GetHighestPriorityInSet(tasks);
		task = rtos_TaskFromPriority(priority);
	}

	if (0 != task)
	{
#if defined(RTOS_SMP)
		if (task != RTOS.CurrentTasks[cpu])
		{
			RTOS_TaskSet_RemoveMember(RTOS.RunningTasks, currentPriority);
			RTOS.CurrentTasks[cpu]->Cpu = RTOS_CPUID_NO_CPU;
			RTOS.CurrentTasks[cpu] = task;
			task->Cpu = cpu;
			RTOS_TaskSet_AddMember(RTOS.RunningTasks, priority);
#if defined(RTOS_SUPPORT_TIMESHARE)
		if (task->IsTimeshared)
		{
			RTOS.TimeShareCpus = RTOS_CpuMask_AddCpu(RTOS.TimeShareCpus, cpu);
		}
		else
		{
			RTOS.TimeShareCpus = RTOS_CpuMask_RemoveCpu(RTOS.TimeShareCpus, cpu);
		}
#endif
		}
#else
		RTOS.CurrentTask = task;
#endif
	}
}

void RTOS_YieldPriority(void)
{
	if ((!RTOS_IsInsideIsr()) && (!RTOS_SchedulerIsLocked()))
	{
		RTOS_INVOKE_YIELD();
	}
}
#endif

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
void RTOS_LockScheduler(void)
{
	RTOS_SavedCriticalState(saved_state);

	RTOS_EnterCriticalSection(saved_state);
	RTOS.SchedulerLocked++;
	RTOS_ExitCriticalSection(saved_state);
}

void RTOS_UnlockScheduler(void)
{
	RTOS_SavedCriticalState(saved_state);

	RTOS_EnterCriticalSection(saved_state);
	if (0 != RTOS.SchedulerLocked)
	{
		RTOS.SchedulerLocked--;
	}
	RTOS_ExitCriticalSection(saved_state);
}
#endif

RTOS_Time RTOS_GetTime(void)
{
	RTOS_Time time;
	RTOS_SavedCriticalState(saved_state);
	
	RTOS_EnterCriticalSection(saved_state);
	time = RTOS.Time;
	RTOS_ExitCriticalSection(saved_state);

	return time;
}

#if 0
void RTOS_SetTime(RTOS_Time time)
{
	RTOS_SavedCriticalState(saved_state);
	
	RTOS_EnterCriticalSection(saved_state);
	RTOS.Time = time;
	RTOS_ExitCriticalSection(saved_state);
}
#endif

#if defined(RTOS_SUPPORT_SLEEP)
RTOS_INLINE void rtos_AddToSleepers(RTOS_Task *task)
{
	RTOS.Sleepers[task->Priority] = task;
}

RTOS_INLINE void rtos_RemoveFromSleepers(RTOS_Task *task)
{
	RTOS.Sleepers[task->Priority] = 0;
}
#endif

#if defined(RTOS_INCLUDE_DELAY)
// Delay() and DelayUntil().
// The parameter absolute chooses if time is relative to the current time i.e. Delay() or absolute i.e. DelayUntil().
RTOS_RegInt rtos_Delay(RTOS_Time time, RTOS_RegInt absolute)
{
    	RTOS_RegInt result;
	volatile RTOS_Task *thisTask;
	RTOS_SavedCriticalState(saved_state);

	thisTask = RTOS_GetCurrentTask();

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(!RTOS_IsInsideIsr());
	RTOS_ASSERT(!RTOS_SchedulerIsLocked());
	RTOS_ASSERT(RTOS_Priority_Idle != thisTask->Priority);

#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
    	if (RTOS_IsInsideIsr())
    	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
    	}

	// The Idle task cannot sleep!
	if (RTOS_Priority_Idle == thisTask->Priority)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

    	RTOS_EnterCriticalSection(saved_state);

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
    	if (RTOS_SchedulerIsLocked())
	{
    		RTOS_ExitCriticalSection(saved_state);
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif
    	thisTask->WakeUpTime = absolute ? time : (RTOS.Time + time);
    	RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, thisTask->Priority);
    	thisTask->Status = RTOS_TASK_STATUS_SLEEPING;
#if defined(RTOS_SUPPORT_TIMESHARE)
	if (thisTask->IsTimeshared)
	{
		thisTask->FcfsLists[ RTOS_LIST_WHICH_WAIT].Previous = 0;
		thisTask->FcfsLists[ RTOS_LIST_WHICH_WAIT].Next = 0;
		rtos_SchedulePeer();
	}
#endif
	rtos_AddToSleepers(thisTask);
    	RTOS_ExitCriticalSection(saved_state);
    	RTOS_INVOKE_SCHEDULER();
    	RTOS_EnterCriticalSection(saved_state);
#if defined( RTOS_INCLUDE_WAKEUP)
    	result = (RTOS_TASK_STATUS_AWAKENED == thisTask->Status) ? RTOS_ABORTED : RTOS_OK;
#else
    	result = RTOS_OK;
#endif
	thisTask->Status = RTOS_TASK_STATUS_ACTIVE;
    	RTOS_ExitCriticalSection(saved_state);
	return result;
}

RTOS_RegInt RTOS_DelayUntil(RTOS_Time wakeUpTime)
{
	return rtos_Delay(wakeUpTime, 1);
}

RTOS_RegInt RTOS_Delay(RTOS_Time ticksToSleep)
{
	if (0 == ticksToSleep)
	{
		return RTOS_OK;
	}
	return rtos_Delay(ticksToSleep, 0);
}
#endif


#if defined(RTOS_INCLUDE_NAKED_EVENTS) || defined(RTOS_INCLUDE_SEMAPHORES)
RTOS_INLINE void rtos_WaitForEvent(RTOS_EventHandle *event, RTOS_Task *task, RTOS_Time timeout)
{
        task->WaitFor = event;
        RTOS_TaskSet_AddMember(event->TasksWaiting, task->Priority);
#if defined(RTOS_SUPPORT_TIMESHARE)
	if (task->IsTimeshared)
	{
		rtos_AppendToTaskDList(&(event->WaitList), RTOS_LIST_WHICH_WAIT, task);
	}
#endif

        RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, task->Priority);
	task->Status = RTOS_TASK_STATUS_WAITING;

#if defined(RTOS_SUPPORT_TIMESHARE)
	if (task->IsTimeshared)
	{
		// We have just unscheduled a task that uses time slices.
		// If there are others that are not running because their time slice has expired
		// now we can pick another one to use the CPU.
		rtos_SchedulePeer();
	}
#endif

        if ((0 != timeout) && ((RTOS_TIMEOUT_FOREVER) != timeout))
        {
		task->WakeUpTime = RTOS.Time + timeout;
		rtos_AddToSleepers(task);
        }
}

RTOS_INLINE void rtos_RemoveTaskWaiting(RTOS_EventHandle *event, RTOS_Task *task)
{
	if (0 == task)
	{
		return;
	}

        RTOS_TaskSet_RemoveMember(event->TasksWaiting, task->Priority);

#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_RemoveFromTaskDList(&(event->WaitList), RTOS_LIST_WHICH_WAIT, task);
#endif
	task->WaitFor = 0;
}

#if defined(RTOS_SUPPORT_TIMESHARE)
RTOS_INLINE RTOS_Task *rtos_GetFirstWaitingTask(RTOS_EventHandle *event)
{
	RTOS_Task *task = 0;

	task = rtos_RemoveFirstTaskFromDList(&(event->WaitList), RTOS_LIST_WHICH_WAIT);
	return task;
}
#endif

RTOS_INLINE RTOS_RegInt rtos_SignalEvent(RTOS_EventHandle *event)
{
	RTOS_TaskPriority priority;
	RTOS_Task *task;

	priority = RTOS_GetHighestPriorityInSet(event->TasksWaiting);

	if (priority <= RTOS_Priority_Highest)
	{

#if defined(RTOS_SUPPORT_TIMESHARE)
		if (RTOS_TaskSet_IsMember(RTOS.TimeshareTasks, priority))
		{
			task = rtos_GetFirstWaitingTask(event);
		}
		else
		{
        		task = rtos_TaskFromPriority(priority);
		}
#else

       		task = rtos_TaskFromPriority(priority);
#endif
		if (0 != task)
		{
			RTOS_TaskSet_RemoveMember(event->TasksWaiting, task->Priority); 
			task->WaitFor = 0;
			rtos_RemoveFromSleepers(task);
			RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, task->Priority);
			task->Status = RTOS_TASK_STATUS_ACTIVE;
			return RTOS_OK;
		}
	}

	return RTOS_TIMED_OUT;
}
#endif

#if defined(RTOS_INCLUDE_NAKED_EVENTS)
RTOS_RegInt RTOS_CreateEventHandle(RTOS_EventHandle *event)
{
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != event);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == event)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	RTOS_EnterCriticalSection(saved_state);
	event->TasksWaiting = 0;
#if defined(RTOS_SUPPORT_TIMESHARE)
	event->WaitList.Head = 0;
	event->WaitList.Tail = 0;
#endif
	RTOS_ExitCriticalSection(saved_state);
	return RTOS_OK;
}

RTOS_RegInt RTOS_WaitForEvent(RTOS_EventHandle *event, RTOS_Time timeout)
{
	RTOS_RegInt result;
	volatile RTOS_Task *thisTask;
	RTOS_RegInt status;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != event);
	RTOS_ASSERT(!RTOS_IsInsideIsr());
	RTOS_ASSERT(!RTOS_SchedulerIsLocked());
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == event)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

    	if (RTOS_IsInsideIsr())
    	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
    	}
#endif

	RTOS_EnterCriticalSection(saved_state);

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
    	if (RTOS_SchedulerIsLocked())
    	{
		RTOS_ExitCriticalSection(saved_state);
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
    	}
#endif

	thisTask = RTOS_CURRENT_TASK();

	rtos_WaitForEvent(event, thisTask, timeout);

	RTOS_ExitCriticalSection(saved_state);

	RTOS_INVOKE_SCHEDULER();

	RTOS_EnterCriticalSection(saved_state);

	status = thisTask->Status;
	
#if defined( RTOS_INCLUDE_WAKEUP)
    	result = (RTOS_TASK_STATUS_TIMED_OUT == status) ? RTOS_TIMED_OUT : ((RTOS_TASK_STATUS_AWAKENED == status) ? RTOS_ABORTED : RTOS_OK);
#else
    	result = (RTOS_TASK_STATUS_TIMED_OUT == status) ? RTOS_TIMED_OUT : RTOS_OK;
#endif

	thisTask->Status = RTOS_TASK_STATUS_ACTIVE;

    	RTOS_ExitCriticalSection(saved_state);

	return result;
}

RTOS_RegInt RTOS_SignalEvent(RTOS_EventHandle *event)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != event);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == event)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	RTOS_EnterCriticalSection(saved_state);
	result = rtos_SignalEvent(event);
	RTOS_ExitCriticalSection(saved_state);

	if (!RTOS_IsInsideIsr() && !RTOS_SchedulerIsLocked())
	{
		RTOS_INVOKE_SCHEDULER();
	}

	return result;
}
#endif

#if defined(RTOS_INCLUDE_SEMAPHORES)
RTOS_RegInt RTOS_CreateSemaphore(RTOS_Semaphore *semaphore, RTOS_SemaphoreCount initialCount)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != semaphore);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == semaphore)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	RTOS_EnterCriticalSection(saved_state);
	result = RTOS_CreateEventHandle(&(semaphore->Event));
	semaphore->Count = initialCount;
	RTOS_ExitCriticalSection(saved_state);

	return result;
}

RTOS_RegInt RTOS_PostSemaphore(RTOS_Semaphore *semaphore)
{
	RTOS_RegInt result = RTOS_OK;
	RTOS_SavedCriticalState(saved_state);
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != semaphore);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == semaphore)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	RTOS_EnterCriticalSection(saved_state);

	if (RTOS_OK == rtos_SignalEvent(&(semaphore->Event)))
	{
		RTOS_ExitCriticalSection(saved_state);

		if (!RTOS_IsInsideIsr() && !RTOS_SchedulerIsLocked())
		{
			RTOS_INVOKE_SCHEDULER();
		}
	}
    	else
    	{
        	if ((RTOS_SEMAPHORE_COUNT_MAX) > semaphore->Count)
		{
        		semaphore->Count++;
		}
		else
		{
			result = RTOS_ERROR_OVERFLOW;
		}

        	RTOS_ExitCriticalSection(saved_state);
    	}

	return result;

}

RTOS_RegInt RTOS_GetSemaphore(RTOS_Semaphore *semaphore, RTOS_Time timeout)
{
    	RTOS_RegInt result;
	volatile RTOS_Task *thisTask;
	RTOS_RegInt status;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != semaphore);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == semaphore)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif
    	RTOS_EnterCriticalSection(saved_state);

    	if (0 != semaphore->Count)
    	{
        	semaphore->Count--;
        	RTOS_ExitCriticalSection(saved_state);
        	return RTOS_OK;
    	}

	// Do allow a GetSempahore() operation from inside an ISR  or with the scheduler locked
	// but only with a zero timeout (i.e. the non-waiting variety), since an ISR cannot sleep,
	// tasks cannot sleep either is the scheduler is locked.
    	if ((RTOS_IsInsideIsr()) || (RTOS_SchedulerIsLocked()))
    	{
        	RTOS_ExitCriticalSection(saved_state);
        	return (0 == timeout) ?  RTOS_TIMED_OUT : RTOS_ERROR_OPERATION_NOT_PERMITTED;
    	}

	thisTask = RTOS_CURRENT_TASK();

	if (0 == timeout)
	{
		thisTask->Status = RTOS_TASK_STATUS_ACTIVE;
    		RTOS_ExitCriticalSection(saved_state);
    		return RTOS_TIMED_OUT;
	}
 
    	rtos_WaitForEvent(&(semaphore->Event), thisTask, timeout);

    	RTOS_ExitCriticalSection(saved_state);

    	RTOS_INVOKE_SCHEDULER();

    	RTOS_EnterCriticalSection(saved_state);

	status = thisTask->Status;

#if defined(RTOS_INCLUDE_WAKEUP)
    	result = (RTOS_TASK_STATUS_TIMED_OUT == status) ? RTOS_TIMED_OUT : ((RTOS_TASK_STATUS_AWAKENED == status) ? RTOS_ABORTED : RTOS_OK);
#else
    	result = (RTOS_TASK_STATUS_TIMED_OUT == status) ? RTOS_TIMED_OUT : RTOS_OK;
#endif

	thisTask->Status = RTOS_TASK_STATUS_ACTIVE;
    	RTOS_ExitCriticalSection(saved_state);
	return result;
}

RTOS_SemaphoreCount RTOS_PeekSemaphore(RTOS_Semaphore *semaphore)
{
	RTOS_SemaphoreCount count = 0;
	RTOS_SavedCriticalState(saved_state);
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != semaphore);
#endif
	
	RTOS_EnterCriticalSection(saved_state);

	if (0 != semaphore)
	{
		count = semaphore->Count;
	}
	RTOS_ExitCriticalSection(saved_state);

	return count;
}
#endif

#if defined(RTOS_INCLUDE_WAKEUP) || defined(RTOS_INCLUDE_SUSPEND_AND_RESUME) || defined(RTOS_INCLUDE_KILLTASK)
static RTOS_RegInt rtos_WakeupTask(RTOS_Task *task)
{
	if (0 == task)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	if ((RTOS_TASK_STATUS_WAITING != task->Status) && (RTOS_TASK_STATUS_SLEEPING != task->Status))
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	if (0 != task->WaitFor)
	{
		rtos_RemoveTaskWaiting(task->WaitFor, task);
	}

	rtos_RemoveFromSleepers(task);
	RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, task->Priority);

	task->Status = RTOS_TASK_STATUS_AWAKENED;

	return RTOS_OK;
}
#endif

#if defined(RTOS_INCLUDE_WAKEUP)
// Prematurely wake up a task that is sleeping or waiting for an event.
// 
RTOS_RegInt RTOS_WakeupTask(RTOS_Task *task)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);
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
	result = rtos_WakeupTask(task);
	RTOS_ExitCriticalSection(saved_state);

    	if (!RTOS_IsInsideIsr() && !RTOS_SchedulerIsLocked())
    	{
		RTOS_INVOKE_SCHEDULER();
	}

	return result;
}
#endif

#if defined(RTOS_SMP)
// Force a task's CPU into a state where it is safe to change OS structures from this thread AND the scheduler will
// be rerun on the task's CPU before any code is executed on the task's behalf.
// This function will be called when we are already inside a critical section.
RTOS_RegInt rtos_TaskForceInterrupted(RTOS_Task *task)
{
	RTOS_CpuId otherCpu = task->Cpu;
	RTOS_CpuId thisCpu = RTOS_CurrentCpu();

	if (thisCpu == otherCpu)
	{
		// If we are here, this CPU is executing OS code so we can just go ahead.
		return RTOS_OK;
	}

	if ((RTOS_CPUID_NO_CPU) == otherCpu)
	{
		// This should not happen.
		return RTOS_ERROR_FAILED;
	}

	if (!rtos_IsCpuInsideIsr(otherCpu))
	{
		rtos_SignalCpu(otherCpu);
	}

	while(!rtos_IsCpuInsideIsr(otherCpu));

	return RTOS_OK;
}
#endif

#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
RTOS_RegInt RTOS_SuspendTask(RTOS_Task *task)
{
	RTOS_RegInt result = RTOS_ERROR_FAILED;
	RTOS_TaskPriority priority;
	RTOS_SavedCriticalState(saved_state);
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

#if defined(RTOS_SUPPORT_SLEEP)
	if ((RTOS_TASK_STATUS_WAITING == task->Status) || (RTOS_TASK_STATUS_SLEEPING == task->Status))
	{
		rtos_WakeupTask(task);
	}
#endif

	priority = task->Priority;

	if (RTOS_TaskSet_IsMember(RTOS.SuspendedTasks, priority))
	{
		result = RTOS_OK;
	}

#if defined(RTOS_SMP)
	// If the task is running on this CPU it is OK to suspend it, but if it is running on another CPU
	// We have to make sure to stop executing on its behalf and run the scheduler when we are done.
	// BTW: RTOS.RunningTasks will be updated by the scheduler so we are not updating it in this function.
	if (RTOS_TaskSet_IsMember(RTOS.RunningTasks, priority))
	{
		if (RTOS_OK != rtos_TaskForceInterrupted(task))
		{
			return RTOS_ERROR_FAILED;
		}
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
		rtos_RemoveFromTaskDList(&(RTOS.PreemptedList), RTOS_LIST_WHICH_PREEMPTED, task);
		result = RTOS_OK;
	}
#endif

	RTOS_ExitCriticalSection(saved_state);

    if ((!RTOS_IsInsideIsr()) && (!RTOS_SchedulerIsLocked()))
    {
    	RTOS_INVOKE_SCHEDULER();
    }

	return result;
}

RTOS_RegInt RTOS_ResumeTask(RTOS_Task *task)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);
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
	if (!RTOS_TaskSet_IsMember(RTOS.SuspendedTasks, task->Priority))
	{
	
		result = RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
	else
	{
		RTOS_TaskSet_RemoveMember(RTOS.SuspendedTasks, task->Priority);
		RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, task->Priority);
		task->Status &= ~(RTOS_TASK_STATUS_SUSPENDED_FLAG);
		result = RTOS_OK;
	}

	RTOS_ExitCriticalSection(saved_state);

    	if ((!RTOS_IsInsideIsr()) && (!RTOS_SchedulerIsLocked()))
    	{
		RTOS_INVOKE_SCHEDULER();
	}

	return result;
}
#endif

#if defined(RTOS_INCLUDE_KILLTASK)
RTOS_RegInt RTOS_KillTask(RTOS_Task *task)
{
	RTOS_RegInt result = RTOS_ERROR_FAILED;
	RTOS_TaskPriority priority;
#if defined(RTOS_SMP)
	int i;
#endif
	RTOS_SavedCriticalState(saved_state);
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == task)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	if (RTOS_TASK_STATUS_KILLED == task->Status)
	{
		return RTOS_OK;
	}

	RTOS_EnterCriticalSection(saved_state);

#if defined(RTOS_SUPPORT_SLEEP)
	if ((RTOS_TASK_STATUS_WAITING == task->Status) || (RTOS_TASK_STATUS_SLEEPING == task->Status))
	{
		rtos_WakeupTask(task);
	}
#endif

	if (RTOS_TASK_STATUS_STOPPED == task->Status)
	{
		result = RTOS_OK;
	}

	priority = task->Priority;

#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
	if (RTOS_TaskSet_IsMember(RTOS.SuspendedTasks, priority))
	{
		RTOS_TaskSet_RemoveMember(RTOS.SuspendedTasks, priority);
		result = RTOS_OK;
	}
#endif

#if defined(RTOS_SMP)
	// If the task is running on this CPU it is OK to suspend it, but if it is running on another CPU
	// We have to make sure to stop executing on its behalf and run the scheduler when we are done.
	// BTW: RTOS.RunningTasks will be updated by the scheduler so we are not updating it in this function.
	if (RTOS_TaskSet_IsMember(RTOS.RunningTasks, priority))
	{
		if (RTOS_OK != rtos_TaskForceInterrupted(task))
		{
			return RTOS_ERROR_FAILED;
		}
	}
#endif

	if (RTOS_TaskSet_IsMember(RTOS.ReadyToRunTasks, priority))
	{
		RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, priority);
		result = RTOS_OK;
	}

#if defined(RTOS_SUPPORT_TIMESHARE)
	if (RTOS_TaskSet_IsMember(RTOS.TimeshareTasks, priority))
	{
		RTOS_TaskSet_RemoveMember(RTOS.TimeshareTasks, priority);
		result = RTOS_OK;
		if (RTOS_TaskSet_IsMember(RTOS.PreemptedTasks, priority))
		{
			RTOS_TaskSet_RemoveMember(RTOS.PreemptedTasks, priority);
			rtos_RemoveFromTaskDList(&(RTOS.PreemptedList), RTOS_LIST_WHICH_PREEMPTED, task);
			result = RTOS_OK;
		}
	}
#endif

	if (RTOS_OK == result)
	{
#if defined(RTOS_SMP)
		for (i = 0; i < (RTOS_SMP_CPU_CORES); i++)
		{
			RTOS_TaskSet_RemoveMember(RTOS.TasksAllowed[i], priority);
		}
#endif
		RTOS.TaskList[priority] = 0;
		task->Status = RTOS_TASK_STATUS_KILLED;

	}

	RTOS_ExitCriticalSection(saved_state);

    if ((!RTOS_IsInsideIsr()) && (!RTOS_SchedulerIsLocked()))
    {
		RTOS_INVOKE_SCHEDULER();
	}

	return result;
}
#endif

#if defined(RTOS_INCLUDE_CHANGEPRIORITY)
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
			if ((0 != task->WaitFor) && (RTOS_TaskSet_IsMember(task->WaitFor->TasksWaiting,  oldPriority)))
			{
				RTOS_TaskSet_RemoveMember(task->WaitFor->TasksWaiting, oldPriority);
				RTOS_TaskSet_AddMember(task->WaitFor->TasksWaiting, targetPriority);
			}
#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
			else if (RTOS_TaskSet_IsMember(RTOS.SuspendedTasks,  oldPriority))
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
#endif

static void rtos_TimerTickFunction(void)
{
	RTOS_TaskPriority i;
	RTOS_Task *task;

	RTOS.Time++;

#if defined(RTOS_SUPPORT_SLEEP)

#if defined(RTOS_USE_TIMER_TASK)
	RTOS_SavedCriticalState(saved_state);
#endif

	// The Idle task should never actually sleep!
	// We can start checking at priority 1 instead of 0.
	for (i = 1; i <= RTOS_Priority_Highest; i++)
	{
#if defined(RTOS_USE_TIMER_TASK)
		RTOS_EnterCriticalSection(saved_state);
#endif
		task = RTOS.Sleepers[i];

		if (0 != task)
		{
			if (task->WakeUpTime == RTOS.Time)
			{
				rtos_RemoveFromSleepers(task);
				if (0 != task->WaitFor)
               			{
					rtos_RemoveTaskWaiting(task->WaitFor, task);
				}

				RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, i);
				task->Status = RTOS_TASK_STATUS_TIMED_OUT;
			}
       		}
#if defined(RTOS_USE_TIMER_TASK)
		RTOS_ExitCriticalSection(saved_state);
#endif
	}
#endif 
}

#if defined(RTOS_USE_TIMER_TASK)
void RTOS_DefaultTimerFunction(void *p)
{
	volatile RTOS_Task *thisTask;
	RTOS_RegInt res;

	thisTask = RTOS_GetCurrentTask();

	while(1)
	{
		rtos_TimerTickFunction();
#if defined(RTOS_TIMER_EXTRA_ACTION)
		RTOS_TIMER_EXTRA_ACTION();
#endif
		res = RTOS_SuspendTask(thisTask);
	}
	(void)p;
}

void rtos_TimerTick(void)
{
	RTOS_Task *task;

 	task = RTOS.TaskList[RTOS_Priority_Timer];

	if (0 != task)
	{
		RTOS_ResumeTask(task);
	}
}

#else
void rtos_TimerTick(void)
{
#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_ManageTimeshared();
#endif
	rtos_TimerTickFunction();

#if defined(RTOS_SMP)
	rtos_SignalCpus(RTOS_CpuMask_RemoveCpu(RTOS.TimeShareCpus, RTOS_CurrentCpu()));
#endif
}
#endif

void rtos_PrepareToStart(void)
{
#if defined(RTOS_SMP) && defined(RTOS_SUPPORT_TIMESHARE)
	RTOS.TimeshareParallelAllowed = (RTOS_TIMESHARE_PARALLEL_MAX);
#endif
}
