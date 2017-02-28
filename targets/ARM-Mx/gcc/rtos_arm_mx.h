/*
* Copyright (c) Andras Zsoter 2016-2017.
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


#ifndef RTOS_ARM_MX_H
#define RTOS_ARM_MX_H

// -----------------------------------------------------------------------------------------------------------
// This section depends on the chip/board used.
//
// I own Nucleo devel boards with he following STM32 chips, you may need to add to the list if you have a
// different board, or replace this section altogether.
#if defined (STM32L476xx)
#include <stm32l4xx_hal.h>
#elif defined(STM32F401xE)
#include <stm32f4xx_hal.h>
#elif defined(STM32L152xE)
#include <stm32l1xx_hal.h>
#elif
#error Unknown Microcontroller/board.
#endif

#if defined (STM32L476xx) || defined(STM32F401xE) || defined(STM32L152xE)
// May require redefinition depending on how you can retrieve clock frequency ion your set-up.
// This shoudl be appropriate for the STM32.
#define SYSTEM_CLOCK_FREQUENCY HAL_RCC_GetHCLKFreq()

#define ARM_M3_INTERRUPT_PRIORITY_bitcount (__NVIC_PRIO_BITS)

// On the Nucleo board we can share Systick between HAL and the RTOS, but the handler needs to take care of that.
#undef RTOS_USE_DEFAULT_ARM_Mx_SYSTICK_HANDLER
#else
#error Need to specify an MCU/board.
#endif
// -----------------------------------------------------------------------------------------------------------

#define ARM_M3_LOWEST_INTERRUPT_PRIORITY_bits (0xFF & (0xFF << ARM_M3_INTERRUPT_PRIORITY_bitcount))

#define ARM_M3_INTERRUPT_PRIORITY_TO_bits(PRIO) ((PRIO) << (8 - ARM_M3_INTERRUPT_PRIORITY_bitcount))


// Interrupt control status register.
#define ARM_M3_REG_ICSR 0xE000ED04
#define ARM_M3_REG_ICSR_bit_PENDV	0x10000000

// SysTick.
#define ARM_M3_REG_STCSR 0xE000E010 // SysTick Control and Status Register.
#define ARM_M3_REG_STCSR_bit_ENABLE		0x00000001
#define ARM_M3_REG_STCSR_bit_TICKINT	0x00000002
#define ARM_M3_REG_STCSR_bit_CLKSOURCE	0x00000004
#define ARM_M3_REG_STCSR_bit_COUNTFLAG	0x00010000

#define ARM_M3_REG_STRVR 0xE000E014 // SysTick Reload Value Register.
#define ARM_M3_REG_STCVR 0xE000E018 // SysTick Current Value Register.
#define ARM_M3_REG_STCR  0xE000E01C  // SysTick Calibration Value Register.

#define ARM_M3_REG_SHPR1 0xE000ED18 // System Handler Priority Register 1
#define ARM_M3_REG_SHPR2 0xE000ED1C // System Handler Priority Register 2
#define ARM_M3_REG_SHPR3 0xE000ED20 // System Handler Priority Register 3

#define ARM_M3_REG_IPR0 0xE000E400	// Interrupt priority register 0.

#define ARM_M3_SET_REG(REG, VALUE) *((volatile uint32_t *)(REG)) = (VALUE)

#define ARM_M3_PSR_ISR_NUMBER_mask 0x01FF

#endif /* RTOS_ARM_MX_H */
