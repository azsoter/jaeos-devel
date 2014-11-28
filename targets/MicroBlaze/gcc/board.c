/*
* Copyright (c) Andras Zsoter 2014.
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

#include <stdint.h>
#include <rtos.h>
#include <rtos_internals.h>
#include <board.h>
#include <xparameters.h>
#include <xtmrctr.h>
#include <xintc.h>
#include <xintc_i.h>
#include <xintc_l.h>
#include <xtmrctr.h>
#include <platform.h>

#define INTC_DEVICE_ID     	XPAR_INTC_0_DEVICE_ID
#define INTC_TIMER_ID     	XPAR_INTC_0_TMRCTR_0_VEC_ID
#define INTC_ADDR 			XPAR_INTC_0_BASEADDR

XIntc    InterruptController;
XTmrCtr TimerInstance;

void  board_TimerHandler(void *addr)
{
    uint32_t  csr = XTmrCtr_GetControlStatusReg(TimerInstance.BaseAddress, 0);
    if (csr & XTC_CSR_INT_OCCURED_MASK)
    {
        XTmrCtr_SetControlStatusReg(TimerInstance.BaseAddress, 0, csr);
        rtos_TimerTick();
    }
    (void)addr;
}

int board_TimerInit(void)
{
	int status;
	status = XTmrCtr_Initialize(&TimerInstance, XPAR_AXI_TIMER_0_DEVICE_ID);
	if (XST_SUCCESS != status)
	{
			return -1;
	}
	XTmrCtr_SetOptions(&TimerInstance, 0, XTC_DOWN_COUNT_OPTION | XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION );
	XTmrCtr_SetResetValue(&TimerInstance, 0, ((XPAR_CPU_CORE_CLOCK_FREQ_HZ) / (RTOS_TICKS_PER_SECOND)));
	XTmrCtr_Start(&TimerInstance, 0 );
	return 0;
}

int board_InterruptInit(void)
{
    XStatus  status = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);

    if (XST_SUCCESS != status)
    {
    		return -1;
    }

    status = XIntc_Connect(&InterruptController, INTC_TIMER_ID, board_TimerHandler, (void *)0);

    if (XST_SUCCESS != status)
    {
    		return -1;
    }

    XIntc_Enable(&InterruptController, INTC_TIMER_ID);

    status = XIntc_Start(&InterruptController, XIN_REAL_MODE);

    if (XST_SUCCESS != status)
    {
     		return -1;
    }

    return 0;
}

void board_HandleIRQ(void)
{
    uint32_t					intrStatus;
    uint32_t                 	mask;
    uint32_t                 	whichIntr;
    XIntc_Config           		*cfgPtr;
    XIntc_VectorTableEntry 		*tableEntry;

    cfgPtr = &XIntc_ConfigTable[0];
    intrStatus = XIntc_GetIntrStatus(INTC_ADDR);

    while (0 != intrStatus)
    {
        whichIntr = *(uint32_t *)((INTC_ADDR) + (XIN_IVR_OFFSET));

        mask   = 1 << whichIntr;

        if (0 != ((cfgPtr->AckBeforeService) & mask))
        {
            XIntc_AckIntr(INTC_ADDR, mask);
        }

        tableEntry = &(cfgPtr->HandlerTable[whichIntr]);

        tableEntry->Handler(tableEntry->CallBackRef);

        if (0 == ((cfgPtr->AckBeforeService) & mask))
        {
            XIntc_AckIntr(INTC_ADDR, mask);
        }

        intrStatus = XIntc_GetIntrStatus(INTC_ADDR);
    }
}

int Board_HardwareInit(void)
{
	int status;
	init_platform();
	RTOS_DisableInterrupts();
	status = board_InterruptInit();
	if (0 == status)
	{
		status = board_TimerInit();
	}
	return status;
}
