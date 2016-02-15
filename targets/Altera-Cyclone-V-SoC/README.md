JaeOS for the Altera Cyclone-V SoC
==================================

Just Another Embedded OS -- Altera Cyclone-V SoC Target

Just another FPGA with a hard core ARM to run JaeOS on.

The ARM part is rather similar to the Xilinx Zynq (e.g. the ZedBoard target).

This is really just a basic port to run on the Altera hardware.

It was tested on an Atlas SoC demo board, but almost any Cyclone-V board would do.
https://www.altera.com/products/boards_and_kits/dev-kits/partners/cyclonev_soc_atlas.html

You need to install the software develoment tools for Altera to compile this target, but it does compile from the command line.
Check out the Makefile for further details.

You may run the resulting .axf file from the debugger from Altera/ARM (you need to run a preloader first, thre image is compiled for SDRAM).
Alternatively you can use the .img file instead of the second stage bootloader (u-boot.img) to boot from an SD card.

At this time this is single CPU only, no SMP support. 

Official Website: http://jaeos.com/

