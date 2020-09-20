#ifndef FORMATTED_PRINTING_H
#define FORMATTED_PRINTING_H

/*
* Copyright (c) Andras Zsoter 2020.
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
#include <stdarg.h>

// The bare minimum to format a string (like sprintf() / vsnprintf() does).
// Only a few formatting options are supported: %d, %X (%x acts like %X), %u, %p, %s, %c.
// The flag # works for %X and field with can be specified, but most flags are not supported.
// Also an assumption was made that int is the same as int32_t.
//
extern int minimal_vsnprintf(char *restrict buffer, size_t n, const char *restrict format, va_list vlist);

// The function board_printf() is a wrapper that uses uses a statically allocated buffer.
// Internally the buffer must be locked so that various threads don't stomp on each other's output.
// Do NOT call it from an interrupt service routine.
extern int board_printf(const char *format, ...);

// Thjis is needed to initialize the semaphores guarding test_printf()'s buffer. Call it before the first use of test_printf().
extern int PrintingInit(void);

#endif /* PRINTING_H_ */
