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

// A somewhat quick and dirty implementation of formatting (ala printf()).
// For now we assume that int is int32_t.

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#undef PC_TESTING

#if defined(PC_TESTING)
#define POINTERS_64BIT
#else
#include <rtos.h>
#include <board.h>
#include <formatted_printing.h>
#endif

static void append_char(char *restrict buffer, size_t n, size_t *count, char c)
{
	if (*count < (n - 1))
	{
		buffer[*count] = c;
	}
	(*count)++;
	// buffer[*count] = '\0';
}

static void append_string_with_padding(char *restrict buffer, size_t n, size_t *pcount, const char *string, int width, int padchar, int padright)
{
	if (width > 0)
	{
		int len = 0;
		const char *ptr;
		for (ptr = string; *ptr; ptr++)
		{
			len++;
		}
		if (len >= width)
		{
			width = 0;
		}
		else
		{
			width -= len;
		}
	}

	if (!padright)
	{
		for (; width > 0; width--)
		{
			append_char(buffer, n, pcount, padchar);
		}
	}

	for ( ; *string ; ++string)
	{
		append_char(buffer, n, pcount, *string);
	}

	for (; width > 0; --width)
	{
		append_char(buffer, n, pcount, padchar);
	}
}

/* the following should be enough for 32 bit int */
static const char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
#define NUM_BUF_LEN 12
static char *format_uint(char *buffer, uint32_t u, unsigned int base)
{
	unsigned int digit;
	char *p = buffer + NUM_BUF_LEN;

	*--p = '\0';

	do
	{
		digit = (u % base);
		u = u / base;
		*--p = table[digit];
	} while (0 != u);

	return p;	
}

int minimal_vsnprintf(char *restrict buffer, size_t n, const char *restrict format, va_list vlist)
{
	int width;
	char padding;
	int padright;
	int alternate = 0;
	size_t count = 0;
	int32_t tmp;
	char *stmp;
	void *v;
	char num_buffer[NUM_BUF_LEN];

	*buffer = '\0';
	for (; '\0' != *format; format++)
	{
		width = 0;
		padding = ' ';
		padright = 0;

		if ('%' == *format)
		{
			format++;

			if ('\0' == *format) break;
			if ('%' == *format)
			{
				append_char(buffer, n, &count, '%');
				continue;
			}

			if ('#' == *format)
			{
				alternate = 1;
				format++;
			}

			if ('-' == *format)
			{
				format++;
				padright = 1;
			}

			while (*format == '0')
			{
				format++;
				padding = '0';
			}

			for (; *format >= '0' && *format <= '9'; format++)
			{
				width *= 10;
				width += (*format) - '0'; // ASCII
			}

			switch(*format)
			{
				case 'd':
					tmp = va_arg(vlist, int);
					if (0 > tmp)
					{
						stmp = format_uint(num_buffer, (uint32_t)(-tmp), 10);
						if ((!padright) && (width > 0) && ('0' == padding))
						{
							append_char(buffer, n, &count, '-');
							append_string_with_padding(buffer, n, &count, stmp, width - 1, padding, padright);
						}
						else
						{
							*--stmp = '-';
							append_string_with_padding(buffer, n, &count, stmp, width, padding, padright);
						}
					}
					else
					{
						stmp = format_uint(num_buffer, (uint32_t)tmp, 10);
						append_string_with_padding(buffer, n, &count, stmp, width, padding, padright);
					}
				break;

				//case 'i':
				//tmp = va_arg(vlist, int);
				//break;
				
				case 'u':
					tmp = va_arg(vlist, int);
					stmp = format_uint(num_buffer, (uint32_t)tmp, 10);
					append_string_with_padding(buffer, n, &count, stmp, width, padding, padright);
				break;
				
				case 'x':
					if (alternate)
					{
						append_char(buffer, n, &count, '0');
						append_char(buffer, n, &count, 'x');
						width -= 2;
					}
					tmp = va_arg(vlist, int);
					stmp = format_uint(num_buffer, (uint32_t)tmp, 16);
					append_string_with_padding(buffer, n, &count, stmp, width, padding, padright);
				break;

				case 'X':
					if (alternate)
					{
						append_char(buffer, n, &count, '0');
						append_char(buffer, n, &count, 'X');
						width -= 2;
					}
					tmp = va_arg(vlist, int);
					stmp = format_uint(num_buffer, (uint32_t)tmp, 16);
					append_string_with_padding(buffer, n, &count, stmp, width, padding, padright);
				break;

				case 'p':
					append_char(buffer, n, &count, '0');
					append_char(buffer, n, &count, 'X');
					v = va_arg(vlist, void *);
#if defined(POINTERS_64BIT)
					// This code is really for testing on 64bit systems such as a PC.
					stmp = format_uint(num_buffer, (uint32_t)(((uint64_t)v) >> 32), 16);
					append_string_with_padding(buffer, n, &count, stmp, 8, '0', 0);
					tmp = (int32_t)(0xFFFFFFFF & ((uint64_t)v));
#else
					tmp = (int32_t)v;
#endif
					stmp = format_uint(num_buffer, (uint32_t)tmp, 16);
					append_string_with_padding(buffer, n, &count, stmp, 8, '0', 0);
					break;

				case 'c':
					tmp = va_arg(vlist, int);
					append_char(buffer, n, &count, (char)tmp);
				break;

				case 's':
					stmp = va_arg(vlist, char*);
					append_string_with_padding(buffer, n, &count, stmp, width, padding, padright);
				break;
			}
		}
		else
		{
			append_char(buffer, n, &count, *format);
		}

	}

	if (count < n)
	{
		buffer[count] = '\0';
	}
	else
	{
		buffer[n - 1] = '\0';
	}

	return (int)count;
}

#if defined(PC_TESTING)
#include <stdio.h>
char buffer[256];
int board_printf(const char *format, ...)
{
	va_list ap;
	int res;
	va_start(ap, format);
	res = minimal_vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);
	puts(buffer);
	return res;
}

int main()
{
	printf("X = %05d %#X %c %d\n",2, 4, '!', -25);
	board_printf("X = %05d %#X %c %d\n",2, 4, '!', -25);

	printf("X = %05d %#010X\n",2, 4);
	board_printf("X = %05d %#010X\n",2, 4);

	printf("X = %05d %#08X %s\n",-2, -4, "Hello World.");
	board_printf("X = %05d %#08X %s\n",-2, -4, "Hello World.");

	printf("main = %p 2 = %d %u\n", main, 2, -10);
	board_printf("main = %p 2 = %d %u\n", main, 2, -10);
	return 0;
}
#else
static char buffer[256];
RTOS_Semaphore printBufferLock;
int board_printf(const char *format, ...)
{
	va_list ap;
	int res;
	RTOS_GetSemaphore(&printBufferLock, RTOS_TIMEOUT_FOREVER);
	va_start(ap, format);
	res = minimal_vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);
	Board_Puts(buffer);
	RTOS_PostSemaphore(&printBufferLock);
	return res;
}

int PrintingInit(void)
{
	return RTOS_CreateSemaphore(&printBufferLock, 1);
}
#endif

