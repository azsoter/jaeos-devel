#ifndef RTOS_DEBUG_H
#define RTOS_DEBUG_H
/*
* Copyright (c) Andras Zsoter 2016.
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

extern void rtos_debug_putchar(char c);
extern void rtos_debug_PrintStr(const char *s);
extern void rtos_debug_PrintStrPadded(const char *s, int width);
extern void rtos_debug_PrintHex(uint32_t x, int cr);
// -------------------------------
extern void rtos_Debug(void);
extern void rtos_debug_PrintOS(void);
extern void rtos_debug_PrintAllTasks(void);
extern void rtos_debug_PrintTask(const RTOS_Task *task);

#endif

