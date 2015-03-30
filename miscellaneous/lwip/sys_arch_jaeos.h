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

#ifndef SYS_ARCH_JAEOS_H
#define SYS_ARCH_JAEOS_H

#ifdef __cplusplus
extern "C" {
#endif
#include "lwipopts.h"

#if defined(OS_IS_JAEOS)

#include "arch/cc.h"
#include <rtos.h>
#include <rtos_queue.h>


#define SYS_THREAD_MAX  MAX_PTHREADS

struct sys_mbox_s
{
	RTOS_Queue q;
	void *slots[];
};

typedef RTOS_Semaphore sys_sem_t;
typedef RTOS_Semaphore sys_mutex_t;

struct sys_mbox_s;
typedef struct sys_mbox_s *sys_mbox_t;

struct sys_thread_s
{
	RTOS_Task task;
	RTOS_StackItem_t stack[(TCPIP_THREAD_STACKSIZE) / sizeof(RTOS_StackItem_t)];
};

typedef struct sys_thread_s *sys_thread_t;

typedef RTOS_Critical_State sys_prot_t;
#define SYS_ARCH_DECL_PROTECT(lev) RTOS_SavedCriticalState(lev)
#define SYS_ARCH_PROTECT(lev) RTOS_EnterCriticalSection(lev)
#define SYS_ARCH_UNPROTECT(lev) RTOS_ExitCriticalSection(lev)

#ifdef __cplusplus
}
#endif

#endif /* OS_IS_JAEOS */
#endif /* SYS_ARCH_JAEOS_H */
