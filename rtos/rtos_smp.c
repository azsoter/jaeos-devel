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

#if defined(RTOS_SMP)
void rtos_RestrictPriorityToCpus(RTOS_TaskPriority priority, RTOS_CpuMask cpus)
{
	RTOS_CpuId i;
	RTOS_CpuMask mask;

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
}

RTOS_RegInt RTOS_RestrictTaskToCpus(RTOS_Task *task, RTOS_CpuMask cpus)
{
	RTOS_RegInt isRunning = RTOS.IsRunning;

	RTOS_SavedCriticalState(saved_state = 0);

	if (0 == task)
	{
		return RTOS_ERROR_FAILED;
	}

	if (isRunning)
	{
		RTOS_EnterCriticalSection(saved_state);
	}

	rtos_RestrictPriorityToCpus(task->Priority, cpus);

	if (isRunning)
	{
		RTOS_ExitCriticalSection(saved_state);
	}

	return RTOS_OK;
}

// Retrieve which CPUs are allowed to run the specified task.
// There is no error reporting, if task is NULL if its priority is ivalid this function simply
// reports that the task is not allowed to run on any of the CPU cores.
RTOS_CpuMask RTOS_GetAllowedCpus(RTOS_Task *task)
{
	RTOS_CpuMask cpus = 0;
	RTOS_CpuId i;
	RTOS_CpuMask mask;
	RTOS_TaskPriority priority;
	RTOS_SavedCriticalState(saved_state);

	if (0 != task)
	{
		RTOS_EnterCriticalSection(saved_state);
		priority = task->Priority;
		for (mask = 1, i = 0; i < (RTOS_SMP_CPU_CORES); i++, mask <<= 1)
		{

			if (RTOS_TaskSet_IsMember(RTOS.TasksAllowed[i], priority))
			{
				cpus |= mask;
			}
		}
		RTOS_ExitCriticalSection(saved_state);
	}

	return cpus;
}

// -----------------------------------------------------------------------------------
RTOS_Task *RTOS_GetCurrentTask(void)
{
	RTOS_Task *thisTask;
	RTOS_SavedCriticalState(saved_state);

	RTOS_EnterCriticalSection(saved_state);
	thisTask = RTOS_CURRENT_TASK();
	RTOS_ExitCriticalSection(saved_state);

	return thisTask;
}

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

	cpu = RTOS_CurrentCpu();
	currentTask = RTOS.CurrentTasks[cpu];

#if defined(RTOS_SUPPORT_TIMESHARE)
	rtos_ManageTimeshared(currentTask);
#endif

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
	if (RTOS_SchedulerIsLocked())
	{
		return;
	}
#endif
	if (0 != currentTask)
	{
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

#if defined(RTOS_INVOKE_YIELD)
void rtos_SchedulerForYield(void)
{
	RTOS_TaskPriority currentPriority;
	RTOS_TaskPriority priority;
	RTOS_Task *task;
	RTOS_TaskSet tasks;
	RTOS_TaskSet t;
	RTOS_Task *currentTask;
	RTOS_TaskSet otherRunningTasks;
	RTOS_CpuId cpu = RTOS_CurrentCpu();
	currentTask = RTOS.CurrentTasks[cpu];

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

	tasks = RTOS_TaskSet_Intersection(tasks, RTOS.TasksAllowed[cpu]);
	otherRunningTasks = RTOS.RunningTasks;
	RTOS_TaskSet_RemoveMember(otherRunningTasks, currentPriority);
	tasks = RTOS_TaskSet_Difference(tasks, otherRunningTasks);	// Don't pick one of the running tasks (except the one trying to yield()).

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
	}
	else
	{
		RTOS_TaskSet_RemoveMember(RTOS.RunningTasks, currentPriority);
		RTOS.CurrentTasks[cpu]->Cpu = RTOS_CPUID_NO_CPU;
		RTOS.CurrentTasks[cpu] = task;	// In SMP configurations it is OK to have a task that is NULL.
	}
}
#endif /* RTOS_INVOKE_YIELD */

#if 0
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

#endif

