#ifndef RTOS_QUEUE_H
#define RTOS_QUEUE_H
/*
* Copyright (c) Andras Zsoter 2014.
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

#include <rtos.h>

// With the current implementation these must be either the same type
// or QueueCount could be smaller but that almost never makes sense.
typedef RTOS_SemaphoreCount RTOS_QueueCount;

struct RTOS_Queue	// A FIFO (Queue or message queue).
{
	RTOS_Semaphore  Sync;
	RTOS_QueueCount Size;
	RTOS_QueueCount Head;
	RTOS_QueueCount Tail;
	void **Buffer;
};

typedef struct RTOS_Queue RTOS_Queue;

extern RTOS_RegInt RTOS_CreateQueue(RTOS_Queue *queue, void *buffer, RTOS_QueueCount size);
extern RTOS_RegInt RTOS_DestroyQueue(RTOS_Queue *queue);
extern RTOS_RegInt RTOS_Enqueue(RTOS_Queue *queue, void *message);
extern RTOS_RegInt RTOS_Dequeue(RTOS_Queue *queue, void **message, RTOS_Time timeout);
extern void *RTOS_PeekQueue(RTOS_Queue *queue);

#ifdef __cplusplus
}
#endif

#endif

