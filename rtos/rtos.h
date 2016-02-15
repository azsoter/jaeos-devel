#ifndef RTOS_H
#define RTOS_H
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

#ifdef __cplusplus
#include <climits>
extern "C" {
#else
#include <limits.h>
#endif

#include <rtos_config.h>

#if !defined(RTOS_Priority_Highest)
#error RTOS_Priority_Highest must be defined.
#endif

#if ((RTOS_Priority_Highest) < 0)
#error RTOS_Priority_Highest must be >= 0.
#endif

#if defined(RTOS_SMP)
#	warning You have enabled SMP -- SMP is still an experimental feature.

#	if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
#	error Scheduler Lock is not supported in SMP mode.
#	endif

#	if !defined(RTOS_SMP_CPU_CORES)
#		error RTOS_SMP_CPU_CORES must be defined if SMP support is enabled.
#	endif

#	if (RTOS_SMP_CPU_CORES) < 1
#		error RTOS_SMP_CPU_CORES must be >= 1.
#	endif

#	if defined(RTOS_SUPPORT_TIMESHARE)

#		if !defined(RTOS_TIMESHARE_PARALLEL_MAX)
#			define RTOS_TIMESHARE_PARALLEL_MAX (RTOS_SMP_CPU_CORES)
#		endif

#	endif

#endif

#if defined(RTOS_USE_TIMER_TASK)
#if !defined(RTOS_Priority_Timer)
#error If a timer task is to be used RTOS_Priority_Timer must be defined in the application config header.
#endif
#define RTOS_INCLUDE_SUSPEND_AND_RESUME
#endif

#include <rtos_types.h>

// Status Codes:
#define RTOS_ABORTED       			 2	
#define RTOS_TIMED_OUT                         	 1
#define RTOS_OK                                	 0
#define RTOS_ERROR_FAILED			-1
#define RTOS_ERROR_OPERATION_NOT_PERMITTED     	-2
#define RTOS_ERROR_PRIORITY_IN_USE       	-3
#define RTOS_ERROR_OVERFLOW			-4

#define RTOS_Priority_Idle			0

// Types used by the OS.
typedef volatile struct rtos_EventHandle RTOS_EventHandle;
typedef volatile struct rtos_Semaphore RTOS_Semaphore;
typedef volatile struct rtos_Task RTOS_Task;

#if defined(RTOS_TASKSET_TYPE)
typedef volatile RTOS_TASKSET_TYPE RTOS_TaskSet;
#else
typedef volatile uint32_t RTOS_TaskSet;
#undef RTOS_HIGHEST_SUPPORTED_TASK_PRIORITY
#define RTOS_HIGHEST_SUPPORTED_TASK_PRIORITY 31
#endif

#if defined(RTOS_TIME_TYPE)
typedef RTOS_TIME_TYPE RTOS_Time;
#else
typedef uint32_t RTOS_Time;
#endif

#if defined(RTOS_REG_INT_TYPE)
typedef signed   RTOS_REG_INT_TYPE RTOS_RegInt;
typedef unsigned RTOS_REG_INT_TYPE RTOS_RegUInt;
#else
typedef signed   int RTOS_RegInt;
typedef unsigned int RTOS_RegUInt;
#endif

#if defined(RTOS_TASKPRIORITY_TYPE)
typedef RTOS_TASKPRIORITY_TYPE RTOS_TaskPriority;
#else
typedef RTOS_RegUInt RTOS_TaskPriority;
#endif

#if ((RTOS_Priority_Highest) > (RTOS_HIGHEST_SUPPORTED_TASK_PRIORITY))
#error RTOS_Priority_Highest is higher than the maximum supported on this target.
#endif

#if defined(RTOS_INCLUDE_NAKED_EVENTS) || defined(RTOS_INCLUDE_SEMAPHORES)
#define RTOS_SUPPORT_EVENTS
#endif

#if defined(RTOS_SUPPORT_EVENTS) || defined(RTOS_INCLUDE_DELAY)
#define RTOS_SUPPORT_SLEEP
#endif

// Doubly linked lists.
struct rtos_Task_DLList
{
	RTOS_Task *Head;
	RTOS_Task *Tail;
};
typedef struct rtos_Task_DLList RTOS_Task_DLList;

struct rtos_Task_DLLink
{
	RTOS_Task *Previous;
	RTOS_Task *Next;
};
typedef struct rtos_Task_DLLink RTOS_Task_DLLink;

// The main RTOS structure representing the internal state of the operating system.
struct rtos_OS
{
#if defined(RTOS_SMP)
	RTOS_Task     		*CurrentTasks[RTOS_SMP_CPU_CORES];	// The currently running task on each CPU.
	RTOS_RegUInt   		InterruptNesting[RTOS_SMP_CPU_CORES];	// Level of interrupts nested.
	RTOS_TaskSet		TasksAllowed[RTOS_SMP_CPU_CORES]; 	// Tasks allowed to run on this particular core.
	RTOS_CpuMutex		OSLock;					// Global lock to prevent access of OS structures from other CPUs.
	RTOS_TaskSet    	RunningTasks;				// Tasks that are currently running on any CPU.
	RTOS_CpuMask		Cpus;					// A bitmap of all CPUs.
	RTOS_CpuMask		CpuHoldingPen;				// CPUs in a 'holding pen' (CPU's not running any task).
#else
	RTOS_Task     		*CurrentTask;				// The currently running task.
	RTOS_RegUInt   		InterruptNesting;			// Level of interrupts nested.
#endif
#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
	RTOS_RegUInt		SchedulerLocked;			// Is the scheduler locked.
#endif
	RTOS_RegInt		IsRunning;				// Is the OS running?
	RTOS_Time		Time;					// System time in ticks.
	RTOS_TaskSet    	ReadyToRunTasks;			// Tasks that are ready to run.
#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
	RTOS_TaskSet		SuspendedTasks;				// Suspended tasks.
#endif
#if defined(RTOS_SUPPORT_TIMESHARE)
	RTOS_TaskSet		TimeshareTasks;				// Tasks subject to time share scheduling.
	RTOS_TaskSet		PreemptedTasks;				// Tasks preempted because their time slice has expired.
	RTOS_Task_DLList	PreemptedList;
#if defined(RTOS_SMP)
	RTOS_CpuMask		TimeShareCpus;				// CPUs running timeshare tasks.
	RTOS_RegUInt		TimeshareParallelAllowed;		// The number of time share tasks allowed to run at the same time on different CPUs.
#endif
#endif
	RTOS_Task     		*TaskList[(RTOS_Priority_Highest) + 1];	// A list (really an array) of pointers to all the task structures.
#if defined(RTOS_SUPPORT_SLEEP)
	RTOS_Task     		*Sleepers[(RTOS_Priority_Highest) + 1]; // All the sleeping tasks.
#endif
};

typedef volatile struct rtos_OS RTOS_OS;

extern RTOS_OS RTOS;
 
#if defined(RTOS_SMP)
extern RTOS_RegInt RTOS_RestrictTaskToCpus(RTOS_Task *task, RTOS_CpuMask cpus);
extern RTOS_CpuMask RTOS_GetAllowedCpus(RTOS_Task *task);
#endif

extern RTOS_Task *RTOS_GetCurrentTask(void);

extern RTOS_Time RTOS_GetTime(void);
// extern void RTOS_SetTime(RTOS_Time time);

// Represents an 'event' that a task can wait for.
// Only used to implement other constructs for synchronization.
struct rtos_EventHandle
{
	RTOS_TaskSet    	TasksWaiting;	// The tasks waiting for this event.
#if defined(RTOS_SUPPORT_TIMESHARE)
	RTOS_Task_DLList	WaitList;
#endif
};

typedef unsigned int RTOS_SemaphoreCount;

// A counting sempahore.
struct rtos_Semaphore
{
	RTOS_EventHandle	Event;		// A generic 'event' structure suitable for waiting.
	RTOS_SemaphoreCount	Count;		// The semaphore's count.
};

#define RTOS_SEMAPHORE_COUNT_MAX (UINT_MAX)
#define RTOS_InitSemaphore(S, COUNT) (S)->Count = (COUNT)
extern RTOS_RegInt RTOS_CreateSemaphore(RTOS_Semaphore *semaphore, RTOS_SemaphoreCount initialCount);

extern RTOS_SemaphoreCount RTOS_PeekSemaphore(RTOS_Semaphore *semaphore);

extern RTOS_RegInt  RTOS_PostSemaphore(RTOS_Semaphore *semaphore);
extern RTOS_RegInt  RTOS_GetSemaphore(RTOS_Semaphore *semaphore, RTOS_Time timeout);

// Structure representing a thread of execution (known as a task in RTOS parlance).
struct rtos_Task
{
	void                	*SP;				// Stack Pointer.
	void                	*SP0;				// Botton of the stack  (for debugging).
	RTOS_TaskPriority  	Priority;			// The tasks priority.
	RTOS_EventHandle   	*WaitFor;			// Event the task is waiting for.
	RTOS_Time      		WakeUpTime;			// Time when to wake up.
	void            	(*Action)(void *);		// The main loop of the task.
	void            	*Parameter;			// Parameter to Action().
#if defined(RTOS_SUPPORT_TIMESHARE)
	RTOS_Time		TicksToRun;			// Ticks remaining from the current timeslice.
	RTOS_Time		TimeSliceTicks;			// The length of a full time slice for this task.
	RTOS_Time		TimeWatermark;			// A time when the task was last seen running.
	RTOS_Task_DLLink	Link;
	RTOS_RegInt		IsTimeshared;			// Is this task time sliced.
#endif
#if defined(RTOS_SMP)
	RTOS_CpuId		Cpu;				// The CPU the task is running on.
#endif
	RTOS_RegInt		Status;				// Task's internal status.
#if defined(RTOS_TARGET_SPECIFIC_TASK_DATA)
RTOS_TARGET_SPECIFIC_TASK_DATA 
#endif

#if defined(RTOS_TASK_NAME_LENGTH)
	char			TaskName[RTOS_TASK_NAME_LENGTH];
#endif
};

#define RTOS_TIMEOUT_FOREVER (~(RTOS_Time)0)

#include <rtos_target.h>

#if !defined(RTOS_IsInsideIsr)
#if defined(RTOS_SMP)
// For bring up only.
// This definition does not always work. For multi-core operation define something more reliable in the actual target specific code.
#define RTOS_IsInsideIsr() (0 != RTOS.InterruptNesting[RTOS_CurrentCpu()])
#else
#define RTOS_IsInsideIsr() (0 != RTOS.InterruptNesting)
#endif
#endif

#if !defined(RTOS_TICKS_PER_SECOND)
#define RTOS_TICKS_PER_SECOND 100	/* Some reasonable default. */
#endif

// Always rund up.
#define RTOS_MS_TO_TICKS(MS) ((999 + ((RTOS_TICKS_PER_SECOND) * (MS))) / 1000)
#define RTOS_TICKS_TO_MS(TICKS) (((1000 * (TICKS)) + (RTOS_TICKS_PER_SECOND) - 1) / (RTOS_TICKS_PER_SECOND))

// This function should be called either during initialization (before the OS is running)
// or from inside a critical section.
extern RTOS_RegInt RTOS_CreateTask(RTOS_Task *task, const char *name, RTOS_TaskPriority priority, void *sp0, unsigned long stackCapacity, void (*f)(void *), void *param);
extern RTOS_Task *RTOS_TaskFromPriority(RTOS_TaskPriority priority);

#if defined(RTOS_SUPPORT_TIMESHARE)
// This function should be called either during initialization (before the OS is running)
// or from inside a critical section.
extern RTOS_RegInt RTOS_CreateTimeShareTask(RTOS_Task *task, const char *name, RTOS_TaskPriority priority, void *sp0, unsigned long stackCapacity, void (*f)(void *), void *param, RTOS_Time slice);
extern void RTOS_YieldTimeSlice(void);
extern RTOS_Time RTOS_GetTimeSlice(RTOS_Task *task);
extern RTOS_Time RTOS_GetRemainingTicks(RTOS_Task *task);
extern RTOS_RegInt RTOS_SetTimeSlice(RTOS_Task *task, RTOS_Time slice);
#endif

#if defined(RTOS_TASK_NAME_LENGTH)
extern void RTOS_SetTaskName(RTOS_Task *task, const char *name);
#define RTOS_GetTaskName(T) ((const char *)((T)->TaskName))
#endif

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
extern RTOS_RegInt RTOS_LockScheduler(void);
extern RTOS_RegInt RTOS_UnlockScheduler(void);
#endif

#if defined(RTOS_INCLUDE_DELAY)
extern RTOS_RegInt RTOS_Delay(RTOS_Time ticksToSleep);
extern RTOS_RegInt RTOS_DelayUntil(RTOS_Time wakeUpTime);
#endif

#if defined(RTOS_INVOKE_YIELD)
extern void RTOS_YieldPriority(void);
#endif

extern RTOS_RegInt RTOS_CreateEventHandle(RTOS_EventHandle *event);
extern RTOS_RegInt RTOS_WaitForEvent(RTOS_EventHandle *event, RTOS_Time timeout);
extern RTOS_RegInt RTOS_SignalEvent(RTOS_EventHandle *event);

extern RTOS_RegInt RTOS_ChangePriority(RTOS_Task *task, RTOS_TaskPriority targetPriority);

extern RTOS_RegInt RTOS_KillSelf(void);
extern RTOS_RegInt RTOS_KillTask(RTOS_Task *task);

#if defined(RTOS_USE_TIMER_TASK)
extern void RTOS_DefaultTimerFunction(void *p);
#endif

#if defined(RTOS_INCLUDE_SUSPEND_AND_RESUME)
extern RTOS_RegInt RTOS_SuspendSelf(void);
extern RTOS_RegInt RTOS_SuspendTask(RTOS_Task *task);
extern RTOS_RegInt RTOS_ResumeTask(RTOS_Task *task);
#endif

#if defined(RTOS_INCLUDE_WAKEUP)
#if !defined(RTOS_SUPPORT_SLEEP)
#error No sleep operation is supported, there is nothing to wake up from.
#endif
extern RTOS_RegInt RTOS_WakeupTask(RTOS_Task *task);
#endif

#if !defined(RTOS_INLINE)
#define RTOS_INLINE
#endif

#ifndef RTOS_ASSERT
extern void rtos_DefaultAssert(int cond);
#define RTOS_ASSERT(Cond) rtos_DefaultAssert(Cond)
#endif

#ifdef __cplusplus
}
#endif

#endif

