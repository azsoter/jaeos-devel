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
* THIS FILE includes pieces of code that was copied from the ARM initialization
* code for Zynq by Xilinx.
*
* Xilinx's license restrict the use of their code to Xilinx devices,
* read the original Xilinx copyright notice below.
*
* In order to comply with Xilinx's license the use of code contained in THIS FILE
* is restricted to the following uses:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
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

// The purpose of the code in this file is to initialize the second CPU core when running SMP
// on a Zynq device.
// This file includes lines of code that were copied from the ARM initialization code
// provided in the Xilinx SDK.
//
// Xilinx's original copyright notice is displayed below.
// Please note that while Xilinx's license is similar to the MIT license it does contain
// a restriction that ONLY allows the use of the code on or with Xilinx devices.
//
// Please do read the notice below carefully and comply with all the terms in it.

/******************************************************************************
*
* Copyright (C) 2010 - 2014 Xilinx, Inc.  All rights reserved.
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
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* XILINX CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/


// Initialize Secondary CPU(s) for SMP operation.

.globl MMUTable

// ----------------------------------------------
// .globl invalidate_dcache
.globl __cpu_init
.globl exit
.globl rtos_SecondaryCpu
.global rtos_InitCpu
// ----------------------------------------------
.set PSS_L2CC_BASE_ADDR, 0xF8F02000
.set PSS_SLCR_BASE_ADDR, 0xF8000000

.set RESERVED,		0x0fffff00
.set TblBase ,		MMUTable
.set LRemap,		0xFE00000F		/* set the base address of the peripheral block as not shared */
.set L2CCWay,		(PSS_L2CC_BASE_ADDR + 0x077C)	/*(PSS_L2CC_BASE_ADDR + PSS_L2CC_CACHE_INVLD_WAY_OFFSET)*/
.set L2CCSync,		(PSS_L2CC_BASE_ADDR + 0x0730)	/*(PSS_L2CC_BASE_ADDR + PSS_L2CC_CACHE_SYNC_OFFSET)*/
.set L2CCCrtl,		(PSS_L2CC_BASE_ADDR + 0x0100)	/*(PSS_L2CC_BASE_ADDR + PSS_L2CC_CNTRL_OFFSET)*/
.set L2CCAuxCrtl,	(PSS_L2CC_BASE_ADDR + 0x0104)	/*(PSS_L2CC_BASE_ADDR + XPSS_L2CC_AUX_CNTRL_OFFSET)*/
.set L2CCTAGLatReg,	(PSS_L2CC_BASE_ADDR + 0x0108)	/*(PSS_L2CC_BASE_ADDR + XPSS_L2CC_TAG_RAM_CNTRL_OFFSET)*/
.set L2CCDataLatReg,	(PSS_L2CC_BASE_ADDR + 0x010C)	/*(PSS_L2CC_BASE_ADDR + XPSS_L2CC_DATA_RAM_CNTRL_OFFSET)*/
.set L2CCIntClear,	(PSS_L2CC_BASE_ADDR + 0x0220)	/*(PSS_L2CC_BASE_ADDR + XPSS_L2CC_IAR_OFFSET)*/
.set L2CCIntRaw,	(PSS_L2CC_BASE_ADDR + 0x021C)	/*(PSS_L2CC_BASE_ADDR + XPSS_L2CC_ISR_OFFSET)*/

.set SLCRlockReg,	    (PSS_SLCR_BASE_ADDR + 0x04)	/*(PSS_SLCR_BASE_ADDR + XPSS_SLCR_LOCK_OFFSET)*/
.set SLCRUnlockReg,     (PSS_SLCR_BASE_ADDR + 0x08)	/*(PSS_SLCR_BASE_ADDR + XPSS_SLCR_UNLOCK_OFFSET)*/
.set SLCRL2cRamReg,     (PSS_SLCR_BASE_ADDR + 0xA1C) /*(PSS_SLCR_BASE_ADDR + XPSS_SLCR_L2C_RAM_OFFSET)*/

/* workaround for simulation not working when L1 D and I caches,MMU and  L2 cache enabled - DT568997 */
.if SIM_MODE == 1
.set CRValMmuCac,	0b00000000000000	/* Disable IDC, and MMU */
.else
.set CRValMmuCac,	0b01000000000101	/* Enable IDC, and MMU */
.endif

.set CRValHiVectorAddr,	0b10000000000000	/* Set the Vector address to high, 0xFFFF0000 */

.set L2CCAuxControl,	0x72360000		/* Enable all prefetching, Cache replacement policy, Parity enable,
                                        Event monitor bus enable and Way Size (64 KB) */
.set L2CCControl,	0x01			/* Enable L2CC */
.set L2CCTAGLatency,	0x0111			/* latency for TAG RAM */
.set L2CCDataLatency,	0x0121			/* latency for DATA RAM */

.set SLCRlockKey,	        0x767B			/* SLCR lock key */
.set SLCRUnlockKey,	        0xDF0D			/* SLCR unlock key */
.set SLCRL2cRamConfig,      0x00020202      /* SLCR L2C ram configuration */

.set vector_base,	_vector_table

.set FPEXC_EN,		0x40000000		/* FPU enable bit, (1 << 30) */

.section .boot,"ax"


/* this initializes the various processor modes */
rtos_InitCpu:
// OKToRun:
	mrc     p15, 0, r0, c0, c0, 0		/* Get the revision */
	and     r5, r0, #0x00f00000
	and     r6, r0, #0x0000000f
	orr     r6, r6, r5, lsr #20-4

#ifdef CONFIG_ARM_ERRATA_742230
        cmp     r6, #0x22                       /* only present up to r2p2 */
        mrcle   p15, 0, r10, c15, c0, 1         /* read diagnostic register */
        orrle   r10, r10, #1 << 4               /* set bit #4 */
        mcrle   p15, 0, r10, c15, c0, 1         /* write diagnostic register */
#endif

#ifdef CONFIG_ARM_ERRATA_743622
	teq     r5, #0x00200000                 /* only present in r2p* */
	mrceq   p15, 0, r10, c15, c0, 1         /* read diagnostic register */
	orreq   r10, r10, #1 << 6               /* set bit #6 */
	mcreq   p15, 0, r10, c15, c0, 1         /* write diagnostic register */
#endif

	/* set VBAR to the _vector_table address in linker script */
	ldr	r0, =vector_base
	mcr	p15, 0, r0, c12, c0, 0

// --------------------------------------
// --- Snip ---
// --------------------------------------
	/* Write to ACTLR */
	mrc	p15, 0, r0, c1, c0, 1		/* Read ACTLR*/
	orr	r0, r0, #(0x01 << 6)		/* set SMP bit */
	orr	r0, r0, #(0x01 )		/* Set the FW bit (bit 0) ####### Comment from ARM ##### -- enable_maintenance_broadcast() */
	mcr	p15, 0, r0, c1, c0, 1		/* Write ACTLR*/

/* Invalidate caches and TLBs */
	mov	r0,#0				/* r0 = 0  */
	mcr	p15, 0, r0, c8, c7, 0		/* invalidate TLBs */
	mcr	p15, 0, r0, c7, c5, 0		/* invalidate icache */
	mcr	p15, 0, r0, c7, c5, 6		/* Invalidate branch predictor array */
	bl	invalidate_dcache		/* invalidate dcache */
// --------------------------------------
// --- Snip ---
// --------------------------------------
	/* Disable MMU, if enabled */
	mrc	p15, 0, r0, c1, c0, 0		/* read CP15 register 1 */
	bic	r0, r0, #0x1			/* clear bit 0 */
	mcr	p15, 0, r0, c1, c0, 0		/* write value back */

// --------------------------------------
// --- Snip ---
// --------------------------------------
/* Set up stack for CPU1 */
	ldr r4, =.stack1_end
	sub r4, r4, #4
	mrs	r0, cpsr			/* get the current PSR */
	mvn	r1, #0x1f			/* set up the irq stack pointer */
	and	r2, r1, r0
	orr	r2, r2, #0x12		/* IRQ mode */
	msr	cpsr, r2
	mov	r13,r4				/* IRQ stack pointer */
	sub r4, r4, #0x400

	mrs	r0, cpsr			/* get the current PSR */
	mvn	r1, #0x1f			/* set up the supervisor stack pointer */
	and	r2, r1, r0
	orr	r2, r2, #0x13		/* supervisor mode */
	msr	cpsr, r2
	mov	r13,r4				/* Supervisor stack pointer */
	sub r4, r4, #0x800

	mrs	r0, cpsr			/* get the current PSR */
	mvn	r1, #0x1f			/* set up the Abort  stack pointer */
	and	r2, r1, r0
	orr	r2, r2, #0x17		/* Abort mode */
	msr	cpsr, r2
	mov	r13,r4				/* Abort stack pointer */
	sub r4, r4, #0x400

	mrs	r0, cpsr			/* get the current PSR */
	mvn	r1, #0x1f			/* set up the FIQ stack pointer */
	and	r2, r1, r0
	orr	r2, r2, #0x11			/* FIQ mode */
	msr	cpsr, r2
	mov	r13,r4				/* FIQ stack pointer */
	sub r4, r4, #0x400

	mrs	r0, cpsr			/* get the current PSR */
	mvn	r1, #0x1f			/* set up the Undefine stack pointer */
	and	r2, r1, r0
	orr	r2, r2, #0x1b		/* Undefine mode */
	msr	cpsr, r2
	mov	r13,r4				/* Undefine stack pointer */
	sub r4, r4, #0x400

	mrs	r0, cpsr			/* get the current PSR */
	mvn	r1, #0x1f			/* set up the system stack pointer */
	and	r2, r1, r0
	orr	r2, r2, #0x1F		/* SYS mode */
	msr	cpsr, r2
	mov	r13,r4				/* SYS stack pointer */

	/* enable MMU and cache */

	ldr	r0,=TblBase			/* Load MMU translation table base */
	orr	r0, r0, #0x5B			/* Outer-cacheable, WB */
	mcr	15, 0, r0, c2, c0, 0		/* TTB0 */


	mvn	r0,#0				/* Load MMU domains -- all ones=manager */
	mcr	p15,0,r0,c3,c0,0

	/* Enable mmu, icache and dcache */
	ldr	r0,=CRValMmuCac

	mcr	p15,0,r0,c1,c0,0		/* Enable cache and MMU */
	dsb					/* dsb	allow the MMU to start up */

	isb					/* isb	flush prefetch buffer */

// ------------------- snip ----------------

	mov	r0, r0
	mrc	p15, 0, r1, c1, c0, 2		/* read cp access control register (CACR) into r1 */
	orr	r1, r1, #(0xf << 20)		/* enable full access for p10 & p11 */
	mcr	p15, 0, r1, c1, c0, 2		/* write back into CACR */

	/* enable vfp */
	fmrx	r1, FPEXC			/* read the exception register */
	orr	r1,r1, #FPEXC_EN		/* set VFP enable bit, leave the others in orig state */
	fmxr	FPEXC, r1			/* write back the exception register */

	mrc	p15,0,r0,c1,c0,0		/* flow prediction enable */
	orr	r0, r0, #(0x01 << 11)		/* #0x8000 */
	mcr	p15,0,r0,c1,c0,0

	mrc	p15,0,r0,c1,c0,1		/* read Auxiliary Control Register */
	orr	r0, r0, #(0x1 << 2)		/* enable Dside prefetch */
	orr	r0, r0, #(0x1 << 1)		/* enable L2 Prefetch hint */
	mcr	p15,0,r0,c1,c0,1		/* write Auxiliary Control Register */

	mrs	r0, cpsr			/* get the current PSR */
	bic	r0, r0, #0x100			/* enable asynchronous abort exception */
	msr	cpsr_xsf, r0

// Start up.
	bl      __cpu_init		/* Initialize the CPU first (BSP provides this) */
	and	r0, r0, r0			/* no op */
	bl rtos_SecondaryCpu
	bl exit


.Ldone:	b	.Ldone				/* Paranoia: we should never get here */



/*
 *************************************************************************
 *
 * invalidate_dcache - invalidate the entire d-cache by set/way
 *
 * Note: for Cortex-A9, there is no cp instruction for invalidating
 * the whole D-cache. Need to invalidate each line.
 *
 *************************************************************************
 */
invalidate_dcache:
	mrc	p15, 1, r0, c0, c0, 1		/* read CLIDR */
	ands	r3, r0, #0x7000000
	mov	r3, r3, lsr #23			/* cache level value (naturally aligned) */
	beq	finished
	mov	r10, #0				/* start with level 0 */
loop1:
	add	r2, r10, r10, lsr #1		/* work out 3xcachelevel */
	mov	r1, r0, lsr r2			/* bottom 3 bits are the Cache type for this level */
	and	r1, r1, #7			/* get those 3 bits alone */
	cmp	r1, #2
	blt	skip				/* no cache or only instruction cache at this level */
	mcr	p15, 2, r10, c0, c0, 0		/* write the Cache Size selection register */
	isb					/* isb to sync the change to the CacheSizeID reg */
	mrc	p15, 1, r1, c0, c0, 0		/* reads current Cache Size ID register */
	and	r2, r1, #7			/* extract the line length field */
	add	r2, r2, #4			/* add 4 for the line length offset (log2 16 bytes) */
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		/* r4 is the max number on the way size (right aligned) */
	clz	r5, r4				/* r5 is the bit position of the way size increment */
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		/* r7 is the max number of the index size (right aligned) */
loop2:
	mov	r9, r4				/* r9 working copy of the max way size (right aligned) */
loop3:
	orr	r11, r10, r9, lsl r5		/* factor in the way number and cache number into r11 */
	orr	r11, r11, r7, lsl r2		/* factor in the index number */
	mcr	p15, 0, r11, c7, c6, 2		/* invalidate by set/way */
	subs	r9, r9, #1			/* decrement the way number */
	bge	loop3
	subs	r7, r7, #1			/* decrement the index */
	bge	loop2
skip:
	add	r10, r10, #2			/* increment the cache number */
	cmp	r3, r10
	bgt	loop1

finished:
	mov	r10, #0				/* swith back to cache level 0 */
	mcr	p15, 2, r10, c0, c0, 0		/* select current cache level in cssr */
	dsb
	isb

	bx	lr

// -----------------------------------------------------------------------

.section .bss
.align 4
.lcomm .stack1_start, 0x1C00 // Stack space for CPU1 stacks.
.lcomm .stack1_end, 4
.end


