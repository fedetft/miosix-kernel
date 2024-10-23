/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "kernel/scheduler/scheduler.h"
#include "kernel/stage_2_boot.h"
#include "config/miosix_settings.h"
#include "interfaces_private/cpu.h"
#include "interfaces/arch_registers.h"
#include "interfaces/interrupts.h"

namespace miosix {

// Same code behavior but different code size TODO document or remove
// #define VARIANT

static void unexpectedInterrupt(void*)
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected Peripheral interrupt\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

const unsigned int numInterrupts=MIOSIX_NUM_PERIPHERAL_IRQ;
#ifdef VARIANT
void (*irqTable[numInterrupts])(void *)={ &unexpectedInterrupt };
void *irqArgs[numInterrupts];
#else
struct IrqForwardingEntry
{
    constexpr IrqForwardingEntry() : handler(&unexpectedInterrupt), arg(nullptr) {}
    void (*handler)(void *);
    void *arg;
};
IrqForwardingEntry irqTable[numInterrupts];
#endif

bool IRQregisterIrq(unsigned int id, void (*handler)(void*), void *arg) noexcept
{
    if(id>=numInterrupts) return false;
    #ifdef VARIANT
    if(irqTable[id]!=unexpectedInterrupt) return false;
    irqTable[id]=handler;
    irqArgs[id]=arg;
    #else
    if(irqTable[id].handler!=unexpectedInterrupt) return false;
    irqTable[id].handler=handler;
    irqTable[id].arg=arg;
    #endif
    return true;
}

void IRQunregisterIrq(unsigned int id) noexcept
{
    if(id>=numInterrupts) return;
    #ifdef VARIANT
    irqTable[id]=unexpectedInterrupt;
    irqArgs[id]=nullptr;
    #else
    irqTable[id].handler=unexpectedInterrupt;
    irqTable[id].arg=nullptr;
    #endif
}

bool IRQisIrqRegistered(unsigned int id) noexcept
{
    if(id>=numInterrupts) return false;
    #ifdef VARIANT
    return irqTable[id]==unexpectedInterrupt;
    #else
    return irqTable[id].handler==unexpectedInterrupt;
    #endif
}

void IRQinvokeScheduler() noexcept
{
    doYield();
}

// NOTE: more compact assembly is produced if this function is not marked noexcept
template<int N> void irqProxy() /*noexcept*/
{
    #ifdef VARIANT
    (*irqTable[N])(irqArgs[N]);
    #else
    (*irqTable[N].handler)(irqTable[N].arg);
    #endif
}

// If all the ARM Cortex microcontrollers had the same number of interrupts, we
// could just write the interrupt proxy table explicitly in the following way:
//
// extern const fnptr interruptProxyTable[numInterrupts]=
// {
//     &irqProxy<0>, &irqProxy<1>, ... &irqProxy<numInterrupts-1>
// };
//
// However, numInterrupts is different in different microcontrollers, so we use
// template metaprogramming to programmatically generate the interrupt proxy
// table  whose size is the value of numInterrupts.

typedef void (*fnptr)();

template<fnptr... args>
struct InterruptProxyTable
{
    const fnptr entry[sizeof...(args)]={ args... };
};

template<unsigned N, fnptr... args>
struct TableGenerator
{
    typedef typename TableGenerator<N-1, &irqProxy<N-1>, args...>::type type;
    static const type table;
};

template<fnptr... args>
struct TableGenerator<0, args...>
{
    typedef InterruptProxyTable<args...> type;
    static const type table;
};

template<unsigned N, fnptr... args>
const typename TableGenerator<N, args...>::type TableGenerator<N, args...>::table;

#ifdef WITH_ERRLOG

/**
 * \internal
 * Used to print an unsigned int in hexadecimal format, and to reboot the system
 * Note that printf/iprintf cannot be used inside an IRQ, so that's why there's
 * this function.
 * \param x number to print
 */
static void printUnsignedInt(unsigned int x)
{
    static const char hexdigits[]="0123456789abcdef";
    char result[]="0x........\r\n";
    for(int i=9;i>=2;i--)
    {
        result[i]=hexdigits[x & 0xf];
        x>>=4;
    }
    IRQerrorLog(result);
}

#endif //WITH_ERRLOG

#if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)

/**
 * \internal
 * \return the program counter of the thread that was running when the exception
 * occurred.
 */
static unsigned int getProgramCounter()
{
    register unsigned int result;
    // Get program counter when the exception was thrown from stack frame
    asm volatile("mrs   %0,  psp    \n\t"
                 "add   %0, %0, #24 \n\t"
                 "ldr   %0, [%0]    \n\t":"=r"(result));
    return result;
}

#endif //WITH_PROCESSES || WITH_ERRLOG

void NMI_Handler()
{
    IRQerrorLog("\r\n***Unexpected NMI\r\n");
    IRQsystemReboot();
}

void HardFault_Handler()
{
    #ifdef WITH_PROCESSES
    if(Thread::IRQreportFault(FaultData(fault::HARDFAULT,getProgramCounter())))
    {
        IRQinvokeScheduler();
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected HardFault @ ");
    printUnsignedInt(getProgramCounter());
    #if !defined(_ARCH_CORTEXM0_STM32F0) && !defined(_ARCH_CORTEXM0_STM32G0) && !defined(_ARCH_CORTEXM0PLUS_STM32L0) && !defined(_ARCH_CORTEXM0PLUS_RP2040)
    unsigned int hfsr=SCB->HFSR;
    if(hfsr & 0x40000000) //SCB_HFSR_FORCED
        IRQerrorLog("Fault escalation occurred\r\n");
    if(hfsr & 0x00000002) //SCB_HFSR_VECTTBL
        IRQerrorLog("A BusFault occurred during a vector table read\r\n");
    #endif // !_ARCH_CORTEXM0_STM32F0 && !_ARCH_CORTEXM0_STM32G0 && !_ARCH_CORTEXM0PLUS_STM32L0
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

// Cortex M0/M0+ architecture does not have the interrupts handled by code
// below this point
#if !defined(_ARCH_CORTEXM0_STM32F0) && !defined(_ARCH_CORTEXM0_STM32G0) && !defined(_ARCH_CORTEXM0PLUS_STM32L0) && !defined(_ARCH_CORTEXM0PLUS_RP2040)

void MemManage_Handler()
{
    #if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)
    unsigned int cfsr=SCB->CFSR;
    #endif //WITH_PROCESSES || WITH_ERRLOG
    #ifdef WITH_PROCESSES
    int id, arg=0;
    if(cfsr & 0x00000001) id=fault::MP_XN;
    else if(cfsr & 0x00000080) { id=fault::MP; arg=SCB->MMFAR; }
    else id=fault::MP_NOADDR;
    if(Thread::IRQreportFault(FaultData(id,getProgramCounter(),arg)))
    {
        SCB->SHCSR &= ~(1<<13); //Clear MEMFAULTPENDED bit
        IRQinvokeScheduler();
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected MemManage @ ");
    printUnsignedInt(getProgramCounter());
    if(cfsr & 0x00000080) //SCB_CFSR_MMARVALID
    {
        IRQerrorLog("Fault caused by attempted access to ");
        printUnsignedInt(SCB->MMFAR);
    } else IRQerrorLog("The address that caused the fault is missing\r\n");
    if(cfsr & 0x00000010) //SCB_CFSR_MSTKERR
        IRQerrorLog("Fault occurred during exception stacking\r\n");
    if(cfsr & 0x00000008) //SCB_CFSR_MUNSTKERR
        IRQerrorLog("Fault occurred during exception unstacking\r\n");
    if(cfsr & 0x00000002) //SCB_CFSR_DACCVIOL
        IRQerrorLog("Fault was caused by invalid PC\r\n");
    if(cfsr & 0x00000001) //SCB_CFSR_IACCVIOL
        IRQerrorLog("Fault was caused by attempted execution from XN area\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

void BusFault_Handler()
{
    #if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)
    unsigned int cfsr=SCB->CFSR;
    #endif //WITH_PROCESSES || WITH_ERRLOG
    #ifdef WITH_PROCESSES
    int id, arg=0;
    if(cfsr & 0x00008000) { id=fault::BF; arg=SCB->BFAR; }
    else id=fault::BF_NOADDR;
    if(Thread::IRQreportFault(FaultData(id,getProgramCounter(),arg)))
    {
        SCB->SHCSR &= ~(1<<14); //Clear BUSFAULTPENDED bit
        IRQinvokeScheduler();
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected BusFault @ ");
    printUnsignedInt(getProgramCounter());
    if(cfsr & 0x00008000) //SCB_CFSR_BFARVALID
    {
        IRQerrorLog("Fault caused by attempted access to ");
        printUnsignedInt(SCB->BFAR);
    } else IRQerrorLog("The address that caused the fault is missing\r\n");
    if(cfsr & 0x00001000) //SCB_CFSR_STKERR
        IRQerrorLog("Fault occurred during exception stacking\r\n");
    if(cfsr & 0x00000800) //SCB_CFSR_UNSTKERR
        IRQerrorLog("Fault occurred during exception unstacking\r\n");
    if(cfsr & 0x00000400) //SCB_CFSR_IMPRECISERR
        IRQerrorLog("Fault is imprecise\r\n");
    if(cfsr & 0x00000200) //SCB_CFSR_PRECISERR
        IRQerrorLog("Fault is precise\r\n");
    if(cfsr & 0x00000100) //SCB_CFSR_IBUSERR
        IRQerrorLog("Fault happened during instruction fetch\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

void UsageFault_Handler()
{
    #if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)
    unsigned int cfsr=SCB->CFSR;
    #endif //WITH_PROCESSES || WITH_ERRLOG
    #ifdef WITH_PROCESSES
    int id;
    if(cfsr & 0x02000000) id=fault::UF_DIVZERO;
    else if(cfsr & 0x01000000) id=fault::UF_UNALIGNED;
    else if(cfsr & 0x00080000) id=fault::UF_COPROC;
    else if(cfsr & 0x00040000) id=fault::UF_EXCRET;
    else if(cfsr & 0x00020000) id=fault::UF_EPSR;
    else if(cfsr & 0x00010000) id=fault::UF_UNDEF;
    else id=fault::UF_UNEXP;
    if(Thread::IRQreportFault(FaultData(id,getProgramCounter())))
    {
        SCB->SHCSR &= ~(1<<12); //Clear USGFAULTPENDED bit
        IRQinvokeScheduler();
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected UsageFault @ ");
    printUnsignedInt(getProgramCounter());
    if(cfsr & 0x02000000) //SCB_CFSR_DIVBYZERO
        IRQerrorLog("Divide by zero\r\n");
    if(cfsr & 0x01000000) //SCB_CFSR_UNALIGNED
        IRQerrorLog("Unaligned memory access\r\n");
    if(cfsr & 0x00080000) //SCB_CFSR_NOCP
        IRQerrorLog("Attempted coprocessor access\r\n");
    if(cfsr & 0x00040000) //SCB_CFSR_INVPC
        IRQerrorLog("EXC_RETURN not expected now\r\n");
    if(cfsr & 0x00020000) //SCB_CFSR_INVSTATE
        IRQerrorLog("Invalid EPSR usage\r\n");
    if(cfsr & 0x00010000) //SCB_CFSR_UNDEFINSTR
        IRQerrorLog("Undefined instruction\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

void DebugMon_Handler()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected DebugMon @ ");
    printUnsignedInt(getProgramCounter());
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

#endif // !defined(_ARCH_CORTEXM0_STM32F0) && !defined(_ARCH_CORTEXM0_STM32G0) && !defined(_ARCH_CORTEXM0PLUS_STM32L0) && !defined(_ARCH_CORTEXM0PLUS_RP2040)

void SVC_Handler()
{
    #ifdef WITH_PROCESSES
    // WARNING: Temporary fix. Rationale:
    // This fix is intended to avoid kernel or process faulting due to
    // another process actions. Consider the case in which a process statically
    // allocates a big array such that there is no space left for saving
    // context data. If the process issues a system call, in the following
    // interrupt the context is saved, but since there is no memory available
    // for all the context data, a mem manage interrupt is set to 'pending'. Then,
    // a fake syscall is issued, based on the value read on the stack (which
    // the process hasn't set due to the memory fault and is likely to be 0);
    // this syscall is usually a yield (due to the value of 0 above),
    // which can cause the scheduling of the kernel thread. At this point,
    // the pending mem fault is issued from the kernel thread, causing the
    // kernel fault and reboot. This is caused by the mem fault interrupt
    // having less priority of the other interrupts.
    // This fix checks if there is a mem fault interrupt pending, and, if so,
    // it clears it and returns before calling the previously mentioned fake
    // syscall.
    if(SCB->SHCSR & (1<<13))
    {
        if(Thread::IRQreportFault(FaultData(fault::MP,0,0)))
        {
            SCB->SHCSR &= ~(1<<13); //Clear MEMFAULTPENDED bit
            return;
        }
    }
    Thread::IRQstackOverflowCheck(); //BUG! here we check the stack but we haven't saved the context!

    //If processes are enabled, check the content of r3. If zero then it
    //it is a simple yield, otherwise handle the syscall
    //Note that it is required to use ctxsave and not cur->ctxsave because
    //at this time we do not know if the active context is user or kernel
    unsigned int threadSp=ctxsave[0];
    unsigned int *processStack=reinterpret_cast<unsigned int*>(threadSp);
    if(processStack[3]!=static_cast<unsigned int>(Syscall::YIELD))
        Thread::IRQhandleSvc(processStack[3]);
    IRQinvokeScheduler(); //TODO: is it right to invoke the scheduler always? Check
    #else //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected SVC @ ");
    printUnsignedInt(getProgramCounter());
    #endif //WITH_ERRLOG
    IRQsystemReboot();
    #endif //WITH_PROCESSES
}

void PendSV_Handler() __attribute__((naked));
void PendSV_Handler()
{
    saveContext();
    //Call ISR_yield(). Name is a C++ mangled name.
    asm volatile("bl _ZN6miosix9ISR_yieldEv");
    restoreContext();
}

/**
 * \internal
 * Called by the software interrupt, yield to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions. Function is not
 * static because otherwise the compiler optimizes it out...
 */
void ISR_yield() __attribute__((noinline));
void ISR_yield()
{
    Thread::IRQstackOverflowCheck();
    Scheduler::IRQrunScheduler();
}


void Reset_Handler() __attribute__((__interrupt__, noreturn));
//Stack top, defined in the linker script
extern char _main_stack_top asm("_main_stack_top");

struct InterruptTable
{
    constexpr InterruptTable(char* stackptr, fnptr i1, fnptr i2, fnptr i3,
                             fnptr i4, fnptr i5, fnptr i6, fnptr i7,
                             fnptr i8, fnptr i9, fnptr i10, fnptr i11,
                             fnptr i12, fnptr i13, fnptr i14, fnptr i15,
                             TableGenerator<numInterrupts>::type interruptProxyTable)
    : stackptr(stackptr), i1(i1), i2(i2), i3(i3), i4(i4), i5(i5), i6(i6), i7(i7),
      i8(i8), i9(i9), i10(i10), i11(i11), i12(i12), i13(i13), i14(i14), i15(i15),
      interruptProxyTable(interruptProxyTable) {}

    char *stackptr;
    fnptr i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15;
    // The rest of the interrupt table, the one for peripheral interrupts is
    // generated programmatically using template metaprogramming to produce the
    // proxy functions that allow dynamically registering interrupts.
    TableGenerator<numInterrupts>::type interruptProxyTable;
};

__attribute__((section(".isr_vector"))) const InterruptTable systemInterruptTable
(
    #if !defined(_ARCH_CORTEXM0_STM32F0) && !defined(_ARCH_CORTEXM0_STM32G0) && !defined(_ARCH_CORTEXM0PLUS_STM32L0) && !defined(_ARCH_CORTEXM0PLUS_RP2040)
        &_main_stack_top,    // Stack pointer
        Reset_Handler,       // Reset Handler
        NMI_Handler,         // NMI Handler
        HardFault_Handler,   // Hard Fault Handler
        MemManage_Handler,   // MPU Fault Handler
        BusFault_Handler,    // Bus Fault Handler
        UsageFault_Handler,  // Usage Fault Handler
        nullptr,             // Reserved
        nullptr,             // Reserved
        nullptr,             // Reserved
        nullptr,             // Reserved
        SVC_Handler,         // SVCall Handler
        DebugMon_Handler,    // Debug Monitor Handler
        nullptr,             // Reserved
        PendSV_Handler,      // PendSV Handler
        nullptr,             // SysTick Handler (Miosix does not use it)
    #else // !defined(_ARCH_CORTEXM0_STM32F0) && !defined(_ARCH_CORTEXM0_STM32G0) && !defined(_ARCH_CORTEXM0PLUS_STM32L0) && !defined(_ARCH_CORTEXM0PLUS_RP2040)
        &_main_stack_top,    // Stack pointer
        Reset_Handler,       // Reset Handler
        NMI_Handler,         // NMI Handler
        HardFault_Handler,   // Hard Fault Handler
        nullptr,             // Reserved
        nullptr,             // Reserved
        nullptr,             // Reserved
        nullptr,             // Reserved
        nullptr,             // Reserved
        nullptr,             // Reserved
        nullptr,             // Reserved
        SVC_Handler,         // SVCall Handler
        nullptr,             // Reserved
        nullptr,             // Reserved
        PendSV_Handler,      // PendSV Handler
        nullptr,             // SysTick Handler (Miosix does not use it)
    #endif // !defined(_ARCH_CORTEXM0_STM32F0) && !defined(_ARCH_CORTEXM0_STM32G0) && !defined(_ARCH_CORTEXM0PLUS_STM32L0) && !defined(_ARCH_CORTEXM0PLUS_RP2040)
    TableGenerator<numInterrupts>::table
);

/**
 * Called by Reset_Handler, performs initialization and calls main.
 * Never returns.
 */
void program_startup() __attribute__((noreturn));
void program_startup()
{
    //Cortex M3 core appears to get out of reset with interrupts already enabled
    __disable_irq();
    //NOTE: needed by some MCUS such as ATSam4l where the SAM-BA bootloader
    //does not relocate the vector table offset
    SCB->VTOR = reinterpret_cast<unsigned int>(&systemInterruptTable);

    //These are defined in the linker script
    extern unsigned char _etext asm("_etext");
    extern unsigned char _data asm("_data");
    extern unsigned char _edata asm("_edata");
    extern unsigned char _bss_start asm("_bss_start");
    extern unsigned char _bss_end asm("_bss_end");

    //Initialize .data section, clear .bss section
    unsigned char *etext=&_etext;
    unsigned char *data=&_data;
    unsigned char *edata=&_edata;
    unsigned char *bss_start=&_bss_start;
    unsigned char *bss_end=&_bss_end;
    memcpy(data, etext, edata-data);
    memset(bss_start, 0, bss_end-bss_start);

    //Move on to stage 2
    _init();

    //If main returns, reboot
    NVIC_SystemReset();
    for(;;) ;
}

/**
 * Reset handler, called by hardware immediately after reset
 */
void Reset_Handler()
{
    /*
     * SystemInit() is called *before* initializing .data and zeroing .bss
     * Despite all startup files provided by ATMEL do the opposite, there are
     * three good reasons to do so:
     * First, the CMSIS specifications say that SystemInit() must not access
     * global variables, so it is actually possible to call it before
     * Second, when running Miosix with the xram linker scripts .data and .bss
     * are placed in the external RAM, so we *must* call SystemInit(), which
     * enables xram, before touching .data and .bss
     * Third, this is a performance improvement since the loops that initialize
     * .data and zeros .bss now run with the CPU at full speed instead of 115kHz
     * Note that it is called before switching stacks because the memory
     * at _heap_end can be unavailable until the external RAM is initialized.
     */
    SystemInit();

    /*
     * Initialize process stack and switch to it.
     * This is required for booting Miosix, a small portion of the top of the
     * heap area will be used as stack until the first thread starts. After,
     * this stack will be abandoned and the process stack will point to the
     * current thread's stack.
     */
    asm volatile("ldr r0,  =_heap_end          \n\t"
                 "msr psp, r0                  \n\t"
                 "movw r0, #2                  \n\n" //Privileged, process stack
                 "msr control, r0              \n\t"
                 "isb                          \n\t":::"r0");

    program_startup();
}

} //namespace miosix
