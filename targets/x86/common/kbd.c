/*
* Author: Andras Zsoter
*
* Decoding scan codes coming from a US keyboard.
* It is not a full driver, for illustration purposes only.
* A real keyboard driver needs to deal with a lot of real life situations that is beyond the scope of this example.
*
* Feel free to treat the contents of this particular file as public domain.
*/

#include <stdint.h>
#include "kbd.h"

extern unsigned char us_kbd_map[128][3];

static KBD_Event_t kbd_bits = 0;

static int E0_flag = 0;
 
KBD_Event_t KBD_ProcessScancode(unsigned char scancode)
{
	char code = 0;
	unsigned int select = 0;

	if (0 == (0x80 & scancode))
	{
		if (0 != (kbd_bits & KBD_BIT_CONTROL_MASK))
		{
			select = 2;
		} else if (0 != (kbd_bits & KBD_BIT_SHIFT_MASK))
		{
			select = 1;
		}

		code = us_kbd_map[scancode][select];

		// CAPS LOCK inverts the meaning of shift for letter keys only.
		if (0 != (kbd_bits & KBD_BIT_CAPSLOCK))
		{
			switch(code)
			{
				case 'a'...'z':
					code = us_kbd_map[scancode][1];
					break;
				case 'A'...'Z':
					code = us_kbd_map[scancode][0];
					break;
				default:
					break;
			}
		}
	}

	switch(scancode)
	{
		case 0x2A:	// left shift pressed
			kbd_bits |= KBD_BIT_LEFT_SHIFT;
			break;
		case 0xAA:	// left shift released
			kbd_bits &= ~KBD_BIT_LEFT_SHIFT;
			break;
		case 0x36:	// right shift pressed
			kbd_bits |= KBD_BIT_RIGHT_SHIFT;
			break;
		case 0xB6:	// right shift released
			kbd_bits &= ~KBD_BIT_RIGHT_SHIFT;
			break;

		case 0x1D:	// left CTRL pressed
			kbd_bits |= KBD_BIT_LEFT_CTRL;
			break;
		case 0x9D:	// left CTRL released
			kbd_bits &= ~KBD_BIT_LEFT_CTRL;
			break;

		case 0x38: // ALT pressed
			kbd_bits |= KBD_BIT_ALT;
			break;

		case 0xB8: // ALT released
			kbd_bits &= ~KBD_BIT_ALT;
			break;

		case 0x3a: // CAPS lock pressed
			kbd_bits ^= KBD_BIT_CAPSLOCK;
			break;

		case 0x45: // NUM lock pressed
			kbd_bits ^= KBD_BIT_NUMLOCK;
			break;

		case 0x46: // scroll lock pressed
			kbd_bits ^= KBD_BIT_SCROLLLOCK;
			break;

		default:
			break;
	}

	E0_flag = (0xE0 == scancode);	// Remember E0-s -- i.e. 'extended' code.

	return kbd_bits | (0xFF00 & (((uint32_t)scancode) << 8)) | code;
}

void KBD_Init(void)
{
	E0_flag = 0;
	kbd_bits = 0;
}


