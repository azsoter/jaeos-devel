/*
* Author: Andras Zsoter
*
* Some rudimentary routines to display something in VGA text mode.
*
* Feel free to treat the contents of this particular file as public domain.
*/

#include <stddef.h>
#include <stdint.h>
#include "vga.h"
#define VGA_WIDTH  80
#define VGA_HEIGHT  25
 
static uint32_t row;
static uint32_t column;
static uint8_t color;
static uint16_t* const buffer = (uint16_t *)0xB8000;
// static uint16_t* buffer;

static uint8_t VGA_CombineFgBg(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}
 
static uint16_t VGA_ColoredChar(char c, unsigned char color)
{
	return (uint16_t)c | (((uint16_t)color) << 8);
}

 
void VGA_Initialize(void)
{
	row = 0;
	column = 0;
	uint32_t ix;
	uint32_t blank;
	uint32_t x;
	uint32_t y;
	// buffer = (uint16_t*) 0xB8000;
	color = VGA_CombineFgBg(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	blank = VGA_ColoredChar(' ', color);
	for (y = 0; y < (VGA_HEIGHT); y++)
	{
		ix = y * (VGA_WIDTH);

		for (x = 0; x < (VGA_WIDTH); x++)
		{
			buffer[ix + x] = blank;
		}
	}
}

void VGA_Cls(void)
{
	VGA_Initialize();
}
 
void VGA_Setcolor(uint8_t color)
{
	color = color;
}
 
void VGA_PrintAt(char c, uint8_t color, uint32_t x, uint32_t y)
{
	buffer[y * (VGA_WIDTH) + x] = VGA_ColoredChar(c, color);
}

void VGA_Linefeed(void)
{
	__asm__ volatile ("movl $0xB8000, %edi");
	__asm__ volatile ("movl $0xB80A0, %esi");
	__asm__ volatile ("movl $0x0C30,  %ecx");
	__asm__ volatile ("cld");
	__asm__ volatile ("rep movsw");
}
 
void VGA_PutChar(char c)
{

	switch(c)
	{
		case '\r':
			column = 0;
			break;
		case '\n':
			if ((row + 1) == (VGA_HEIGHT))
			{
				VGA_Linefeed();
			}
			else
			{
				row++;
			}
			break;
		default:
			VGA_PrintAt(c, color, column, row);
			if (++column == VGA_WIDTH)
			{
				column = 0;
				if ((row + 1) == (VGA_HEIGHT))
				{
					VGA_Linefeed();
				}
				else
				{
					row++;
				}

			}
	}
}
 
void VGA_Puts(const char* data)
{
	char c;
	if (0 != data)
	{
		while (1)
		{
			c = *data++;
			if (0 == c) break;
			VGA_PutChar(c);
		}
	}
}

