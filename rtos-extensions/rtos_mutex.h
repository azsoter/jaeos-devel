/*
* Copyright (c) Andras Zsoter 2018.
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

#ifndef RTOS_MUTEX_H
#define RTOS_MUTEX_H

#include <rtos.h>

#if defined(RTOS_INLCUDE_MUTEXES)
// A Mutex with immediate priority boost (ICPP).
//
// I am not a fan of manipulating task priorities to avoid priority inversion, and prefer using
// separate high priority tasks to perform urgent operations.
//
// Acquiring a mutex that also causes a priority change is a more expensive operation
// than e.g. using a semaphore as a simple lock.
// The main advantages are that a mutex can conceptually be owned by a task which allows some extra features
// namely a priority boost and optionally the ability to re-acquire a mutex which is being held by the same task.
// These operations may not be the appropriate solution for a lot of problems that programmers may use them for,
// and should only be used if the problem cannot be solved in some other way.
//
// Nevertheless, there may be situations when it is appropriate to boots task priority when it acquires a resource.
// Or simply it may be necessary to support existing code that assumes that such operation exists.
//
// Because the OS runs each task at a separate priority level, it would be impractical to boost the priority
// of the task holding the resource to that of the highest one waiting (priority inheritance).
//
// A dedicated priority level can be reserved however to run the task at while holding the resource (priority ceiling).
// For simplicity the priority is boosted immediately upon acquiring the mutex, regardless of who is waiting for it.
//
// The priority of the task acquiring the mutex is boosted to an elevated (ceiling) priority.
// If the task already has a higher priority then the elevated priority level, it will continue to run at its own priority.
// Do not use any other API function that manipulates the priority of the task holding a mutex, and do not try to
// terminate the task while holding a mutex. Those actions will likely end up rendering the system non-functional.
//
// Restriction:
// Multiple mutexes can be acquired by a task but they MUST be released in the reverse order as they were acquired (LIFO),
// in order to properly rewind all the priority changes.
// Ordering the acquisition of locks is generally necessary to avoid deadlocks, so releasing them in the reverse order
// should feel natural.
//
// Mutexes can be configured to either allow locking only once or to allow recursive locking --
// i.e lock the same mutex multiple times from the same task without releasing it first.
// This is a compile time configuration option that applies to all the mutexes in the system.
//
// The mutex's priority level is reserved when the mutex is created and no task or other mutex can use it.
// It means priority levels assigned to mutexes come out of the total number of priority levels in the system.
// This puts a limit on how many mutexes with a priority boost can exist in the system.
// The priority level is released (no longer reserved) when the mutex is uninitialized (destroyed).
//
// The priority can be specified as 0, in which case no priority boost is taking place when the mutex is acquired.
// There is no restriction on the number of mutexes with priority set to 0 in the system.
//

struct rtos_Mutex
{
        RTOS_EventHandle  Event;             // A generic 'event' structure suitable for waiting.
        RTOS_Task         *Owner;            // The owner of the mutex.
        RTOS_TaskPriority CeilingPriority;   // The used while the task has the resource.
        RTOS_TaskPriority OwnerPriority;     // The previous priority of the owner.
#if defined(RTOS_MUTEX_IS_RECURSIVE)
        RTOS_SemaphoreCount ExtraLockCount;  // Count how many times a recursive mutex has been locked.
#endif
};

typedef volatile struct rtos_Mutex RTOS_Mutex;

extern RTOS_RegInt RTOS_LockMutex(RTOS_Mutex *mutex, RTOS_Time timeout);
extern RTOS_RegInt RTOS_UnlockMutex(RTOS_Mutex *mutex);
extern RTOS_RegInt RTOS_CreateMutex(RTOS_Mutex *mutex, RTOS_TaskPriority CeilingPriority);
extern RTOS_RegInt RTOS_DestroyMutex(RTOS_Mutex *mutex);

#endif

#endif
