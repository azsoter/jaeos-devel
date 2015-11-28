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

# The multiboot header comes from the description at http://wiki.osdev.org/Bare_Bones

# Declare constants used for creating a multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

.globl _bss_start
.globl _bss_end

# https://www.gnu.org/software/grub/manual/multiboot/multiboot.html

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Selectors for the CODE and DATA segments in the GDT.
.set CODE_SEGMENT, 0x10
.set DATA_SEGMENT, 0x18

# The entry point.
.section .text
.global main
.global _start
.type _start, @function
_start:
	cli
	# Set up a stack first.
	movl $_boot_stack_top, %esp

	# We are not supposed to rely on any descriptor tables set up by grub.
	# Create our own.
	call _init_tables
	# --------------------------
	# Wipe out BSS (GRUB might do this already, but better be safe).
	movl	$_bss_start, %edi
	movl	$_bss_end, %ecx
	subl	%edi, %ecx
	shr	$2, %ecx
	xor	%eax, %eax
	cld
	rep	stosl
	# --------------------------
	# Now we can call C code.
	# xchgw	%bx,%bx	# Magic break point for bochs.
	call main

	# If main ever returns just go brain dead -- disable interrupts and halt.
	cli
	hlt
.Lhang:
	jmp .Lhang
.size _start, . - _start

# --------------------------------------------
.global GenericInterruptHandler
.type _interrupt_handler, @function
_interrupt_handler:
	pusha
	call	GenericInterruptHandler
	popa
	iret
.size _interrupt_handler, . - _interrupt_handler
# --------------------------------------------
.type _init_tables, @function
_init_tables:
	lgdt	(_gdtr)
	ljmp	$CODE_SEGMENT, $.Linit0
	nop
.Linit0:
	movw	$DATA_SEGMENT, %ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss
	movw	%ax,%fs
	movw	%ax,%gs
	nop
	movl	$256, %ecx
	// movl	$_interrupt_handler, %eax
	movl	$interrupt_func, %eax	# Debug
	movl	%eax, %ebx
	shr	$16,%ebx
	movl	$_idt, %edi
.LinitLoop0:
	movw	%ax,(%edi)
	movw	%bx, 6(%edi)
	addl	$8, %edi
	addl	$12, %eax	# Debug
	subl	$1,%ecx
	jnz	.LinitLoop0
	nop
	lidt	(_idtr)
	nop
	ret			# This works because we are really in the same stack segment as before.
.size _init_tables, . - _init_tables

.data
# Setting up a Global Descriptor Table, I may be a bit rusty on this.
.align 16
.globl _gdt
_gdt:				# First entry set to 0.
.Lgdt0:
	.quad 0
.Lgdt1:				# Leave this empty.
	.quad 0
#if 1
.Lgdt2:				# 4GB Code Segment.
	.word 0xffff
	.byte 0, 0, 0, 0x9a
	.byte 0xcf
	.byte 0
.Lgdt3:				# 4GB Data Segment.
	.word 0xffff
	.byte 0, 0, 0, 0x92
	.byte 0xcf
	.byte 0
#else
.Lgdt2:				# ??? Code Segment. -- For debugging.
	.word 0x0103
	.byte 0, 0, 0, 0x9a
	.byte 0xc0
	.byte 0
.Lgdt3:				# ??? Data Segment. -- For Debugging.
	.word 0x0111
	.byte 0, 0, 0, 0x92
	.byte 0xc0
	.byte 0
#endif
.size _gdt, . - _gdt
.set _gdt_length, . - _gdt
_gdtr:
	.word _gdt_length - 1
	.long _gdt

.align 16
.global _idt
_idt:
	.rept 256
	.word 0
	.word CODE_SEGMENT
	.byte 0
	.byte 0x8e # 0xee
	.word 0
	.endr
.size _idt, . - _idt
.set _idt_length, . - _idt
_idtr:
	.word _idt_length - 1
	.long _idt
.globl _which_int
_which_int:
	.long 0
.text
.align 16
interrupt_func:
	.rept 256
	movl $(. - interrupt_func) / 12,(_which_int)
	jmp _interrupt_handler
	.endr

# Initial stack.
.section .boot_stack, "aw", @nobits
_boot_stack_bottom:
.skip 16384 # 16K
_boot_stack_top:

