#ifndef RASPI_PERIPHERALS
#define RASPI_PERIPHERALS
//
// Copyright (c) Andras Zsoter 2014.
//
// Some #define-s to refer to registers in various perripherals in the BCM2835.
//
// The datasheet "BCM2835 ARM Peripherals" from Boardcom should be consulted for a full description
// of how various peripherals in the RaspberryPI work.
//
// Permission is hereby granted to use the code in this particular file as a whole or portions of it without restrictions
// for any project related or unrelated to JaeOS with or without attribution to me.
// If you are working on anything that requires bare metal access on the RaspberryPI or any similar board, please feel free
// to cut and paste code from THIS FILE.
// This permission does not extend to other files that might be distributed or bundled together with this one.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#define ARM_BASE_INTC		0x2000B200
#define ARM_INTC_BASIC		((ARM_BASE_INTC) + 0x00)
#define ARM_INTC_PENDING1	((ARM_BASE_INTC) + 0x04)
#define ARM_INTC_PENDING2	((ARM_BASE_INTC) + 0x08)
#define ARM_INTC_FIQ_CTRL	((ARM_BASE_INTC) + 0x0C)
#define ARM_INTC_ENABLE1	((ARM_BASE_INTC) + 0x10)
#define ARM_INTC_ENABLE2	((ARM_BASE_INTC) + 0x14)
#define ARM_INTC_ENABLE_BASIC	((ARM_BASE_INTC) + 0x18)
#define ARM_INTC_DISABLE1	((ARM_BASE_INTC) + 0x1C)
#define ARM_INTC_DISABLE2	((ARM_BASE_INTC) + 0x20)
#define ARM_INTC_DISABLE_BASIC	((ARM_BASE_INTC) + 0x24)


#define ARM_IRQ_ID_AUX		29
#define ARM_IRQ_ID_SPI_SLAVE 	43
#define ARM_IRQ_ID_PWA0		45
#define ARM_IRQ_ID_PWA1		46
#define ARM_IRQ_ID_SMI 		48
#define ARM_IRQ_ID_GPIO_0 	49
#define ARM_IRQ_ID_GPIO_1 	50
#define ARM_IRQ_ID_GPIO_2 	51
#define ARM_IRQ_ID_GPIO_3 	52
#define ARM_IRQ_ID_I2C 		53
#define ARM_IRQ_ID_SPI 		54
#define ARM_IRQ_ID_PCM 		55
#define ARM_IRQ_ID_UART 	57


#define ARM_IRQ_ID_TIMER_0 	64
#define ARM_IRQ_ID_MAILBOX_0	65
#define ARM_IRQ_ID_DOORBELL_0 	66
#define ARM_IRQ_ID_DOORBELL_1 	67
#define ARM_IRQ_ID_GPU0_HALTED 	68

#define TIMER_BASE	0x2000B400
#define TIMER_LOAD	(TIMER_BASE)
#define TIMER_VALUE	(TIMER_BASE + 4)
#define TIMER_CONTROL	(TIMER_BASE + 8)
#define TIMER_CLEAR	(TIMER_BASE + 12)
#define TIMER_RIRQ	(TIMER_BASE + 16)
#define TIMER_MIRQ	(TIMER_BASE + 20)
#define TIMER_RELOAD	(TIMER_BASE + 24)
#define TIMER_DIVIDER	(TIMER_BASE + 28)
#define TIMER_COUNT	(TIMER_BASE + 32)


#define GPIO_BASE	0x20200000
#define GPPUD		(GPIO_BASE + 0x94)
#define GPPUDCLK0	(GPIO_BASE + 0x98)
 
#define UART0_BASE	0x20201000
 
#define UART0_DR      (UART0_BASE + 0x00)
#define UART0_RSRECR  (UART0_BASE + 0x04)
#define UART0_FR      (UART0_BASE + 0x18)
#define UART0_ILPR    (UART0_BASE + 0x20)
#define UART0_IBRD    (UART0_BASE + 0x24)
#define UART0_FBRD    (UART0_BASE + 0x28)
#define UART0_LCRH    (UART0_BASE + 0x2C)
#define UART0_CR      (UART0_BASE + 0x30)
#define UART0_IFLS    (UART0_BASE + 0x34)
#define UART0_IMSC    (UART0_BASE + 0x38)
#define UART0_RIS     (UART0_BASE + 0x3C)
#define UART0_MIS     (UART0_BASE + 0x40)
#define UART0_ICR     (UART0_BASE + 0x44)
#define UART0_DMACR   (UART0_BASE + 0x48)
#define UART0_ITCR    (UART0_BASE + 0x80)
#define UART0_ITIP    (UART0_BASE + 0x84)
#define UART0_ITOP    (UART0_BASE + 0x88)
#define UART0_TDR     (UART0_BASE + 0x8C)

#endif
