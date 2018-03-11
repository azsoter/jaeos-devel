#include <rtos.h>
#include <rtos_internals.h>

/*
* Copyright (c) Andras Zsoter 2016-2018.
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

#if defined(RTOS_SUPPORT_TIMESHARE)
// Utility functions to manipulate the time slice of a time share task.

RTOS_Time RTOS_GetTimeSlice(RTOS_Task *task)
{
	RTOS_Time slice;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == task)
	{
		return 0;
	}
#endif

	RTOS_EnterCriticalSection(saved_state);
	slice = task->TimeSliceTicks;
	RTOS_ExitCriticalSection(saved_state);

	return slice;
}

RTOS_Time RTOS_GetRemainingTicks(RTOS_Task *task)
{
	RTOS_Time ticks;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == task)
	{
		return 0;
	}
#endif

	RTOS_EnterCriticalSection(saved_state);
	ticks = task->TicksToRun;
	RTOS_ExitCriticalSection(saved_state);

	return ticks;
}

RTOS_RegInt RTOS_SetTimeSlice(RTOS_Task *task, RTOS_Time slice)
{
	RTOS_SavedCriticalState(saved_state);
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != task);
	// RTOS_ASSERT(rtos_IsTimeSharing(task->Priority));
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == task)
	{
		return  RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}

//	if (!rtos_IsTimeSharing(task->Priority))
//	{
//		return  RTOS_ERROR_FAILED;
//	}
#endif
	RTOS_EnterCriticalSection(saved_state);
		task->TimeSliceTicks = slice;

		if ((task->TicksToRun > slice) || (RTOS_TIMEOUT_FOREVER == task->TicksToRun) || (RTOS_TIMEOUT_FOREVER == slice))
		{
			task->TicksToRun = slice;
		}
	RTOS_ExitCriticalSection(saved_state);

	return RTOS_OK;
}

#endif

