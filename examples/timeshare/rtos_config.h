#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H
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

// Use #undef to disable whichever option is not needed, or just comment it out.
#define RTOS_INCLUDE_SEMAPHORES		/* Counting Semaphores. */ 
#define RTOS_INCLUDE_NAKED_EVENTS	/* Just wait for an event. It is a low level concept, for most purposes semaphores are better. */
#define RTOS_INCLUDE_DELAY		/* Delay -- i.e. pause for a certain amount of time. */
// These are probably not really needed for most applications.
#define RTOS_INCLUDE_SCHEDULER_LOCK
#define RTOS_INCLUDE_WAKEUP 
#define RTOS_INCLUDE_CHANGEPRIORITY

// #define RTOS_USE_TIMER_TASK

#define RTOS_TASK_NAME_LENGTH		32

// TIME SHARE
#define RTOS_SUPPORT_TIMESHARE

#define RTOS_TICKS_PER_SECOND 500
// ---------------------------

// RTOS_Priority_Highest must be defined and it must be equal to the highest priority ever used by the application.
#define RTOS_Priority_LastPeer   6

#if defined(RTOS_USE_TIMER_TASK)
#define RTOS_Priority_Timer	 7
#define RTOS_Priority_Highest   RTOS_Priority_Timer
#else
#define RTOS_Priority_Highest   RTOS_Priority_LastPeer
#endif

// ------------------------------------------ ASSERTS --------------------------------------------------------
extern void app_AssertFailed(const char *condition, const char *file, const char *line, const char *function);
#define STRINGIFY(X) #X
#define STR(X) STRINGIFY(X)
#define RTOS_ASSERT(COND)  (COND) ? (void)0 : app_AssertFailed(#COND, __FILE__, STR(__LINE__), __func__)
// -----------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif

