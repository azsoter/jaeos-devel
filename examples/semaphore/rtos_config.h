#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H
/*
* Copyright (c) Andras Zsoter 2015.
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

#define RTOS_TASK_NAME_LENGTH	32

#define RTOS_TICKS_PER_SECOND 	100

#define RTOS_INCLUDE_SEMAPHORES
#define RTOS_INCLUDE_DELAY

#define RTOS_Priority_Task1	1
#define RTOS_Priority_Task2	2

// RTOS_Priority_Highest must be defined and it must be equal to the highest priority ever used by the application.
#define RTOS_Priority_Highest    RTOS_Priority_Task2

#ifdef __cplusplus
}
#endif

#endif

