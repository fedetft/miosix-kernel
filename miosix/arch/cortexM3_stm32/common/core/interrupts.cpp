/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "kernel/logging.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"
#include "interrupts.h"

using namespace miosix;

//Look up table used by printUnsignedInt()
static const char hexdigits[]="0123456789abcdef";

/**
 * \internal
 * Used to print an unsigned int in hexadecimal format, and to reboot the system
 * Note that printf/iprintf cannot be used inside an IRQ, so that's why there's
 * this function.
 * \param x number to print
 */
#ifdef WITH_ERRLOG
static void printUnsignedInt(unsigned int x)
{
    char result[]="0x........\r\n";
    for(int i=9;i>=2;i--)
    {
        result[i]=hexdigits[x & 0xf];
        x>>=4;
    }
    IRQerrorLog(result);
}
#endif// WITH_ERRLOG

/**
 * \internal
 * Print the program counter of the thread that was running when the exception
 * occurred.
 */
static void printProgramCounter()
{
    #ifdef WITH_ERRLOG
    register unsigned int pc;
    //Get program counter when the exception was thrown from stack frame
    asm volatile("mrs   %0,  psp    \n\t"
                 "add   %0, %0, #24 \n\t"
                 "ldr   %0, [%0]    \n\t":"=r"(pc)); //24=offset of pc
    printUnsignedInt(pc);
    #endif //WITH_ERRLOG
}

/**
 * Wait until all data is sent to the console and reboot
 */
static void waitConsoleAndReboot()
{
    while(!Console::IRQtxComplete()) ; //Wait until all data sent
    miosix_private::IRQsystemReboot();
}

void NMI_Handler()
{
    IRQerrorLog("\r\n***Unexpected NMI\r\n");
    waitConsoleAndReboot();
}

void HardFault_Handler()
{
    IRQerrorLog("\r\n***Unexpected HardFault @ ");
    printProgramCounter();
    #ifdef WITH_ERRLOG
    unsigned int hfsr=SCB->HFSR;
    if(hfsr & SCB_HFSR_FORCED) IRQerrorLog("Fault escalation occurred\r\n");
    if(hfsr & SCB_HFSR_VECTTBL)
        IRQerrorLog("A BusFault occurred during a vector table read\r\n");
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void MemManage_Handler()
{
    IRQerrorLog("\r\n***Unexpected MemManage @ ");
    printProgramCounter();
    #ifdef WITH_ERRLOG
    unsigned int cfsr=SCB->CFSR;
    if(cfsr & SCB_CFSR_MMARVALID)
    {
        IRQerrorLog("Fault caused by attempted access to ");
        printUnsignedInt(SCB->MMFAR);
    } else IRQerrorLog("The address that caused the fault is missing\r\n");
    if(cfsr & SCB_CFSR_MSTKERR)
        IRQerrorLog("Fault occurred during exception stacking\r\n");
    if(cfsr & SCB_CFSR_MUNSTKERR)
        IRQerrorLog("Fault occurred during exception unstacking\r\n");
    if(cfsr & SCB_CFSR_DACCVIOL)
        IRQerrorLog("Fault was caused by invalid PC\r\n");
    if(cfsr & SCB_CFSR_IACCVIOL)
        IRQerrorLog("Fault was caused by attempted execution from XN area\r\n");
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void BusFault_Handler()
{
    IRQerrorLog("\r\n***Unexpected BusFault @ ");
    printProgramCounter();
    #ifdef WITH_ERRLOG
    unsigned int cfsr=SCB->CFSR;
    if(cfsr & SCB_CFSR_BFARVALID)
    {
        IRQerrorLog("Fault caused by attempted access to ");
        printUnsignedInt(SCB->BFAR);
    } else IRQerrorLog("The address that caused the fault is missing\r\n");
    if(cfsr & SCB_CFSR_STKERR)
        IRQerrorLog("Fault occurred during exception stacking\r\n");
    if(cfsr & SCB_CFSR_UNSTKERR)
        IRQerrorLog("Fault occurred during exception unstacking\r\n");
    if(cfsr & SCB_CFSR_IMPRECISERR) IRQerrorLog("Fault is imprecise\r\n");
    if(cfsr & SCB_CFSR_PRECISERR) IRQerrorLog("Fault is precise\r\n");
    if(cfsr & SCB_CFSR_IBUSERR)
        IRQerrorLog("Fault happened during instruction fetch\r\n");
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void UsageFault_Handler()
{
    IRQerrorLog("\r\n***Unexpected UsageFault @ ");
    printProgramCounter();
    #ifdef WITH_ERRLOG
    unsigned int cfsr=SCB->CFSR;
    if(cfsr & SCB_CFSR_DIVBYZERO) IRQerrorLog("Divide by zero\r\n");
    if(cfsr & SCB_CFSR_UNALIGNED) IRQerrorLog("Unaligned memory access\r\n");
    if(cfsr & SCB_CFSR_NOCP) IRQerrorLog("Attempted coprocessor access\r\n");
    if(cfsr & SCB_CFSR_INVPC) IRQerrorLog("EXC_RETURN not expected now\r\n");
    if(cfsr & SCB_CFSR_INVSTATE) IRQerrorLog("Invalid EPSR usage\r\n");
    if(cfsr & SCB_CFSR_UNDEFINSTR) IRQerrorLog("Undefined instruction\r\n");
    #endif //WITH_ERRLOG
    waitConsoleAndReboot();
}

void DebugMon_Handler()
{
    IRQerrorLog("\r\n***Unexpected DebugMon @ ");
    printProgramCounter();
    waitConsoleAndReboot();
}

void PendSV_Handler()
{
    IRQerrorLog("\r\n***Unexpected PendSV @ ");
    printProgramCounter();
    waitConsoleAndReboot();
}

void unexpectedInterrupt()
{
    IRQerrorLog("\r\n***Unexpected Peripheral interrupt\r\n");
    waitConsoleAndReboot();
}
