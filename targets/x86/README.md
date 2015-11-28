JaeOS for the x86
=================

Just Another Embedded OS -- X86 Target

JaeOS is aimed at really small embedded systems, so an x86 target might seem odd.
On the other hand one of the most common type of hardware that people have at hand for experimentation is an X86 based PC.
Also most people interested in operating systems have at least seen X86 assembly and can read it to some degree.
So I am providing a JaeOS port for an x86 based PC as an easy to understand example.

The executable is using a protected mode 32bit flat memory model, and it is bootable via a multiboot compatible boot loader such as GRUB.

It really is just an example of more or less the bare minimum needed to run JaeOS on a new CPU/board.
To make it it usable in a practical system a whole lot of device drivers would need to be added.

At this time the x86 target is single CPU only, no SMP support. 

Official Website: http://jaeos.com/

