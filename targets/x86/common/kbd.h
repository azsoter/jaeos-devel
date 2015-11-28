#ifndef KBD_H
#define KBD_H
/*
* Author: Andras Zsoter
*
* Decoding scan codes coming from a US keyboard.
* It is not a full driver, for illustration purposes only.
* A real keyboard driver needs to deal with a lot of real life situations that is beyond the scope of this example.
*
* Feel free to treat the contents of this particular file as public domain.
*/

typedef uint32_t KBD_Event_t;
 
#define KBD_BIT_CAPSLOCK	0x80000000
#define KBD_BIT_NUMLOCK		0x40000000
#define KBD_BIT_SCROLLLOCK	0x20000000

#define KBD_BIT_ALT		0x10000000

#define KBD_BIT_LEFT_SHIFT	0x01000000
#define KBD_BIT_RIGHT_SHIFT	0x02000000
#define KBD_BIT_SHIFT_MASK	0x03000000

#define KBD_BIT_LEFT_CTRL	0x04000000
#define KBD_BIT_RIGHT_CTRL	0x08000000
#define KBD_BIT_CONTROL_MASK	0x0C000000

extern KBD_Event_t KBD_ProcessScancode(unsigned char scancode);
// extern KBD_Event_t KBD_handler(void);
extern void KBD_Init(void);
#endif
