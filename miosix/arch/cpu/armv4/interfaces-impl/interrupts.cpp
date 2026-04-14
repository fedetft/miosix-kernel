/***************************************************************************
 *   Copyright (C) 2008-2026 by Terraneo Federico                          *
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
#include "interfaces/poweroff.h"
#include "interfaces_private/cpu.h"
#include "config/miosix_settings.h"
#include "kernel/scheduler/scheduler.h"

namespace miosix {

//
// Declaration of interrupt handler functions, used to fill the interrupt table
//

void __attribute__((interrupt("IRQ"),naked)) emulatedPendSV_Handler();
void __attribute__((interrupt("IRQ"))) defaultInterrupt();
static void unexpectedInterrupt(void*);

//
// Code to build the interrupt forwarding table in RAM and generate the proxies
//

/**
 * \internal
 * In the VIC only up to 16 interrupts are actually vectored. To enable more
 * than 16 interrupts, the remaining ones would need to be demultiplexed in
 * software in the default interrupt handler. This code does not support that
 * feature, though. We currently only support registering up to 15 interrupts.
 *
 * Why 15 interrupts and not 16? We reserve the last interrupt vector (vector 15
 * as the count starts from 0) for the emulatedPendSV used for context switches.
 */
constexpr unsigned int numInterruptVectors=15;

/**
 * \internal
 * The ARMv4 VIC supports up to 32 hardware peripherals generating interrupts,
 * however not all of them exist on all chips. Moreover, we reserve interrupt
 * number 31 as the emulatedPendsvNumber
 */
constexpr unsigned int numInterruptNumbers=31;

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
static IrqForwardingEntry irqForwardingTable[numInterruptVectors];

/**
 * \internal
 * Interrupt proxy function. One instance of this function is generated for
 * each registrable peripheral interrupt in the VIC, and pointers to all these
 * functions are placed in the corresponding VIC entry.
 * Note that the need to have these forwarding functions is twofold. First, it
 * allows regular functions using regilar calling conventions to be registered
 * as interrupts, and second it is needed to pass args to registered interrupts,
 * something that the hardware doesn't do.
 * \tparam N which entry of the irqForwardingTable this function should refer to
 * NOTE: more compact assembly is produced if this function is not marked noexcept
 */
template<int N> void __attribute__((interrupt("IRQ"))) irqProxy() /*noexcept*/
{
    (*irqForwardingTable[N].handler)(irqForwardingTable[N].arg);

    //Every vectored interrupt should end by writing to VICVectAddr, so we do it
    //in the proxy functions
    VICVectAddr=0xff;
}

//
// Implementation of the interrupts.h interface
//

void IRQinitIrqTable()
{
    VICIntEnClr=0xffffffff; //Clear all interrupt enable
    VICIntEnable=1<<emulatedPendsvNumber; //Only enable emulated PendSV
    VICSoftIntClr=0xffffffff; //Clear all pending flags
    VICIntSelect=0; //No interrupt number enabled as FIQ
    VICVectAddr0=reinterpret_cast<unsigned int>(irqProxy<0>);
    VICVectAddr1=reinterpret_cast<unsigned int>(irqProxy<1>);
    VICVectAddr2=reinterpret_cast<unsigned int>(irqProxy<2>);
    VICVectAddr3=reinterpret_cast<unsigned int>(irqProxy<3>);
    VICVectAddr4=reinterpret_cast<unsigned int>(irqProxy<4>);
    VICVectAddr5=reinterpret_cast<unsigned int>(irqProxy<5>);
    VICVectAddr6=reinterpret_cast<unsigned int>(irqProxy<6>);
    VICVectAddr7=reinterpret_cast<unsigned int>(irqProxy<7>);
    VICVectAddr8=reinterpret_cast<unsigned int>(irqProxy<8>);
    VICVectAddr9=reinterpret_cast<unsigned int>(irqProxy<9>);
    VICVectAddr10=reinterpret_cast<unsigned int>(irqProxy<10>);
    VICVectAddr11=reinterpret_cast<unsigned int>(irqProxy<11>);
    VICVectAddr12=reinterpret_cast<unsigned int>(irqProxy<12>);
    VICVectAddr13=reinterpret_cast<unsigned int>(irqProxy<13>);
    VICVectAddr14=reinterpret_cast<unsigned int>(irqProxy<14>);
    VICVectAddr15=reinterpret_cast<unsigned int>(emulatedPendSV_Handler);
    VICDefVectAddr=reinterpret_cast<unsigned int>(defaultInterrupt);
    for(unsigned int i=0;i<numInterruptVectors;i++)
    {
        (&VICVectCntl0)[i]=0; //VIC slot disabled
        //Initialize all interrupts to a default handler and store the interrupt
        //vector number as arg for debugging use in unexpectedInterrupt
        irqForwardingTable[i].handler=&unexpectedInterrupt;
        #ifdef WITH_ERRLOG
        irqForwardingTable[i].arg=reinterpret_cast<void*>(i);
        #endif //WITH_ERRLOG
    }
    VICVectCntl15=0x20 | emulatedPendsvNumber; //VIC slot 15 = emulated PendSV
}

void IRQregisterIrq(GlobalIrqLock& lock, unsigned int id,
                    void (*handler)(void*), void *arg) noexcept
{
    // NOTE: the VIC slots are also the interrupt priorities, with VIC entry 0
    // having the highest priority. This is annoying, as interrupt priorities
    // will be implicitly determined by the order of interrupt registration.
    // For now, we'll register interrupts starting from the last so at least
    // system interrupts such as the timer and serial will get lower priority
    // than other drivers enabled after them, but that's not optimal.
    if(id>=numInterruptNumbers) errorHandler(Error::INTERRUPT_REGISTRATION_ERROR);
    for(int i=numInterruptVectors-1;i>=0;i--)
    {
        if((&VICVectCntl0)[i]!=0) continue;
        //Found free VIC slot
        VICIntEnable=1<<id;
        (&VICVectCntl0)[i]=0x20 | id;
        irqForwardingTable[i].handler=handler;
        irqForwardingTable[i].arg=arg;
        return;
    }
    //No free VIC slot to register IRQ, sorry
    errorHandler(Error::INTERRUPT_REGISTRATION_ERROR);
}

void IRQunregisterIrq(GlobalIrqLock& lock, unsigned int id,
                      void (*handler)(void*), void *arg) noexcept
{
    if(id>=numInterruptNumbers) errorHandler(Error::INTERRUPT_REGISTRATION_ERROR);
    for(int i=numInterruptVectors-1;i>=0;i--)
    {
        if((&VICVectCntl0)[i]!=(0x20 | id)) continue;
        //Attempted to unregister an interrupt without knowing who registered it
        if(irqForwardingTable[i].handler!=handler || irqForwardingTable[i].arg!=arg)
            errorHandler(Error::INTERRUPT_REGISTRATION_ERROR);
        (&VICVectCntl0)[i]=0;
        VICIntEnClr=1<<id;
        VICSoftIntClr=1<<id;
        irqForwardingTable[id].handler=unexpectedInterrupt;
        #ifdef WITH_ERRLOG
        irqForwardingTable[id].arg=reinterpret_cast<void*>(id);
        #endif //WITH_ERRLOG
        return;
    }
    //Attempted to unregister an interrupt that wasn't registered
    errorHandler(Error::INTERRUPT_REGISTRATION_ERROR);
}

bool IRQisIrqRegistered(unsigned int id) noexcept
{
    if(id>=numInterruptNumbers) return false;
    return (VICIntEnable & (1<<id)) ? true : false;
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
#endif //WITH_ERRLOG

//
// Hardware interrupt table and interrupt handlers
//

/**
 * In ARMv4 the interrupt table contains instructions, not data.
 * Thus it is a function.
 */
void __attribute__((section(".isr_vector"))) hardwareInterruptTable()
{
    asm volatile("ldr pc, Reset_Addr              \n\t"
                 "ldr pc, UNDEF_Addr              \n\t"
                 "ldr pc, SWI_Addr                \n\t"
                 "ldr pc, PABT_Addr               \n\t"
                 "ldr pc, DABT_Addr               \n\t"
                 "nop                             \n\t" // ISP checksum entry
                 "ldr pc, [pc,#-0xFF0]            \n\t" // 0xfffff030 VICVectAddr
                 "ldr pc, FIQ_Addr                \n\t"
                 "Reset_Addr: .word _ZN6miosix13Reset_HandlerEv \n\t"
                 "UNDEF_Addr: .word UNDEF_Handler \n\t"
                 "SWI_Addr:   .word SWI_Handler   \n\t"
                 "PABT_Addr:  .word PABT_Handler  \n\t"
                 "DABT_Addr:  .word DABT_Handler  \n\t"
                 "FIQ_Addr:   .word FIQ_Handler   \n\t");
}

void __attribute__((naked)) Reset_Handler()
{
    /*
     * ARMv4 such as ARM7TDMI CPUs have 7 operating modes: USR, SYS, SVC, IRQ,
     * FIQ, ABT, UND. USR and SYS share the stack pointer, but all other modes
     * have a separate stack, so cycle through all modes to initialize stacks.
     *
     * While we do so, we also set whether interrupts (IRQ) and fast interrupts
     * (FIQ) should be enbaled or not in each mode. Note that IRQ and FIQ are
     * disabled in SYS mode as Miosix performs the initial part of the boot with
     * interrupts disabled. They are enabled again before performing the first
     * context switch.
     *
     * Miosix does not use ABT and UND modes (the respective handlers only print
     * a message and reboot), so we don't use a dedicated stack for those. We
     * reuse the IRQ stack, thus entering ABT or UND modes due to a fault does
     * corrupt the IRQ stack, but that's not an issue since the board is
     * rebooted immediately afterward.
     * Miosix also does not use FIQ, but there is provision to let applications
     * use it. The FIQ stack size is by default zero in the linker script,
     * thus attempting to trigger a FIQ whose code uses the stack will not work,
     * but it's possible to increase the FIQ stack in the linker script.
     * Lastly, SVC and IRQ share the same stack as in Miosix they are mutually
     * exclusive.
     *
     * We also need to call IRQmemoryAndClockInit() for the early platform
     * initialization. Since on boards with an external memory this function is
     * reponsible for initializing external RAM, and the heap may be in external
     * memory, we call it with the IRQ stack which is guaranteed by convention
     * in how linker scripts are made for Miosix to always be in internal RAM.
     *
     * After that, since we haven't booted yet and thus we are not inside a
     * thread, we need to make the SYS stack pointer point somewhere.
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
    asm volatile("msr     CPSR_c, #0x1b|0xc0  \n\t" // UND: IRQ Off, FIQ Off
                 "ldr     sp, =_irq_stack_top \n\t"
                 "msr     CPSR_c, #0x17|0xc0  \n\t" // ABT: IRQ Off, FIQ Off
                 "ldr     sp, =_irq_stack_top \n\t"
                 "msr     CPSR_c, #0x11|0xc0  \n\t" // FIQ: IRQ Off, FIQ Off
                 "ldr     sp, =_fiq_stack_top \n\t"
                 "msr     CPSR_c, #0x12|0x80  \n\t" // IRQ: IRQ Off, FIQ On
                 "ldr     sp, =_irq_stack_top \n\t"
                 "msr     CPSR_c, #0x13|0x80  \n\t" // SVC: IRQ Off, FIQ On
                 "ldr     sp, =_irq_stack_top \n\t"
                 "msr     CPSR_c, #0x1f|0xc0  \n\t" // SYS: IRQ Off, FIQ Off
                 "ldr     sp, =_irq_stack_top \n\t"
                 "bl      _ZN6miosix21IRQmemoryAndClockInitEv \n\t"
                 "ldr     sp, =_heap_end      \n\t"
                 "bl      _ZN6miosix23IRQkernelBootEntryPointEv");
}

/**
 * \internal
 * This ISR handles Undefined instruction.
 * Prints an error message, showing an address near the instruction that caused
 * the fault. This address together with the map file allows finding the
 * function that caused the fault.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 */
extern "C" __attribute__((interrupt("UNDEF"))) void UNDEF_Handler()
{
    #ifdef WITH_ERRLOG
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt.
    int returnAddress;
    asm volatile("mov %0, lr" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected UNDEF @ ");
    printUnsignedInt(returnAddress);
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

/**
 * \internal
 * Miosix 3 does not use SWI as userspace processes are not supported on ARMv4.
 * Prints an error message, and reboots the system.
 */
extern "C" __attribute__((interrupt("SWI"))) void SWI_Handler()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected SWI\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

/**
 * \internal
 * This ISR handles prefetch abort.
 * Prints an error message, showing an address near the instruction that caused
 * the fault. This address together with the map file allows finding the
 * function that caused the fault.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 */
extern "C" __attribute__((interrupt("PABT"))) void PABT_Handler()
{
    #ifdef WITH_ERRLOG
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt. (lr has an offset of 4 during a data abort)
    int returnAddress;
    asm volatile("sub %0, lr, #4" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected prefetch abort @ ");
    printUnsignedInt(returnAddress);
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

/**
 * \internal
 * This ISR handles data abort.
 * Prints an error message, showing an address near the instruction that caused
 * the fault. This address together with the map file allows finding the
 * function that caused the fault.
 * Please note that when compiling with some level of optimization, the compiler
 * can inline functions so the address is no longer accurate.
 */
extern "C" __attribute__((interrupt("DABT"))) void DABT_Handler()
{
    #ifdef WITH_ERRLOG
    //These two instructions MUST be the first two instructions of the interrupt
    //routine. They store in return_address the pc of the instruction that
    //caused the interrupt. (lr has an offset of 8 during a data abort)
    int returnAddress;
    asm volatile("sub %0, lr, #8" : "=r"(returnAddress));

    IRQerrorLog("\r\n***Unexpected data abort @ ");
    printUnsignedInt(returnAddress);
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

/**
 * \internal
 * The Miosix kernel does not use FIQ.
 * Prints an error message, and reboots the system.
 * NOTE: user code that wants to use FIQ may need to increase its stack size in
 * the linker script
 */
extern "C" __attribute__((interrupt("FIQ"))) void FIQ_Handler()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Unexpected FIQ\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

/**
 * \internal
 * Emulated PendSV interrupt, call the scheduler to yield to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions. Function is not
 * static because otherwise the compiler optimizes it out...
 */
void __attribute__((noinline)) emulatedPendsvImpl()
{
    VICSoftIntClr=1<<emulatedPendsvNumber; //Clear software interrupt
    FastGlobalLockFromIrq lock;
    Thread::IRQstackOverflowCheck();
    Scheduler::IRQrunScheduler();
    //Every vectored interrupt should end by writing to VICVectAddr.
    //This is the only vectored interrupt that does not go through the proxy
    //functions so we need to do it here.
    VICVectAddr=0xff;
}

/**
 * \internal
 * software interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
void __attribute__((interrupt("IRQ"),naked)) emulatedPendSV_Handler()
{
    //In Miosix 3.0, a context switch is possible only from this interrupt,
    //unless processes are enabled in which case also fault handlers and the SVC
    //handler can do so. If there is the need to perform a context switch
    //from within another interrupt, such as if a peripheral driver interrupt
    //has woken up a thread whose priority is higher than the one that was
    //running when the interrupt occurred, the interrupt must call
    //IRQinvokeScheduler which causes the emulated PendSV interrupt (this
    //interrupt) to become pending. When the peripheral interrupt complets, this
    //interrupt is run calling the scheduler and performing the constext switch.
    //Note that just like in Miosix 2.x, in order to perform a context switch we
    //need to override the compiler generated function prologue and epilogue
    //as we need to save the entire CPU context with our OS-specific code.
    //To do so, we mark the function naked. As naked function can only contain
    //assembly code, we save and restore the context with the saveContext and
    //restoreContext macros written in assembly, and call the actual interrupt
    //implementation which is written in C++ in the function emulatedPendsvImpl().
    //Since we are forced to perform the function call in assembly, we need to
    //call the C++ mangled name of emulatedPendsvImpl()
    saveContextFromIrq();
    asm volatile("bl _ZN6miosix18emulatedPendsvImplEv");
    restoreContext();
}

/**
 * \internal
 * VIC Default interrupt handler.
 * The LPC2138 datasheet says that spurious interrups can occur, but until now
 * it never happened. If and when spurious interruts will occur, this code will
 * be modified to deal with them. Until then, this code just reboots the system.
 */
void __attribute__((interrupt("IRQ"))) defaultInterrupt()
{
    #ifdef WITH_ERRLOG
    IRQerrorLog("\r\n***Default IRQ\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

/**
 * \internal
 * Unexpected interrupt handler.
 * This function is called whenever an unregistered interrupt occurs. It is
 * called by the interrupt forwarding code so it must be a regular function that
 * uses function calling conventions, and differs from defaultInterrupt() which
 * is called by the VIC directly and thus needs to follow interrupt calling
 * conventions.
 * The implementation prints the interrupt number and reboots.
 */
static void unexpectedInterrupt(void* arg)
{
    FastGlobalLockFromIrq lock;
    #ifdef WITH_ERRLOG
    auto entryNum=reinterpret_cast<unsigned int>(arg);
    IRQerrorLog("\r\n***Caught unregistered interrupt vector number ");
    printUnsignedInt(entryNum);
    IRQerrorLog("\r\n");
    #endif //WITH_ERRLOG
    IRQsystemReboot();
}

} //namespace miosix
