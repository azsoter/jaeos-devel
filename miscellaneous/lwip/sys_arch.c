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

#include "lwipopts.h"
#ifdef OS_IS_JAEOS

#include "arch/sys_arch.h"

/* ------------------------ lwIP includes --------------------------------- */
#include "lwip/opt.h"

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

// Should define this to be something more useful.
#define DEBUG_PRINTF(FORMAT, ...) 

#undef DEBUG

/*
 * Prints an assertion messages and aborts execution.
 */
void sys_assert( const char *message )
{
	(void) message;

	DEBUG_PRINTF("sys_assert(%s)\r\n", message);

	for (;;)
	{
	}
}

u32_t sys_now(void)
{
	return (u32_t)(RTOS_TICKS_TO_MS(RTOS_GetTime()));
}

/** Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return a new mutex */
err_t sys_mutex_new( sys_mutex_t *pxMutex )
{
	if (0 == pxMutex)
	{
		return ERR_MEM;
	}

	pxMutex->Count = 1;
	return ERR_OK;
}

/** Lock a mutex
 * @param mutex the mutex to lock */
void sys_mutex_lock(sys_mutex_t *pxMutex)
{
	RTOS_GetSemaphore(pxMutex, RTOS_TIMEOUT_FOREVER);
}

/** Unlock a mutex
 * @param mutex the mutex to unlock */
void sys_mutex_unlock(sys_mutex_t *pxMutex )
{
	RTOS_PostSemaphore(pxMutex);
}

/*---------------------------------------------------------------------------*
 * Routine:  sys_sem_new
 *---------------------------------------------------------------------------*
 * Description:
 *      Creates and returns a new semaphore. The "ucCount" argument specifies
 *      the initial state of the semaphore.
 *      NOTE: Currently this routine only creates counts of 1 or 0
 * Inputs:
 *      sys_mbox_t mbox         -- Handle of mailbox
 *      u8_t ucCount              -- Initial ucCount of semaphore (1 or 0)
 * Outputs:
 *      sys_sem_t               -- Created semaphore or 0 if could not create.
 *---------------------------------------------------------------------------*/
err_t sys_sem_new( sys_sem_t *pxSemaphore, u8_t ucCount )
{
	if (0 == pxSemaphore)
	{
		return ERR_MEM;
	}

	pxSemaphore->Count = ucCount;
	return ERR_OK;
}

/*---------------------------------------------------------------------------*
 * Routine:  sys_sem_signal
 *---------------------------------------------------------------------------*
 * Description:
 *      Signals (releases) a semaphore
 * Inputs:
 *      sys_sem_t sem           -- Semaphore to signal
 *---------------------------------------------------------------------------*/
void sys_sem_signal(sys_sem_t *pxSemaphore)
{
	RTOS_PostSemaphore(pxSemaphore);
}

/*---------------------------------------------------------------------------*
 * Routine:  sys_arch_sem_wait
 *---------------------------------------------------------------------------*
 * Description:
 *      Blocks the thread while waiting for the semaphore to be
 *      signaled. If the "timeout" argument is non-zero, the thread should
 *      only be blocked for the specified time (measured in
 *      milliseconds).
 *
 *      If the timeout argument is non-zero, the return value is the number of
 *      milliseconds spent waiting for the semaphore to be signaled. If the
 *      semaphore wasn't signaled within the specified time, the return value is
 *      SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
 *      (i.e., it was already signaled), the function may return zero.
 *
 *      Notice that lwIP implements a function with a similar name,
 *      sys_sem_wait(), that uses the sys_arch_sem_wait() function.
 * Inputs:
 *      sys_sem_t sem           -- Semaphore to wait on
 *      u32_t timeout           -- Number of milliseconds until timeout
 * Outputs:
 *      u32_t                   -- Time elapsed or SYS_ARCH_TIMEOUT.
 *---------------------------------------------------------------------------*/
u32_t sys_arch_sem_wait( sys_sem_t *pxSemaphore, u32_t ulTimeout )
{
	RTOS_Time timeout = (0 == ulTimeout) ? RTOS_TIMEOUT_FOREVER : RTOS_MS_TO_TICKS(ulTimeout);
	RTOS_Time start = RTOS_GetTime();
	int res;

	res = RTOS_GetSemaphore(pxSemaphore, timeout);

	if (RTOS_OK == res)
	{
		if (0 == ulTimeout)
		{
			return 0;
		}
		return RTOS_TICKS_TO_MS((u32_t)(RTOS_GetTime() - start));
	}
	return SYS_ARCH_TIMEOUT;
}


int sys_sem_valid(sys_sem_t *sem)
{
	return (0 != sem);
}

void sys_sem_set_invalid(sys_sem_t *sem)
{

}

void sys_sem_free(sys_sem_t *sem)
{
}

// ==================================================================================
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	RTOS_RegInt res;
	void *p;
	RTOS_Queue *q;

	p = mem_malloc(sizeof(struct sys_mbox_s) + sizeof(void *) * size);
	if (0 == p)
	{
		return ERR_MEM;
	}
	q = &((sys_mbox_t)p)->q;
	memset(q, 0, sizeof(RTOS_Queue));
	res = RTOS_CreateQueue(q, ((sys_mbox_t)p)->slots, (unsigned int)size);
#if defined(DEBUG_MBOX)
	DEBUG_PRINTF("RTOS_InitializeQueue(%x, %d)=%d\r\n", mbox, size, res);
#endif
	// Need to deal with failed initialization???
	*mbox = (sys_mbox_t)p;
	return ERR_OK;
}

void sys_mbox_free (sys_mbox_t *mbox)
{
	if (0 != mbox)
	{
		if (0 != *mbox)
		{
			mem_free(*mbox);
			*mbox = 0;
		}
	}
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	RTOS_RegInt res;
	RTOS_Queue *q = &((*mbox)->q);

#if defined(DEBUG_MBOX)
	DEBUG_PRINTF("Enqueue(%x, %x)\r\n", mbox, msg);
#endif
	while (1)
	{
		res = RTOS_Enqueue(q, msg, RTOS_TIMEOUT_FOREVER);
		if (RTOS_OK == res)
		{
			break;
		}
		// outbyte('<');
		RTOS_Delay(1);

	}
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	RTOS_RegInt res;
	RTOS_Queue *q = &((*mbox)->q);

	res = RTOS_Enqueue(q, msg, 0);
#if defined(DEBUG_MBOX)
	DEBUG_PRINTF("Enqueue no wait(%x, %x)=%d\r\n", mbox, msg, res);
#endif
	if (RTOS_OK == res)
	{
		return ERR_OK;
	}

	return ERR_MEM;
}

/*---------------------------------------------------------------------------*
 * Routine:  sys_arch_mbox_fetch
 *---------------------------------------------------------------------------*
 * Description:
 *      Blocks the thread until a message arrives in the mailbox, but does
 *      not block the thread longer than "timeout" milliseconds (similar to
 *      the sys_arch_sem_wait() function). The "msg" argument is a result
 *      parameter that is set by the function (i.e., by doing "*msg =
 *      ptr"). The "msg" parameter maybe NULL to indicate that the message
 *      should be dropped.
 *
 *      The return values are the same as for the sys_arch_sem_wait() function:
 *      Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
 *      timeout.
 *
 *      Note that a function with a similar name, sys_mbox_fetch(), is
 *      implemented by lwIP.
 * Inputs:
 *      sys_mbox_t mbox         -- Handle of mailbox
 *      void **msg              -- Pointer to pointer to msg received
 *      u32_t timeout           -- Number of milliseconds until timeout
 * Outputs:
 *      u32_t                   -- SYS_ARCH_TIMEOUT if timeout, else number
 *                                  of milliseconds until received.
 *---------------------------------------------------------------------------*/
u32_t sys_arch_mbox_fetch_internal(sys_mbox_t *mbox, void **msg, RTOS_Time timeout)
{
	RTOS_Time start_ticks = 0;
	RTOS_Time stop_ticks = 0;
	RTOS_Time ticks = 0;
	RTOS_RegInt res;
	RTOS_Queue *q = &((*mbox)->q);
	void *dummy;
	void **ptr;

	ptr = (0 == msg) ? &dummy : msg;
	*ptr = 0;
	// outbyte('>');
	start_ticks = RTOS_GetTime();

	res = RTOS_Dequeue(q, ptr, timeout); // msg

	stop_ticks = RTOS_GetTime();

	ticks = stop_ticks - start_ticks;
#if defined(DEBUG_MBOX)
	DEBUG_PRINTF("Dequeue(%x,,%d)=%d %x\r\n", mbox, res, *msg, timeout);
#endif
	if (RTOS_OK != res)
	{
		return SYS_ARCH_TIMEOUT;
	}

	return (u32_t)(RTOS_TICKS_TO_MS(ticks));
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	RTOS_Time timeOut = (0 == timeout) ? (RTOS_TIMEOUT_FOREVER) : RTOS_MS_TO_TICKS(timeout);
	return sys_arch_mbox_fetch_internal(mbox, msg, timeOut);
}

/*---------------------------------------------------------------------------*
 * Routine:  sys_arch_mbox_tryfetch
 *---------------------------------------------------------------------------*
 * Description:
 *      Similar to sys_arch_mbox_fetch, but if message is not ready
 *      immediately, we'll return with SYS_MBOX_EMPTY.  On success, 0 is
 *      returned.
 * Inputs:
 *      sys_mbox_t mbox         -- Handle of mailbox
 *      void **msg              -- Pointer to pointer to msg received
 * Outputs:
 *      u32_t                   -- SYS_MBOX_EMPTY if no messages.  Otherwise,
 *                                  return ERR_OK.
 *---------------------------------------------------------------------------*/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	return sys_arch_mbox_fetch_internal(mbox, msg, 0);
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
	return ((0 != mbox) && (0 != *mbox));
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	*mbox = 0;
}
// ==================================================================================
/*---------------------------------------------------------------------------*
 * Routine:  sys_thread_new
 *---------------------------------------------------------------------------*
 * Description:
 *      Starts a new thread with priority "prio" that will begin its
 *      execution in the function "thread()". The "arg" argument will be
 *      passed as an argument to the thread() function. The id of the new
 *      thread is returned. Both the id and the priority are system
 *      dependent.
 * Inputs:
 *      char *name              -- Name of thread
 *      void (* thread)(void *arg) -- Pointer to function to run.
 *      void *arg               -- Argument passed into function
 *      int stacksize           -- Required stack amount in bytes
 *      int prio                -- Thread priority
 * Outputs:
 *      sys_thread_t            -- Pointer to per-thread timeouts.
 *---------------------------------------------------------------------------*/
struct sys_thread_s sys_task_array[(RTOS_Priority_LastManagedTask) - (RTOS_Priority_FirstManagedTask) + 1];

sys_thread_t sys_thread_new(const char *name, void(*f)(void *), void *param, int stackSize, int priority )
{
	RTOS_TaskPriority i;
	RTOS_RegInt res;
	RTOS_Task *task = 0;
	sys_thread_t sthread = 0;

	RTOS_SavedCriticalState(saved_state);

	if ((TCPIP_THREAD_STACKSIZE) < stackSize)
	{
		DEBUG_PRINTF("Cannot create thread, not enough stack space.\r\n");
		return 0;
	}

	if (DEFAULT_THREAD_PRIO != priority)
	{
		if ((RTOS_Priority_LastManagedTask < priority) || (RTOS_Priority_FirstManagedTask > priority))
		{
			DEBUG_PRINTF("Cannot create thread, Invalid priority(%d)\r\n", priority);
			return 0;
		}

		RTOS_EnterCriticalSection(saved_state);
		sthread = &(sys_task_array[(priority - (RTOS_Priority_FirstManagedTask))]);
		task = &(sthread->task);
		memset(task, 0, sizeof(RTOS_Task));
		res = RTOS_CreateTask(task, name, priority, sthread->stack, ((TCPIP_THREAD_STACKSIZE)/sizeof(RTOS_StackItem_t)), f, param);
		RTOS_ExitCriticalSection(saved_state);

		if (RTOS_OK == res)
		{
			return sthread;
		}

		DEBUG_PRINTF("Failed to create thread RTOS_CreateTask() has returned %d\r\n", res);

		return 0;
	}

	for (i = (RTOS_Priority_FirstManagedTask); i <= (RTOS_Priority_LastManagedTask); i++)
	{
		RTOS_EnterCriticalSection(saved_state);

		if (0 == RTOS.TaskList[i])	// Need to change this?
		{
			sthread = &(sys_task_array[(i - (RTOS_Priority_FirstManagedTask))]);
			task = &(sthread->task);
			memset(task, 0, sizeof(RTOS_Task));
			res = RTOS_CreateTimeShareTask( task, name, i, sthread->stack, ((TCPIP_THREAD_STACKSIZE)/sizeof(RTOS_StackItem_t)), f, param, 
					                      RTOS_DEFAULT_TIME_SLICE
							);
			if (RTOS_OK != res)
			{
				sthread = 0;
				task = 0;
				continue;
			}
		}

		RTOS_ExitCriticalSection(saved_state);

		if (0 != sthread)
		{
			return sthread;
		}
	}
	return 0;
}

// -------------------------------------------------------
void sys_init(void)
{

}

#endif


