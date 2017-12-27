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

#if defined(RTOS_INCLUDE_NAKED_EVENTS)
RTOS_RegInt RTOS_WaitForEvent(RTOS_EventHandle *event, RTOS_Time timeout)
{
	volatile RTOS_Task *thisTask;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != event);
	RTOS_ASSERT(((!RTOS_IsInsideIsr()) || (0 == timeout)));
	RTOS_ASSERT(((!RTOS_SchedulerIsLocked()) || (0 == timeout)));
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == event)
	{
        	return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	if (0 == timeout)
	{
		return RTOS_TIMED_OUT;
	}

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
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

	return thisTask->CrossContextReturnValue;
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

	RTOS_REQUEST_RESCHEDULING();

	return result;
}
#endif

