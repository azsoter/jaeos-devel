/*
* Copyright (c) Andras Zsoter 2014-2017.
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

RTOS_RegInt rtos_CreateTask(RTOS_Task *task, void *sp0, unsigned long stackCapacity, void (*f)(void *), void *param)
{
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
#endif
	if (0 == task)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	// sp0 == 0 means that we are initializing a task structure for the 'current thread of execution'.
	// In that case no stack initialization will be done.
	// Also check for sufficient stack space. RTOS_INITIAL_STACK_DEPT defined in the target specific rtos_types.h is in bytes.
	if ((0 != sp0) && ((RTOS_INITIAL_STACK_DEPTH) >= (sizeof(RTOS_StackItem_t) * stackCapacity)))
	{
		return RTOS_ERROR_FAILED;
	}

	task->Action = f;
	task->Parameter = param;
	task->SP0 = sp0;

	rtos_TargetInitializeTask(task, stackCapacity);

#if defined(RTOS_SUPPORT_TIMESHARE)
	task->IsTimeshared = 0;
	task->TicksToRun = RTOS_TIMEOUT_FOREVER;
	task->TimeSliceTicks = RTOS_TIMEOUT_FOREVER;
	task->Link.Previous = 0;
	task->Link.Next = 0;
#endif
	return RTOS_OK;
}

// This function needs to be called from a single threaded context.
// Either before the OS has started or from a critical section.
RTOS_RegInt rtos_RegisterTask(RTOS_Task *task, RTOS_TaskPriority priority)
{

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
	RTOS_ASSERT(0 == RTOS.TaskList[priority]);
#endif

	if (0 != RTOS.TaskList[priority])
	{
        	return RTOS_ERROR_PRIORITY_IN_USE;
	}

	if (0 == task)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	task->Priority = priority;

	RTOS.TaskList[priority] = task;

#if defined(RTOS_SMP)
	rtos_RestrictPriorityToCpus(priority, ~(RTOS_CpuMask)0);
#endif

#if defined(RTOS_SUPPORT_TIMESHARE)
	if (task->IsTimeshared)
	{
		RTOS_TaskSet_AddMember(RTOS.TimeshareTasks, task->Priority);
	}
#endif

	RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, priority);

	return RTOS_OK;
}

#if defined(RTOS_TASK_NAME_LENGTH)
// This function can be called during initialization before the RTOS is fully functional.
void RTOS_SetTaskName(RTOS_Task *task, const char *name)
{
	RTOS_RegInt i;
	char c;
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
#endif
	
	if (0 == task)
	{
		return;
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
}
#endif


// This function can be called during initialization before the RTOS is fully functional.
RTOS_RegInt RTOS_CreateTask(RTOS_Task *task, const char *name, RTOS_TaskPriority priority, void *sp0, unsigned long stackCapacity, void (*f)(), void *param)
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
		result = rtos_RegisterTask(task, priority);
	}

	if (RTOS.IsRunning)
	{
		RTOS_ExitCriticalSection(saved_state);
	}
	return result;
}
// -----------------------------------------------------------------------------------
#if !defined(RTOS_SMP)
RTOS_Task *RTOS_GetCurrentTask(void)
{
	return RTOS.CurrentTask;
}
#endif

// Stop the calling task and remove it from the system (i.e. 'kill it').
RTOS_RegInt RTOS_KillSelf(void)
{
	RTOS_Task *task;
	RTOS_TaskPriority priority;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(!RTOS_IsInsideIsr());
	RTOS_ASSERT(!RTOS_SchedulerIsLocked());
#endif

    if (RTOS_IsInsideIsr())
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	} 

	RTOS_EnterCriticalSection(saved_state);

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
	if (RTOS_SchedulerIsLocked())
	{
		RTOS_ExitCriticalSection(saved_state);
        return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	task = RTOS_CURRENT_TASK();
	priority = task->Priority;

	// Task is removed from the ready to run set.
	// No other sets are touched. The assumption is that the task cannot be sleeping or waiting for some event 
	// since it is obviously executing otherwise we would not be here.
	RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, priority);

#if defined(RTOS_SUPPORT_TIMESHARE)
	// Make sure the task is not in the time share set.
	RTOS_TaskSet_RemoveMember(RTOS.TimeshareTasks, priority);
#endif

#if defined(RTOS_SMP)
	rtos_RestrictPriorityToCpus(priority, (RTOS_CpuMask)0);
#endif

	// Remove task from the list of valid tasks and mark it as killed.
	RTOS.TaskList[priority] = 0;

	RTOS_INVOKE_SCHEDULER();

	RTOS_ExitCriticalSection(saved_state); 	// Not reachable, but pacifies GCC.
	return RTOS_ERROR_FAILED;				// Not actually reachable.
}

void rtos_RunTask(void)
{
	volatile RTOS_Task *thisTask;

	RTOS_SavedCriticalState(saved_state);

	thisTask = RTOS_GetCurrentTask();

	// Execute the task's function.
	thisTask->Action(thisTask->Parameter);

 	// If the tasks function has returned just kill the task and try to clean up after it as much as possible.
	// There is no real resource management, so the task should first do its own clean up.
	RTOS_EnterCriticalSection(saved_state);
	RTOS_KillSelf();

	// Invoke the scheduler to select a different task to run.
	RTOS_INVOKE_SCHEDULER();
	// We truly should never get here.
	while(1);
	RTOS_ExitCriticalSection(saved_state); // Not reachable, but pacifies GCC.
}

#if defined(RTOS_FIND_HIGHEST)
RTOS_TaskPriority RTOS_GetHighestPriorityInSet(RTOS_TaskSet taskSet)
{
	return RTOS_FIND_HIGHEST(taskSet);
}
#else
#error RTOS_FIND_HIGHEST must be defined by the target port.
#endif

//RTOS_INLINE RTOS_Task *rtos_TaskFromPriority(RTOS_TaskPriority priority)
//{
//	return (priority > RTOS_Priority_Highest) ? 0 : RTOS.TaskList[priority];
//}

// This is an API function wrapper, do not confuse it with the internal (and much faster) macro rtos_TaskFromPriority(PRIORITY).
RTOS_Task *RTOS_TaskFromPriority(RTOS_TaskPriority priority)
{
	RTOS_Task *task;
	RTOS_SavedCriticalState(saved_state);

	RTOS_EnterCriticalSection(saved_state);
	task = rtos_TaskFromPriority(priority);
	RTOS_ExitCriticalSection(saved_state);
	return task;
}

#if !defined(RTOS_SMP)
// The Scheduler must be called from inside an ISR or some similar context depending on the target system.
void rtos_Scheduler(void)
{
	RTOS_TaskPriority priority;
	RTOS_Task *task;

#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_ManageTimeshared(RTOS.CurrentTask);
#endif

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
	if (RTOS_SchedulerIsLocked())
	{
		return;
	}
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
#if !defined(RTOS_SMP)
void rtos_SchedulerForYield(void)
{
	RTOS_TaskPriority currentPriority;
	RTOS_TaskPriority priority;
	RTOS_Task *task;
	RTOS_TaskSet tasks;
	RTOS_TaskSet t;
	RTOS_Task *currentTask;
	currentTask = RTOS.CurrentTask;

#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_ManageTimeshared(currentTask);
#endif

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
	if (RTOS_SchedulerIsLocked())
	{
		return;
	}
#endif
	currentPriority = currentTask->Priority;

	tasks = RTOS.ReadyToRunTasks;

	t = tasks;
	RTOS_TaskSet_RemoveMember(t, RTOS_Priority_Idle);		// Not the idle task.
	
	t = t & ~((~(RTOS_TaskSet)0) << currentPriority);		// This makes the assumption that RTOS_TaskSet is some integer type.

	priority = RTOS_GetHighestPriorityInSet(t);
	task = rtos_TaskFromPriority(priority);

	if (0 == task) // If a normal yield did not produce a task just act like a normal scheduler.
	{
		priority = RTOS_GetHighestPriorityInSet(tasks);
		task = rtos_TaskFromPriority(priority);
	}

	if (0 != task)
	{
		RTOS.CurrentTask = task;
	}
}
#endif

void RTOS_YieldPriority(void)
{
	if ((!RTOS_IsInsideIsr()) && (!RTOS_SchedulerIsLocked()))
	{
		RTOS_INVOKE_YIELD();
	}
}
#endif

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
RTOS_RegInt RTOS_LockScheduler(void)
{
	RTOS_SavedCriticalState(saved_state);

	if (RTOS_IsInsideIsr())
	{
		return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	RTOS_EnterCriticalSection(saved_state);
	RTOS.SchedulerLocked++;
	RTOS_ExitCriticalSection(saved_state);

	return RTOS_OK;
}

RTOS_RegInt RTOS_UnlockScheduler(void)
{
	RTOS_SavedCriticalState(saved_state);

	if (RTOS_IsInsideIsr())
	{
		return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	RTOS_EnterCriticalSection(saved_state);

	if (0 == RTOS.SchedulerLocked)
	{
		RTOS_ExitCriticalSection(saved_state);
		return RTOS_ERROR_FAILED;
	}
	else
	{
		RTOS.SchedulerLocked--;
	}

	RTOS_ExitCriticalSection(saved_state);

	if (0 == RTOS.SchedulerLocked)
	{
    		RTOS_INVOKE_SCHEDULER();
	}
	
	return RTOS_OK;
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

RTOS_INLINE RTOS_RegInt rtos_isSleeping(const RTOS_Task *task)
{
	return (RTOS.Sleepers[task->Priority] == task);
}
#else
#define rtos_isSleeping(TASK) 0
#endif

#if defined(RTOS_SUPPORT_EVENTS)
RTOS_INLINE RTOS_RegInt rtos_isWaiting(const RTOS_Task *task)
{
	return RTOS_TaskSet_IsMember(RTOS.WaitingTasks, task->Priority);
}
#else
#define rtos_isWaiting(TASK) 0
#endif

#if defined(RTOS_INCLUDE_DELAY)
// Delay() and DelayUntil().
// The parameter absolute chooses if time is relative to the current time i.e. Delay() or absolute i.e. DelayUntil().
RTOS_RegInt rtos_Delay(RTOS_Time time, RTOS_RegInt absolute)
{
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
    	thisTask->CrossContextReturnValue = RTOS_OK;
    	thisTask->WakeUpTime = absolute ? time : (RTOS.Time + time);
    	RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, thisTask->Priority);

#if defined(RTOS_SUPPORT_TIMESHARE)
    	if (thisTask->IsTimeshared)
    	{
    		thisTask->Link.Previous = 0;
    		thisTask->Link.Next = 0;
    	}
#endif
    	rtos_AddToSleepers(thisTask);
    	RTOS_ExitCriticalSection(saved_state);
    	RTOS_INVOKE_SCHEDULER();
    	return (RTOS_TIMED_OUT == thisTask->CrossContextReturnValue) ? RTOS_OK : thisTask->CrossContextReturnValue;
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


#if defined(RTOS_SUPPORT_EVENTS)
void rtos_WaitForEvent(RTOS_EventHandle *event, RTOS_Task *task, RTOS_Time timeout)
{
	task->CrossContextReturnValue = RTOS_OK;
    task->WaitFor = event;
    RTOS_TaskSet_AddMember(event->TasksWaiting, task->Priority);
#if defined(RTOS_SUPPORT_TIMESHARE)
	if (task->IsTimeshared)
	{
		rtos_AppendTaskToDLList(&(event->WaitList), task);
	}
#endif
    RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, task->Priority);
    RTOS_TaskSet_AddMember(RTOS.WaitingTasks, task->Priority);

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
    RTOS_TaskSet_RemoveMember(RTOS.WaitingTasks, task->Priority);

#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_RemoveTaskFromDLList(&(event->WaitList), task);
#endif
	task->WaitFor = 0;
}

#if defined(RTOS_SUPPORT_TIMESHARE)
RTOS_INLINE RTOS_Task *rtos_GetFirstWaitingTask(RTOS_EventHandle *event)
{
	RTOS_Task *task = 0;

	task = rtos_RemoveFirstTaskFromDLList(&(event->WaitList));
	return task;
}
#endif

RTOS_RegInt rtos_SignalEvent(RTOS_EventHandle *event)
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
			RTOS_TaskSet_RemoveMember(RTOS.WaitingTasks, task->Priority);
			RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, task->Priority);
			return RTOS_OK;
		}
	}

	return RTOS_TIMED_OUT;
}
#endif

#if defined(RTOS_SUPPORT_EVENTS)
RTOS_RegInt RTOS_CreateEventHandle(RTOS_EventHandle *event)
{
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != event);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == event)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	event->TasksWaiting = 0;
#if defined(RTOS_SUPPORT_TIMESHARE)
	event->WaitList.Head = 0;
	event->WaitList.Tail = 0;
#endif
	return RTOS_OK;
}
#endif


#if defined(RTOS_INCLUDE_WAKEUP) || defined(RTOS_INCLUDE_SUSPEND_AND_RESUME) 
RTOS_RegInt rtos_WakeupTask(RTOS_Task *task)
{
	if (0 == task)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	// if ((RTOS_TASK_STATUS_WAITING != task->Status) && (RTOS_TASK_STATUS_SLEEPING != task->Status))
	if (!rtos_isSleeping(task) && (!rtos_isWaiting(task)))
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

#if defined(RTOS_SUPPORT_EVENTS)
	if (0 != task->WaitFor)
	{
		rtos_RemoveTaskWaiting(task->WaitFor, task);
	}
#endif

	rtos_RemoveFromSleepers(task);
	RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, task->Priority);

	task->CrossContextReturnValue = RTOS_ABORTED;

	return RTOS_OK;
}
#endif


#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
RTOS_RegInt RTOS_SuspendSelf(void)
{
	RTOS_Task *task;
	RTOS_TaskPriority priority;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(!RTOS_IsInsideIsr());
	RTOS_ASSERT(!RTOS_SchedulerIsLocked());
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
    	if (RTOS_IsInsideIsr())
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	} 
#endif
	RTOS_EnterCriticalSection(saved_state);

	if (RTOS_SchedulerIsLocked())
	{
		RTOS_ExitCriticalSection(saved_state);
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

	task = RTOS_CURRENT_TASK();
	priority = task->Priority;

	RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, priority);
	RTOS_TaskSet_AddMember(RTOS.SuspendedTasks, priority);

	RTOS_ExitCriticalSection(saved_state);

	RTOS_INVOKE_SCHEDULER();

	return RTOS_OK;	// This will only be reached on resume.
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
		result = RTOS_OK;
	}

	RTOS_ExitCriticalSection(saved_state);

	RTOS_REQUEST_RESCHEDULING();

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
#if defined(RTOS_SUPPORT_EVENTS)
				if (0 != task->WaitFor)
                {
					rtos_RemoveTaskWaiting(task->WaitFor, task);
				}
#endif
				RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, i);
				task->CrossContextReturnValue = RTOS_TIMED_OUT;
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
	while(1)
	{
		rtos_TimerTickFunction();
#if defined(RTOS_TIMER_EXTRA_ACTION)
		RTOS_TIMER_EXTRA_ACTION();
#endif
		RTOS_SuspendSelf();
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

	// Perhaps we need some action for SMP here too.
	// Check this when SMP features become more mature.
}

#else
void rtos_TimerTick(void)
{
	rtos_TimerTickFunction();

#if defined(RTOS_TIMER_EXTRA_ACTION)
		RTOS_TIMER_EXTRA_ACTION();
#endif
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

// The default assert function, if the assert fails just hang the system and go to an infinite loop.
// One will need a debugger to find out what has happened.
void rtos_DefaultAssert(int cond)
{
	RTOS_SavedCriticalState(saved_state);

	if (!cond)
	{
		// Make the system dead.
		RTOS_EnterCriticalSection(saved_state);
		while(1);
		RTOS_ExitCriticalSection(saved_state); // Not reachable, but pacifies GCC.
	}

}

// End of RTOS Core Functions.

