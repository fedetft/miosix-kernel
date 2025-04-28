/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
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

#include "util/util.h"
#include "kernel/logging.h"
#include "kernel/thread.h"
#include "kernel/process.h"
#include "kernel/scheduler/scheduler.h"
#include "kernel/boot.h"
#include "config/miosix_settings.h"
#include "interfaces/poweroff.h"
#include "interfaces_private/cpu.h"
#include "interfaces/arch_registers.h"
#include "interfaces/interrupts.h"

#ifndef __CORTEX_M
#error "__CORTEX_M undefined"
#endif //__CORTEX_M

namespace miosix {

//
// Declaration of interrupt handler functions, used to fill the interrupt table
//

extern char _main_stack_top asm("_main_stack_top"); //defined in the linker script
void __attribute__((naked)) Reset_Handler();
void NMI_Handler();
void HardFault_Handler();
#if __CORTEX_M != 0
void MemManage_Handler();
void BusFault_Handler();
void UsageFault_Handler();
void DebugMon_Handler();
#endif //__CORTEX_M != 0
void SVC_Handler();
void __attribute__((naked)) PendSV_Handler();
static void unexpectedInterrupt(void*);

//
// Code to build the interrupt table in FLASH and interrupt forwarding table in RAM
//

/**
 * \internal
 * numInterrupts is the size of the peripheral interrupt table of the chip
 * we are compiling for. We patch the \code enum IRQn \endcode in the CMSIS
 * to add the MIOSIX_NUM_PERIPHERAL_IRQ constant for every MCU we support.
 */
const unsigned int numInterrupts=MIOSIX_NUM_PERIPHERAL_IRQ;

/**
 * \internal
 * To enable interrupt registration at run-time and interrupt arg pointer
 * passing, we use one of these structs per peripheral interrupt allocated in RAM
 */
struct IrqForwardingEntry
{
    //NOTE: a constexpr constructor can be used to perform static initialization
    //of the table entries, but this uses a lot of Flash memory to store the
    //initialization values. We thus opted to leave the table uninitialized
    //and add the IRQinitIrqTable function that provides space efficient
    //initialization by means of a loop
    // constexpr IrqForwardingEntry() : handler(&unexpectedInterrupt), arg(nullptr) {}
    void (*handler)(void *);
    void *arg;
};

/// \internal Table of run-time registered interrupt handlers and args
static IrqForwardingEntry irqForwardingTable[numInterrupts];

/**
 * \internal
 * Interrupt proxy function. One instance of this function is generated for
 * each peripheral interrupt, and pointers to all these functions are placed in
 * the hardware interrupt table in Flash memory.
 * Note that even though we can move the hardware interrupt table in RAM, we
 * don't do so, as we also need to pass args to registered interrupts, something
 * that the hardware doesn't do, so we do need this proxy level to pass args
 * to interrupts
 * \tparam N which entry of the irqForwardingTable this function should refer to
 * NOTE: more compact assembly is produced if this function is not marked noexcept
 */
template<int N> void irqProxy() /*noexcept*/
{
    (*irqForwardingTable[N].handler)(irqForwardingTable[N].arg);
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
// table whose size is the value of numInterrupts.

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
};

template<fnptr... args>
struct TableGenerator<0, args...>
{
    typedef InterruptProxyTable<args...> type;
};

/**
 * \internal
 * This struct is the type whose memory layout corresponds to the ARM Cortex
 * hardware interrupt table.
 */
struct InterruptTable
{
    constexpr InterruptTable(char* stackptr, fnptr i1, fnptr i2, fnptr i3,
                             fnptr i4, fnptr i5, fnptr i6, fnptr i7,
                             fnptr i8, fnptr i9, fnptr i10, fnptr i11,
                             fnptr i12, fnptr i13, fnptr i14, fnptr i15)
    : stackptr(stackptr), i1(i1), i2(i2), i3(i3), i4(i4), i5(i5), i6(i6), i7(i7),
      i8(i8), i9(i9), i10(i10), i11(i11), i12(i12), i13(i13), i14(i14), i15(i15) {}

    // The first entries of the interrupt table are the stack pointer, reset
    // vector and "system" interrupt entries. We don't make these run-time
    // registrable nor provide arg pointers for those, as only the kernel
    // uses them
    char *stackptr;
    fnptr i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15;
    // The rest of the interrupt table, the one for peripheral interrupts is
    // generated programmatically using template metaprogramming to produce the
    // proxy functions that allow dynamically registering interrupts.
    TableGenerator<numInterrupts>::type interruptProxyTable;
};

/// \internal The actual hardware interrupt table, placed in the .isr_vector
/// section to make sure it is placed at the start of the Flash memory where the
/// hardware expects it
__attribute__((section(".isr_vector"))) extern const InterruptTable hardwareInterruptTable
(
    #if __CORTEX_M != 0
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
        nullptr              // SysTick Handler (Miosix does not use it)
    #else //__CORTEX_M != 0
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
        nullptr              // SysTick Handler (Miosix does not use it)
    #endif //__CORTEX_M != 0
);

//
// Implementation of the interrupts.h interface
//

void IRQinitIrqTable() noexcept
{
    //Cortex M0 non-plus do not have a VTOR register
    #if __CORTEX_M != 0 || (defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U))
    //Can't set VTOR to point to XRAM
    #ifndef __CODE_IN_XRAM
    //NOTE: the bootoader in some MCUs such as ATSam4l does not relocate the
    //vector table offset to the one in the firmware, so we force it here.
    SCB->VTOR=reinterpret_cast<unsigned int>(&hardwareInterruptTable);
    #endif
    #endif

    //Quirk: static_cast<IRQn_Type>(-5) is sometimes defined as SVCall_IRQn and
    //sometimes as SVC_IRQn. We could use complicated means to detect which
    //enumeration item is defined, or we could use its value directly which
    //is much simpler. This is not a constant that is subject to change anyway.
    NVIC_SetPriority(static_cast<IRQn_Type>(-5),defaultIrqPriority);
    NVIC_SetPriority(PendSV_IRQn,defaultIrqPriority);
    NVIC_SetPriority(SysTick_IRQn,defaultIrqPriority);
    #if __CORTEX_M != 0
    NVIC_SetPriority(BusFault_IRQn,defaultIrqPriority-1); //Higher
    NVIC_SetPriority(UsageFault_IRQn,defaultIrqPriority-1); //Higher
    NVIC_SetPriority(MemoryManagement_IRQn,defaultIrqPriority-1); //Higher
    NVIC_SetPriority(DebugMonitor_IRQn,defaultIrqPriority);
    NVIC_SetPriorityGrouping(7); //Disable interrupt nesting
    #endif //__CORTEX_M != 0
    //NOTE: Cortex M0 cannot disable interrupt nesting

    for(unsigned int i=0;i<numInterrupts;i++)
    {
        //Initialize all interrupts to a default priority and handler and store
        //the interrupt number as arg for debugging use in unexpectedInterrupt
        irqForwardingTable[i].handler=&unexpectedInterrupt;
        #ifdef WITH_ERRLOG
        irqForwardingTable[i].arg=reinterpret_cast<void*>(i);
        #endif //WITH_ERRLOG
        NVIC_SetPriority(static_cast<IRQn_Type>(i),defaultIrqPriority);
    }
}

void IRQregisterIrq(unsigned int id, void (*handler)(void*), void *arg) noexcept
{
    if(id>=numInterrupts || irqForwardingTable[id].handler!=unexpectedInterrupt)
        errorHandler(Error::INTERRUPT_REGISTRATION_ERROR);
    irqForwardingTable[id].handler=handler;
    irqForwardingTable[id].arg=arg;
    NVIC_EnableIRQ(static_cast<IRQn_Type>(id));
}

bool IRQisIrqRegistered(unsigned int id, void (*handler)(void*), void *arg) noexcept
{
    if(id>=numInterrupts || irqForwardingTable[id].handler!=unexpectedInterrupt)
        return false;
    irqForwardingTable[id].handler=handler;
    irqForwardingTable[id].arg=arg;
    NVIC_EnableIRQ(static_cast<IRQn_Type>(id));
    return true;
}

void IRQunregisterIrq(unsigned int id, void (*handler)(void*), void *arg) noexcept
{
    if(id>=numInterrupts
    || irqForwardingTable[id].handler!=handler
    || irqForwardingTable[id].arg!=arg)
        errorHandler(Error::INTERRUPT_REGISTRATION_ERROR);
    irqForwardingTable[id].handler=unexpectedInterrupt;
    #ifdef WITH_ERRLOG
    irqForwardingTable[id].arg=reinterpret_cast<void*>(id);
    #endif //WITH_ERRLOG
    NVIC_DisableIRQ(static_cast<IRQn_Type>(id));
    NVIC_ClearPendingIRQ(static_cast<IRQn_Type>(id));
}

//
// Support functions to implement interrupt handlers
//

#ifdef WITH_ERRLOG
/**
 * \internal
 * Used to print an unsigned int in hexadecimal format, and to reboot the system
 * This function exists because printf/iprintf cannot be used inside an IRQ.
 * \param x number to print
 */
static void printUnsignedInt(unsigned int x)
{
    char result[]="0x........\r\n";
    formatHex(result+2,x,8);
    IRQerrorLog(result);
}

/**
 * \internal
 * \return attempt to get the program counter of the thread that was running
 * when the exception occurred.
 */
static unsigned int tryGetKernelThreadProgramCounter()
{
    // Attempt to get program counter at the time the exception was thrown from
    // stack frame. This can fail if the stack pointer itself is corrupted, in
    // this case return 0xbadadd. Failing to validate the thread stack pointer
    // before dereferencing it may cause further memory faults and a CPU lockup
    unsigned int psp=__get_PSP();
    // Miosix uses the stack-in-heap approach for kernel threads, so compare the
    // stack pointer against the boundaries of the heap
    extern char _end asm("_end"); //defined in the linker script
    extern char _heap_end asm("_heap_end"); //defined in the linker script
    const unsigned int off=24; //Offset in bytes of PC in the stack frame on ARM
    if(psp<reinterpret_cast<unsigned int>(&_end)
    || psp>reinterpret_cast<unsigned int>(&_heap_end)-off-sizeof(unsigned int))
        return 0xbadadd;
    return *reinterpret_cast<unsigned int *>(psp+off);
}
#endif //WITH_ERRLOG

//
// Interrupt handlers
//

void __attribute__((naked)) Reset_Handler()
{
    /*
     * The reset handler is the first function that is called by hardware at
     * boot. The ARM Cortex cores come out of reset with the stack pointer
     * already set, thus it should be possible to write the boot code straight
     * in C/C++ but that's not so easy.
     *
     * The ARM Cortex cores have two stacks: main (MSP) and process (PSP).
     * After booting, Miosix uses the MSP only for interrupt handlers, which
     * are executed from a dedicated stack memory area placed in the
     * microcontroller internal memory. The top of this stack is identified by
     * the symbol _main_stack_top.
     * Miosix uses the PSP as the stack of the currently running thread.
     * Stack threads are allocated on the heap, which can also be in an external
     * SRAM or SDRAM memory for boards that have one.
     *
     * During boot, we must consider that if the board uses external memory,
     * this may not be available until we enable it. Part of the memory map
     * in the linker script may thus not be accessible.
     * Moreover, global variables in .data/.bss cannot be accessed because they
     * are only initialized when kernelBootEntryPoint() is called.
     *
     * Thus the early stage of the boot proceeds as follows:
     * The Cortex CPU comes out of reset using the MSP stack which points to
     * a small stack (it's meant to be used only for interrupts, after all)
     * that is always located in internal RAM. We take advantage of this stack
     * to call a board-specific function miosix::memoryAndClockInit() that
     * usually does only the following:
     * - Enable PLL and configure FLASH wait states
     * - Enable the external memory if available
     * This board-specific function shall not access global variables, as they
     * are surely not initialized and possibly located in an external memory
     * that isn't accessible yet.
     * When this function returns, though, the whole memory map is accessible.
     * Thus, we abandon the MSP stack (leaving it empty as the only function we
     * pushed just returned) and leave it for interrupts only, and use the PSP
     * stack instead.
     * Since we haven't booted yet and thus we are not inside a thread, we need
     * to make the PSP point somewhere.
     * Miosix reuses the top part of the heap for this. During this phase of
     * the boot, before the first context switch, we thus have no heap-stack
     * smashing protection, but we have access to a stack that is bigger than
     * the interrupt-only stack. After the first context switch, this stack too
     * will be abandoned although not in an empty state, but it doesn't matter
     * as we'll never return to it and its memory will just be reused as heap.
     *
     * The code in this reset handler thus switches stack pointer mid-function,
     * something that's best done in assembly, as the compiler may use the stack
     * implicitly. That's why we don't write this part in C++.
     *
     * After switching stack we continue boot by calling
     * miosix::IRQkernelBootEntryPoint()
     */
    asm volatile("cpsid i                                 \n\t" //Disable interrupts
                 "bl  _ZN6miosix21IRQmemoryAndClockInitEv \n\t" //Initialize PLL,FLASH,XRAM
                 "ldr r0,  =_heap_end                     \n\t" //Get pointer to heap end
                 "msr psp, r0                             \n\t" //and use it as PSP
                 "movs r0, #2                             \n\n" //Privileged, process stack
                 "msr control, r0                         \n\t" //Activate PSP
                 "isb                                     \n\t" //Required when switching stack
                 "bl  _ZN6miosix23IRQkernelBootEntryPointEv");  //Continue boot
}

void NMI_Handler()
{
    FastGlobalLockFromIrq lock;
    IRQerrorLog("\r\n***Unexpected NMI\r\n");
    IRQsystemReboot();
}

#ifdef WITH_PROCESSES
void __attribute__((naked)) HardFault_Handler()
{
    saveContext();
    asm volatile("bl _ZN6miosix13hardfaultImplEv");
    restoreContext();
}

void __attribute__((noinline)) hardfaultImpl()
{
    FastGlobalLockFromIrq lock;
    if(Thread::IRQreportFault(FaultData(fault::HARDFAULT))) return;
#else //WITH_PROCESSES
void HardFault_Handler()
{
    FastGlobalLockFromIrq lock;
#endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected HardFault @ ");
    printUnsignedInt(tryGetKernelThreadProgramCounter());
    #if __CORTEX_M != 0
    unsigned int hfsr=SCB->HFSR;
    if(hfsr & 0x40000000) //SCB_HFSR_FORCED
        IRQerrorLog("Fault escalation occurred\r\n");
    if(hfsr & 0x00000002) //SCB_HFSR_VECTTBL
        IRQerrorLog("A BusFault occurred during a vector table read\r\n");
    #endif //__CORTEX_M != 0
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

// Cortex M0/M0+ architecture does not have these interrupt handlers
#if __CORTEX_M != 0

#ifdef WITH_PROCESSES
void __attribute__((naked)) MemManage_Handler()
{
    saveContext();
    asm volatile("bl _ZN6miosix13memManageImplEv");
    restoreContext();
}

void __attribute__((noinline)) memManageImpl()
#else //WITH_PROCESSES
void MemManage_Handler()
#endif //WITH_PROCESSES
{
    FastGlobalLockFromIrq lock;
    #if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)
    unsigned int cfsr=SCB->CFSR;
    #endif //WITH_PROCESSES || WITH_ERRLOG
    #ifdef WITH_PROCESSES
    int id, arg=0;
    if(cfsr & 0x00000001) id=fault::MP_XN;
    else if(cfsr & 0x00000080) { id=fault::MP; arg=SCB->MMFAR; }
    else if(cfsr & 0x00000010)
    { 
        id=fault::MP_STACK;
        arg=ctxsave[getCurrentCoreId()][STACK_OFFSET_IN_CTXSAVE];
    } else id=fault::MP_NOADDR;
    if(Thread::IRQreportFault(FaultData(id,arg)))
    {
        //Clear MMARVALID, MLSPERR, MSTKERR, MUNSTKERR, DACCVIOL, IACCVIOL
        SCB->CFSR = 0x000000bb;
        //Clear MEMFAULTPENDED bit. Corrupted thread stack pointer causes memory
        //faults during exception stacking (MSTKERR bit), and if the core has an
        //FPU and the thread was using the FPU registers attempting to save
        //these registers causes a second memory fault (MSLPERR bit) which at
        //least on an STM32H755 causes the memory fault interrupt to become both
        //pending and active, thus it gets run twice. This is a problem since
        //the first run reports the fault and switches the thread context from
        //userspace to kernelspace, and then the second run finds the thread
        //already in kernelspace and erroneously attributes the fault to code
        //running in kernelspace triggering a reboot
        SCB->SHCSR &= ~(1<<13);
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected MemManage @ ");
    printUnsignedInt(tryGetKernelThreadProgramCounter());
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

#ifdef WITH_PROCESSES
void __attribute__((naked)) BusFault_Handler()
{
    saveContext();
    asm volatile("bl _ZN6miosix12busFaultImplEv");
    restoreContext();
}

void __attribute__((noinline)) busFaultImpl()
#else //WITH_PROCESSES
void BusFault_Handler()
#endif //WITH_PROCESSES
{
    FastGlobalLockFromIrq lock;
    #if defined(WITH_PROCESSES) || defined(WITH_ERRLOG)
    unsigned int cfsr=SCB->CFSR;
    #endif //WITH_PROCESSES || WITH_ERRLOG
    #ifdef WITH_PROCESSES
    int id, arg=0;
    if(cfsr & 0x00008000) { id=fault::BF; arg=SCB->BFAR; }
    else id=fault::BF_NOADDR;
    if(Thread::IRQreportFault(FaultData(id,arg)))
    {
        //Clear BFARVALID, LSPERR, STKERR, UNSTKERR, IMPRECISERR, PRECISERR, IBUSERR
        SCB->CFSR = 0x0000bf00;
        //Clear BUSFAULTPENDED as a defensive measure against issues similar to
        //the one described in MEMFAULTPENDED
        SCB->SHCSR &= ~(1<<14);
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected BusFault @ ");
    printUnsignedInt(tryGetKernelThreadProgramCounter());
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

#ifdef WITH_PROCESSES
void __attribute__((naked)) UsageFault_Handler()
{
    saveContext();
    asm volatile("bl _ZN6miosix14usageFaultImplEv");
    restoreContext();
}

void __attribute__((noinline)) usageFaultImpl()
#else //WITH_PROCESSES
void UsageFault_Handler()
#endif //WITH_PROCESSES
{
    FastGlobalLockFromIrq lock;
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
    if(Thread::IRQreportFault(FaultData(id)))
    {
        //Clear DIVBYZERO, UNALIGNED, UNDEFINSTR, INVSTATE, INVPC, NOCP
        SCB->CFSR = 0x030f0000;
        //Clear BUSFAULTPENDED as a defensive measure against issues similar to
        //the one described in USGFAULTPENDED
        SCB->SHCSR &= ~(1<<12);
        return;
    }
    #endif //WITH_PROCESSES
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected UsageFault @ ");
    printUnsignedInt(tryGetKernelThreadProgramCounter());
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
    FastGlobalLockFromIrq lock;
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected DebugMon @ ");
    printUnsignedInt(tryGetKernelThreadProgramCounter());
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

#endif //__CORTEX_M != 0

#ifdef WITH_PROCESSES
void __attribute__((naked)) SVC_Handler()
{
    saveContext();
    asm volatile("bl _ZN6miosix7svcImplEv");
    restoreContext();
}

void __attribute__((noinline)) svcImpl()
{
    FastGlobalLockFromIrq lock;
    Thread::IRQstackOverflowCheck();
    Thread::IRQhandleSvc();
}
#else //WITH_PROCESSES
void SVC_Handler()
{
    FastGlobalLockFromIrq lock;
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected SVC @ ");
    printUnsignedInt(tryGetKernelThreadProgramCounter());
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}
#endif //WITH_PROCESSES

/**
 * \internal
 * Called by the PendSV interrupt, call the scheduler to yield to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions. Function is not
 * static because otherwise the compiler optimizes it out...
 */
void __attribute__((noinline)) pendsvImpl()
{
    FastGlobalLockFromIrq lock;
    Thread::IRQstackOverflowCheck();
    Scheduler::IRQrunScheduler();
}

void __attribute__((naked)) PendSV_Handler()
{
    //In Miosix 3.0, a context switch is possible only from this interrupt,
    //unless processes are enabled in which case also fault handlers and the SVC
    //handler can do so. If there is the need to perform a context switch
    //from within another interrupt, such as if a peripheral driver interrupt
    //has woken up a thread whose priority is higher than the one that was
    //running when the interrupt occurred, the interrupt must call
    //IRQinvokeScheduler which causes the PendSV interrupt (this interrupt) to
    //become pending. When the peripheral interrupt complets, the PendSV
    //interrupt is run calling the scheduler and performing the constext switch.
    //Note that just like in Miosix 2.x, in order to perform a context switch we
    //need to override the compiler generated function prologue and epilogue
    //as we need to save the entire CPU context with our OS-specific code.
    //To do so, we mark the function naked. As naked function can only contain
    //assembly code, we save and restore the context with the saveContext and
    //restoreContext macros written in assembly, and call the actual interrupt
    //implementation which is written in C++ in the function pendsvImpl().
    //Since we are forced to perform the function call in assembly, we need to
    //call the C++ mangled name of pendsvImpl()
    saveContext();
    asm volatile("bl _ZN6miosix10pendsvImplEv");
    restoreContext();
}

static void unexpectedInterrupt(void* arg)
{
    FastGlobalLockFromIrq lock;
    #ifdef WITH_ERRLOG
    auto entryNum=reinterpret_cast<unsigned int>(arg);
    IRQerrorLog("\r\n***Caught unregistered interrupt number ");
    printUnsignedInt(entryNum);
    IRQerrorLog("\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

} //namespace miosix
