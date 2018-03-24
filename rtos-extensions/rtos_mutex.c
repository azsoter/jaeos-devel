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

#include <rtos.h>
#include <rtos_internals.h>
#include <rtos_mutex.h>

#if defined(RTOS_INLCUDE_MUTEXES)

RTOS_RegInt RTOS_LockMutex (RTOS_Mutex * mutex, RTOS_Time timeout)
{
        RTOS_RegInt res;
        volatile RTOS_Task *thisTask = RTOS_CURRENT_TASK();
        RTOS_TaskPriority old_priority, elevated_priority;
        RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
        RTOS_ASSERT(0 != mutex);
        RTOS_ASSERT((!RTOS_IsInsideIsr()));
#if defined(RTOS_MUTEX_IS_RECURSIVE)
        RTOS_ASSERT(((!RTOS_SchedulerIsLocked()) || (0 == timeout) || (thisTask == mutex->Owner)));
#endif
        RTOS_ASSERT(((!RTOS_SchedulerIsLocked()) || (0 == timeout)));
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
        if (0 == mutex)
        {
                return RTOS_ERROR_OPERATION_NOT_PERMITTED;
        }

        if (RTOS_IsInsideIsr())
        {
                return RTOS_ERROR_OPERATION_NOT_PERMITTED;
        }
#endif

        RTOS_EnterCriticalSection(saved_state);

        elevated_priority = mutex->CeilingPriority;
        // thisTask = RTOS_CURRENT_TASK();

        if (0 == mutex->Owner)
        {
                old_priority = thisTask->Priority;
                mutex->OwnerPriority = old_priority;
                mutex->Owner = thisTask;

                if (elevated_priority > old_priority)
                {
                         thisTask->Priority = elevated_priority;
                         RTOS.TaskList[elevated_priority] = thisTask;
                         RTOS_TaskSet_RemoveMember (RTOS.ReadyToRunTasks, old_priority);
                         RTOS.TaskList[old_priority] = 0;
                         RTOS_TaskSet_AddMember (RTOS.ReadyToRunTasks, elevated_priority);
                }
                RTOS_INVOKE_SCHEDULER();
                res = RTOS_OK;
        }
        else
        {
#if defined(RTOS_MUTEX_IS_RECURSIVE)
                if (thisTask == mutex->Owner)
                {
                	if ((RTOS_SEMAPHORE_COUNT_MAX) != mutex->ExtraLockCount)
                	{
                        mutex->ExtraLockCount++;
                        res = RTOS_OK;
                	}
                	else
                	{
                		res = RTOS_ERROR_OVERFLOW;
                	}
                }
                else
                {
#endif
                	if (0 == timeout)
                	{
                		res = RTOS_TIMED_OUT;
                	}
                	else
                	{
                		if (RTOS_SchedulerIsLocked())
                		{
                    		res = RTOS_ERROR_OPERATION_NOT_PERMITTED;
                		}
                		else
                		{
                			rtos_WaitForEvent(&(mutex->Event), thisTask, timeout);
                			RTOS_INVOKE_SCHEDULER();
                			res = thisTask->CrossContextReturnValue;
                		}
                	}
#if defined(RTOS_MUTEX_IS_RECURSIVE)
                }
#endif
        }

        RTOS_ExitCriticalSection(saved_state);

        return res;
}

RTOS_RegInt RTOS_UnlockMutex(RTOS_Mutex *mutex)
{
        //RTOS_RegInt res;
        volatile RTOS_Task *thisTask = RTOS_CURRENT_TASK();
        volatile RTOS_Task *waitingTask = 0;
        RTOS_TaskPriority this_priority, waiting_priority, elevated_priority, old_priority;
        RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
        RTOS_ASSERT(0 != mutex);
        RTOS_ASSERT((!RTOS_IsInsideIsr()));
        // RTOS_ASSERT((!RTOS_SchedulerIsLocked()));
        RTOS_ASSERT(thisTask == mutex->Owner);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
        if (0 == mutex)
        {
                return RTOS_ERROR_OPERATION_NOT_PERMITTED;
        }

        if (RTOS_IsInsideIsr())
        {
                return RTOS_ERROR_OPERATION_NOT_PERMITTED;
        }
#endif

        if (thisTask != mutex->Owner)
        {
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
        }

        RTOS_EnterCriticalSection(saved_state);

        old_priority = mutex->OwnerPriority;

#if defined(RTOS_MUTEX_IS_RECURSIVE)
        if (0 != mutex->ExtraLockCount)
        {
            mutex->ExtraLockCount--;
            RTOS_ExitCriticalSection(saved_state);
            return RTOS_OK;
        }
#endif

        this_priority = thisTask->Priority;
        elevated_priority = mutex->CeilingPriority;

        // If the current task had elevated priority move it back to its normal place.
        if (old_priority != this_priority)
        {
#if defined(RTOS_USE_ASSERTS)
    RTOS_ASSERT((elevated_priority == this_priority));
#endif
                if (elevated_priority != this_priority)
                {
                    // We are trying to release mutexes in the wrong order, or some similar problem.
                	// Mutexes MUST be released in LIFO order!
                    RTOS_ExitCriticalSection(saved_state);
                    return RTOS_ERROR_FAILED;
                }

                RTOS_TaskSet_RemoveMember(RTOS.ReadyToRunTasks, elevated_priority);
                RTOS.TaskList[elevated_priority] = 0;

                thisTask->Priority = old_priority;
                RTOS.TaskList[old_priority] = thisTask;
                RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, old_priority);
        }

        waitingTask = rtos_GetFirstWaitingTask (&(mutex->Event));

        if (0 != waitingTask)
        {
                waiting_priority = waitingTask->Priority;
                waitingTask->CrossContextReturnValue = RTOS_OK;

                mutex->OwnerPriority = waiting_priority;

                if (waiting_priority < elevated_priority)
                {
                    // Elevate the priority of the waiting task.
                    waitingTask->Priority = elevated_priority;
                    RTOS.TaskList[elevated_priority] = waitingTask;
                    RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, elevated_priority);
                    RTOS.TaskList[waiting_priority] = 0;
                }
                else
                {
                    RTOS_TaskSet_AddMember(RTOS.ReadyToRunTasks, waiting_priority);
                }
        }
        else
        {
                mutex->OwnerPriority = RTOS_InvalidPriority;
                mutex->Owner = 0;
        }

        RTOS_ExitCriticalSection(saved_state);

        RTOS_INVOKE_SCHEDULER();

        return RTOS_OK;
}

RTOS_RegInt RTOS_CreateMutex(RTOS_Mutex *mutex, RTOS_TaskPriority CeilingPriority)
{
	RTOS_RegInt result = RTOS_OK;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
    RTOS_ASSERT(0 != mutex);
    RTOS_ASSERT(CeilingPriority <= (RTOS_Priority_Highest));
#endif

    if (0 == mutex)
    {
    	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
    }

	if (CeilingPriority > (RTOS_Priority_Highest))
	{
		return RTOS_ERROR_INVALID_PRIORITY;
	}

	if (RTOS.IsRunning)
	{
		RTOS_EnterCriticalSection(saved_state);
	}

	if (0 != CeilingPriority)
	{
		if (RTOS_TaskSet_IsMember(RTOS.PrioritiesInUse, CeilingPriority))
		{
			result = RTOS_ERROR_PRIORITY_IN_USE;
		}
		else
		{
			RTOS_TaskSet_AddMember(RTOS.PrioritiesInUse, CeilingPriority);
		}
	}

	if (RTOS.IsRunning)
	{
		RTOS_ExitCriticalSection(saved_state);
	}

	if (RTOS_OK != result)
	{
		return result;
	}

	mutex->Owner = 0;
	mutex->OwnerPriority = RTOS_InvalidPriority;
	mutex->CeilingPriority = CeilingPriority;
#if defined(RTOS_MUTEX_IS_RECURSIVE)
	mutex->ExtraLockCount = 0;
#endif
	return RTOS_CreateEventHandle(&(mutex->Event));
}

RTOS_RegInt RTOS_DestroyMutex(RTOS_Mutex *mutex)
{
	RTOS_SavedCriticalState(saved_state);
#if defined(RTOS_USE_ASSERTS)
    RTOS_ASSERT(0 != mutex);
#endif
    if (0 == mutex)
    {
    	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
    }

    RTOS_EnterCriticalSection(saved_state);

    if (0 != mutex->Owner)
    {
    	RTOS_ExitCriticalSection(saved_state);
    	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
    }

    if ((0 != mutex->CeilingPriority) && (mutex->CeilingPriority <= RTOS_Priority_Highest))
    {
    	RTOS_TaskSet_RemoveMember(RTOS.PrioritiesInUse, mutex->CeilingPriority);
    	mutex->CeilingPriority = 0;
    }

    RTOS_ExitCriticalSection(saved_state);
    return RTOS_OK;
}
#endif
