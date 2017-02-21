/*
* Copyright (c) Andras Zsoter 2014-2017.
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

#include <stdint.h>
#include <board.h>
#include "utility.h"

// Print a number in hex without using any of the libraries.
// In case during OS or hardware bring up none is available on the target system.

static const char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
void PrintHex(uint32_t x)
{
	char buff[11];
	int i;
	buff[10] = '\0';
	buff[8] = '\r';
	buff[9] = '\n';
	for (i = 0; i < 8; i++)
	{
		buff[7-i] = hexDigits[x & 0x0f];
		x >>= 4;
	}
	Board_Puts(buff);
}

void PrintHexNoCr(uint32_t x)
{
	char buff[11];
	int i;
	buff[10] = '\0';
	buff[8] = '\0';
	buff[9] = '\0';
	for (i = 0; i < 8; i++)
	{
		buff[7-i] = hexDigits[x & 0x0f];
		x >>= 4;
	}
	Board_Puts(buff);
}

void PrintUnsignedDecimal(uint32_t x)
{
	char buff[11];
	int i;
	char *p;
	buff[10] = '\0';
	p = &(buff[10]);
	if (0 == x)
	{
		*(--p) = '0';
	}
	else
	{
		while(x)
		{
			*(--p) = hexDigits[x % 10];
			x = x / 10;
		}
	}
	Board_Puts(p);
}

// Print some information about what assert has failed.
// Then just loop in an endless loop.
void app_AssertFailed(const char *condition, const char *file, const char *line, const char *function)
{
	Board_Puts("\r\nAssertion '");
	Board_Puts(condition);
	Board_Puts("' has failed. @");
	Board_Puts(file);
	Board_Putc(':');
	Board_Puts(line);
	Board_Puts(" in function: ");
	Board_Puts(function);
	Board_Puts("().\r\n");
	while(1);
}

