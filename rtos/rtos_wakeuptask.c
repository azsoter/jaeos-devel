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


