#ifndef RTOS_INTERNALS_H
#define RTOS_INTERNALS_H
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

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(RTOS_TARGET_SUPPLIES_SET_OPERATIONS)
// Operations on task sets.
#define RTOS_TaskSet_IsEmpty(S) (0 == (S))
#define RTOS_TaskSet_AddMember(S,I) (S |= (((RTOS_TaskSet)1) << (I)))
#define RTOS_TaskSet_RemoveMember(S,I) (S &= ~(((RTOS_TaskSet)1) << (I)))
#define RTOS_TaskSet_IsMember(S,I) ( 0 != (S & (((RTOS_TaskSet)1) << (I))))
#define RTOS_TaskSet_Intersection(S1, S2) ((S1) & (S2))
#define RTOS_TaskSet_Union(S1, S2) ((S1) | (S2))
#endif

#if defined(RTOS_INCLUDE_SCHEDULER_LOCK)
#define RTOS_SchedulerIsLocked() (0 != RTOS.SchedulerLocked)
#else
#define RTOS_SchedulerIsLocked() 0
#endif

// This function must be defined by the target port.
extern void rtos_TargetInitializeTask(RTOS_Task *task, unsigned long stackCapacity);

// Initialization.
extern RTOS_RegInt rtos_CreateTask(RTOS_Task *task, RTOS_TaskPriority priority, void *sp0, unsigned long stackCapacity, void (*f)(), void *param);

#define RTOS_SET_CURRENT_TASK(TASK) RTOS.CurrentTask = (TASK)

// These should only be called by the target port or other OS components.
extern void rtos_TimerTick(void);
extern void rtos_Scheduler(void);
extern void rtos_SchedulerForYield(void);
extern void rtos_RunTask(void);

#if defined(RTOS_SUPPORT_TIMESHARE)
// Doubly-Linked Lists.
extern void rtos_AppendToTaskDList(volatile RTOS_Task_DLList *list, RTOS_RegInt which, RTOS_Task *task);
extern void rtos_RemoveFromTaskDList(volatile RTOS_Task_DLList *list, RTOS_RegInt which, RTOS_Task *task);
extern RTOS_Task *rtos_RemoveFirstTaskFromDList(volatile RTOS_Task_DLList *list, RTOS_RegInt which);

extern void rtos_PreemptTask(RTOS_Task *task);
extern void rtos_SchedulePeer(void);
extern void rtos_DeductTick(RTOS_Task *task);
// extern RTOS_RegInt rtos_AdjustTimeSlice(void);
// extern void rtos_AdjustRemainingTimeSlice(RTOS_Task *task);
extern void rtos_ManageTimeshared(RTOS_Task *task);
#endif

#ifdef __cplusplus
}
#endif

#endif
