/* link-arm-eabi.ld - linker script for arm eabi */
/* This file is based on the linker script example in a tutorial found here:
* http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C
* For licensing of material obtained from that web site please read this: http://wiki.osdev.org/OSDev_Wiki:License
* To spell it out: This file is based on material available on the Internet as Public Domain.
* I hereby release any modifications I have made to this linker script to the Public Domain, please feel free to use it any way you choose.
* Andras Zsoter, 2014.
*/
MEMORY
{
	RAM			(rwx)	: ORIGIN = 0x00008000, LENGTH = 128M
}

ENTRY(_start)
 
SECTIONS
{
    /* Starts at LOADER_ADDR. */
    . = 0x8000;
    _start = .;
    .init : {
	KEEP(*(.init))
     } > RAM 
     . = ALIGN(4); 
    _text_start = .;
    .text :
    {
        KEEP(*(.text.boot))
        *(.text)
    } >RAM
    . = ALIGN(4); /* align to page size */
    _text_end = .;
 
    _rodata_start = .;
    .rodata :
    {
        *(.rodata)
    } >RAM
    . = ALIGN(4); /* align to page size */
    _rodata_end = .;
 
    _data_start = .;
    .data :
    {
        *(.data)
    } >RAM
    . = ALIGN(4); /* align to page size */
    _data_end = .;
 
    __bss_start = .;
    .bss :
    {
        bss = .;
        *(.bss)
	*(.bss.*)
    }
    . = ALIGN(4); /* align to page size */
    __bss_end = .;
    _end = .;
}

