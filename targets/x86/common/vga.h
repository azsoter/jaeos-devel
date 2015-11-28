#ifndef VGA_H
#define VGA_H
/*
* Author: Andras Zsoter
*
* Some rudimentary routines to display something in VGA text mode.
*
* Feel free to treat the contents of this particular file as public domain.
*/

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

extern void VGA_Initialize(void);
extern void VGA_SetColor(unsigned char color);
extern void VGA_PutChar(char c);
extern void VGA_Puts(const char* data);
extern void VGA_PrintAt(char c, uint8_t color, uint32_t x, uint32_t y);
extern void VGA_Cls(void);

#endif

