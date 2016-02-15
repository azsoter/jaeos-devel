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
#include <rtos_queue.h>

RTOS_RegInt RTOS_CreateQueue(RTOS_Queue *queue, void *buffer, RTOS_QueueCount size)
{
	RTOS_RegInt result;
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != queue);
	RTOS_ASSERT(0 != size);
	RTOS_ASSERT(0 != buffer);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if ((0 == queue) || (0 == size) || (0 == buffer))
	{
		return  RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	queue->Head = 0;
	queue->Tail = 0;
	result = RTOS_CreateSemaphore(&(queue->SemFilledSlots), 0);
	result = RTOS_CreateSemaphore(&(queue->SemEmptySlots), size);

	if (RTOS_OK == result)
	{
		queue->Size = size;
		queue->Buffer = buffer;
	}
	else
	{
		queue->Size = 0;
		queue->Buffer = 0;
	}

	return result;
}

RTOS_INLINE RTOS_QueueCount rtos_NextIndexInQueue(RTOS_Queue *queue, RTOS_QueueCount ix)
{
	ix++;

	if (ix >= queue->Size)
	{
		ix = 0;
	}

	return ix;
}

RTOS_RegInt RTOS_Enqueue(RTOS_Queue *queue, void *message, RTOS_Time timeout)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != queue);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == queue)
	{
		return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif
	if ((0 != timeout) && (0 != RTOS_IsInsideIsr()))
	{
		return RTOS_ERROR_FAILED;
	}

	result = RTOS_GetSemaphore(&(queue->SemEmptySlots), timeout);

	if (RTOS_OK == result)
	{

		RTOS_EnterCriticalSection(saved_state);

		// Check if the queue was destroyed by another thread after the GetSempahore() operation.
		if ((0 == queue->Size) || (0 == queue->Buffer))
		{
			result = RTOS_ERROR_OPERATION_NOT_PERMITTED;
		}
		else
		{
			queue->Buffer[queue->Tail] = message;
			queue->Tail = rtos_NextIndexInQueue(queue, queue->Tail);
		}

		RTOS_ExitCriticalSection(saved_state);
	}

	if (RTOS_OK == result)
	{
		return RTOS_PostSemaphore(&(queue->SemFilledSlots));
	}

	return result;
}

// Push a message (back) to a queue, i.e. prepend it at the head of the queue.
RTOS_RegInt RTOS_PrependQueue(RTOS_Queue *queue, void *message, RTOS_Time timeout)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != queue);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == queue)
	{
		return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif
	if ((0 != timeout) && (0 != RTOS_IsInsideIsr()))
	{
		return RTOS_ERROR_FAILED;
	}

	result = RTOS_GetSemaphore(&(queue->SemEmptySlots), timeout);

	if (RTOS_OK == result)
	{

		RTOS_EnterCriticalSection(saved_state);

		// Check if the queue was destroyed by another thread after the GetSempahore() operation.
		if ((0 == queue->Size) || (0 == queue->Buffer))
		{
			result = RTOS_ERROR_OPERATION_NOT_PERMITTED;
		}
		else
		{
			if (0 == queue->Head)
			{
				queue->Head = queue->Size - 1;
			}
			else
			{
				queue->Head -= 1;
			}
			queue->Buffer[queue->Head] = message;
		}

		RTOS_ExitCriticalSection(saved_state);
	}

	if (RTOS_OK == result)
	{
		return RTOS_PostSemaphore(&(queue->SemFilledSlots));
	}

	return result;
}

RTOS_RegInt RTOS_Dequeue(RTOS_Queue *queue, void **message, RTOS_Time timeout)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != queue);
	RTOS_ASSERT(0 != message);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if ((0 == queue) || (0 == message))
	{
		return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	if ((0 != timeout) && (0 != RTOS_IsInsideIsr()))
	{
		return RTOS_ERROR_FAILED;
	}

	result = RTOS_GetSemaphore(&(queue->SemFilledSlots), timeout);

	if (RTOS_OK == result)
	{
		RTOS_EnterCriticalSection(saved_state);

		// Check if the queue was destroyed by another thread after the GetSempahore() operation.
		if ((0 == queue->Size) || (0 == queue->Buffer))
		{
			result = RTOS_ERROR_OPERATION_NOT_PERMITTED;
		}
		else
		{
			*message = queue->Buffer[queue->Head];
			queue->Head = rtos_NextIndexInQueue(queue, queue->Head);
		}

		RTOS_ExitCriticalSection(saved_state);
	}

	if (RTOS_OK == result)
	{
		RTOS_PostSemaphore(&(queue->SemEmptySlots));
	}

	return result;
}

// Take a peek at the first element in the queue but do not remove it.
RTOS_RegInt RTOS_PeekQueue(RTOS_Queue *queue, void **message)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);
#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != queue);
	RTOS_ASSERT(0 != message);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if ((0 == queue) || (0 == message))
	{
		return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif
		
	RTOS_EnterCriticalSection(saved_state);

	if ((0 == queue->Size) || (0 == queue->Buffer))
	{
		result = RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
	else
	{
		if (0 == RTOS_PeekSemaphore(&(queue->SemFilledSlots)))
		{
			// Queue is empty.
			result = RTOS_ERROR_FAILED;
		}
		else
		{
			*message = queue->Buffer[queue->Head];
			result = RTOS_OK;
		}
	}

	RTOS_ExitCriticalSection(saved_state);

	return result;
}

RTOS_RegInt RTOS_DestroyQueue(RTOS_Queue *queue)
{
	RTOS_RegInt result;
	RTOS_SavedCriticalState(saved_state);

#if defined(RTOS_USE_ASSERTS)
	RTOS_ASSERT(0 != queue);
#endif

#if !defined(RTOS_DISABLE_RUNTIME_CHECKS)
	if (0 == queue)
	{
		return RTOS_ERROR_OPERATION_NOT_PERMITTED;
	}
#endif

	RTOS_EnterCriticalSection(saved_state);
	
	if (0 != RTOS_PeekSemaphore(&(queue->SemFilledSlots)))
	{
		result = RTOS_ERROR_FAILED;
	}
	else
	{
		queue->Buffer = 0;
		queue->Size = 0;
		result = RTOS_OK;
	}

	RTOS_ExitCriticalSection(saved_state);

	return result;
}

