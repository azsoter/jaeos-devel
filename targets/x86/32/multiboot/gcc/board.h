#ifndef BOARD_H
#define BOARD_H
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

extern void Board_Putc(char c);
extern void Board_Puts(const char *s);
extern char Board_Getc(void);

extern void board_HandleIRQ(void);

extern int Board_HardwareInit(void);

// The X86 has separate instruction to access I/O ports (they are typically not memory mapped).
// Here are some functions to access them.

static __inline__ unsigned char inb(unsigned short int port)
{
  unsigned char value;

  __asm__ volatile ("inb %w1,%0":"=a" (value):"Nd" (port));
  return value;
}


static __inline__ void outb (unsigned char value, unsigned short int port)
{
  __asm__ volatile ("outb %b0,%w1": :"a" (value), "Nd" (port));
}

#endif
