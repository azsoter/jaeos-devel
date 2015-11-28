/* Entry point, GRUB will recognize it. */
ENTRY(_start)

SECTIONS
{
	/* GRUB booted kernels typically start at 1 MB. */
	. = 1M;

	/* .text -- i.e. code section. */
	/* Place the multiboot header first so that GRUB can find it easily. */
	.text BLOCK(4K) : ALIGN(4K)
	{
		_text_start = .;
		*(.multiboot)
		*(.text)
		*(.text.*)
		. = ALIGN(16);
		_text_end = .;

	}

	/* Read-only data. */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Read-write data. */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Uninitialized data. */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		_bss_start = .;
		*(COMMON)
		*(.bss)
		*(.boot_stack)
		. = ALIGN(16);
		_bss_end = .;
	}
}

