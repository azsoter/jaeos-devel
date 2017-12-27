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

#if defined(RTOS_INCLUDE_SEMAPHORES)
RTOS_RegInt RTOS_CreateSemaphore(RTOS_Semaphore *semaphore, RTOS_SemaphoreCount initialCount)
{
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != semaphore);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == semaphore)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif
	semaphore->Count = initialCount;
	return RTOS_CreateEventHandle(&(semaphore->Event));
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

		RTOS_REQUEST_RESCHEDULING()
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
	volatile RTOS_Task *thisTask;
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
	// tasks cannot sleep either if the scheduler is locked.
    	if ((RTOS_IsInsideIsr()) || (RTOS_SchedulerIsLocked()))
    	{
        	RTOS_ExitCriticalSection(saved_state);
        	return (0 == timeout) ?  RTOS_TIMED_OUT : RTOS_ERROR_OPERATION_NOT_PERMITTED;
    	}

	thisTask = RTOS_CURRENT_TASK();

	if (0 == timeout)
	{
    	RTOS_ExitCriticalSection(saved_state);
    	return RTOS_TIMED_OUT;
	}
 
    rtos_WaitForEvent(&(semaphore->Event), thisTask, timeout);

    RTOS_ExitCriticalSection(saved_state);

    RTOS_INVOKE_SCHEDULER();

    return thisTask->CrossContextReturnValue;
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

