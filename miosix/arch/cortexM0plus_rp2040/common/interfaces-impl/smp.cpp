/***************************************************************************
 *   Copyright (C) 2025 by Daniele Cattaneo                                *
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

#include "interfaces_private/smp.h"

#ifdef WITH_SMP

#include "kernel/thread.h"
#include "interfaces_private/os_timer.h"
#include "interfaces/arch_registers.h"
#include "interfaces/cpu_const.h"
#include "interfaces_private/cpu.h"
#include "kernel/error.h"

// System mode stack size for core 1
#define CORE1_SYSTEM_STACK_SIZE 0x200
// Stringification macros used for embedding the above macro in assembly code
#define XSTR(a) STR(a)
#define STR(a) #a

namespace miosix {

char core1SystemStack[CORE1_SYSTEM_STACK_SIZE];

static void fifoDrain()
{
    while(sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS)
        (unsigned int)sio_hw->fifo_rd;
}

static bool fifoSend(unsigned long s)
{
    // Send
    while(!(sio_hw->fifo_st & SIO_FIFO_ST_RDY_BITS)) ;
    sio_hw->fifo_wr=s;
    __SEV();
    // Read back and check the value is the same
    while(!(sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS)) ;
    unsigned long r=sio_hw->fifo_rd;
    return s==r;
}

static unsigned long fifoReceive()
{
    // Read the new value
    while(!(sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS)) __WFE();
    unsigned long r=sio_hw->fifo_rd;
    // Send the value back for acknowledgment
    while(!(sio_hw->fifo_st & SIO_FIFO_ST_RDY_BITS)) ;
    sio_hw->fifo_wr=r;
    return r;
}

struct IPIFlags
{
    enum
    {
        InvokeScheduler = 1,
        CallOnCore = 2,
        HangUp = 4
    };
};
struct IPICall
{
    void (*func)(void *);
    void *arg;
    Thread *waiting;
};
unsigned char flags[2];
IPICall invocations[2];

void IRQinterProcessorInterruptHandler()
{
    FastGlobalLockFromIrq dLock;
    if(sio_hw->fifo_st & (SIO_FIFO_ST_ROE_BITS|SIO_FIFO_ST_WOF_BITS))
    {
        // Interrupt was triggered by a fifo error, clear the error.
        // This can happen if IRQinvokeSchedulerOnCore() is called way too
        // many times in rapid succession.
        sio_hw->fifo_st &= ~(SIO_FIFO_ST_ROE_BITS|SIO_FIFO_ST_WOF_BITS);
    }
    // Flush the FIFO
    fifoDrain();
    // Check what we have to do
    unsigned char coreId = getCurrentCoreId();
    if(flags[coreId] & IPIFlags::InvokeScheduler) IRQinvokeScheduler();
    if(flags[coreId] & IPIFlags::CallOnCore)
    {
        IPICall& inv=invocations[coreId];
        inv.func(inv.arg);
        inv.waiting->IRQwakeup();
        inv.waiting=nullptr;
    }
    if(flags[coreId] & IPIFlags::HangUp)
    {
        FastGlobalUnlockFromIrq dUnlock(dLock);
        for(;;) ;
    }
    // Clear the flags
    flags[coreId]=0;
}

__attribute__((naked)) void initCore1()
{
    asm volatile(
        "cpsid i                              \n" //Disable interrupts
        "mrs r0, msp                          \n"
        "msr psp, r0                          \n" //Use current stack as PSP
        "ldr r0, =_ZN6miosix16core1SystemStackE+" XSTR(CORE1_SYSTEM_STACK_SIZE) "\n"
        "msr msp, r0                          \n" //Select correct MSP
        "movs r0, #2                          \n" //Privileged, process stack
        "msr control, r0                      \n" //Activate PSP
        "isb                                  \n" //Required when switching stack
        "bl _ZN6miosix20IRQcontinueInitCore1Ev\n" //Continue core 1 init
    );
}

__attribute__((noreturn)) void IRQcontinueInitCore1()
{
    // Read main function info from FIFO
    void (*f)()=reinterpret_cast<void (*)()>(fifoReceive());
    // Initialize all interrupts to a default priority
    NVIC_SetPriority(SVCall_IRQn,defaultIrqPriority);
    NVIC_SetPriority(PendSV_IRQn,defaultIrqPriority);
    NVIC_SetPriority(SysTick_IRQn,defaultIrqPriority);
    const unsigned int numInterrupts=MIOSIX_NUM_PERIPHERAL_IRQ;
    for(unsigned int i=0;i<numInterrupts;i++)
        NVIC_SetPriority(static_cast<IRQn_Type>(i),defaultIrqPriority);
    // Register IPI (FIFO) interrupt handler for core 1
    IRQregisterIrq(SIO_IRQ_PROC1_IRQn,IRQinterProcessorInterruptHandler);
    // Register timer interrupt handler for core 1
    IRQosTimerInitSMP();
    // Clear fifo status flags and pending interrupt flag to avoid spurious
    // interrupts on core 1 side
    sio_hw->fifo_st=0;
    NVIC_ClearPendingIRQ(SIO_IRQ_PROC1_IRQn);
    // Enable SEVONPEND for core 1, needed for spinlocks that still allow
    // for interrupt servicing
    SCB->SCR|=SCB_SCR_SEVONPEND_Msk;
    // Signal to the other core that we are done with setup
    __DSB();
    (unsigned long)sio_hw->spinlock[HwLocks::RP2040InitCoreSync];
    // Call the main function, which shouldn't return. If it does, hang up
    f();
    for(;;) ;
}

void IRQinitSMP(void *const stackPtrs[], void (*const mains[])()) noexcept
{
    // Ensure the core setup end spinlock is not taken
    sio_hw->spinlock[HwLocks::RP2040InitCoreSync]=1;
    __DSB();
    // Send FIFO commands for the bootrom core idling mechanism
    for(;;)
    {
        fifoDrain();
        __SEV();
        if(!fifoSend(0)) continue;
        fifoDrain();
        __SEV();
        if(!fifoSend(0)) continue;
        if(fifoSend(1)) break;
    }
    unsigned long vtor=SCB->VTOR;
    unsigned long psp=reinterpret_cast<unsigned long>(stackPtrs[0]);
    unsigned long pc=reinterpret_cast<unsigned long>(&initCore1);
    if (!fifoSend(vtor)) errorHandler(Error::UNEXPECTED);
    if (!fifoSend(psp)) errorHandler(Error::UNEXPECTED);
    if (!fifoSend(pc)) errorHandler(Error::UNEXPECTED);
    // Send main function address and args, which will be read by
    // IRQcontinueInitCore1
    unsigned long fp=reinterpret_cast<unsigned long>(mains[0]);
    if (!fifoSend(fp)) errorHandler(Error::UNEXPECTED);
    // Register IPI (FIFO) interrupt handler for core 0
    IRQregisterIrq(SIO_IRQ_PROC0_IRQn,IRQinterProcessorInterruptHandler);
    // Register timer interrupt handler for core 0
    IRQosTimerInitSMP();
    // Clear fifo status flags and pending interrupt flag to avoid spurious
    // interrupts on core 0 side
    sio_hw->fifo_st=0;
    NVIC_ClearPendingIRQ(SIO_IRQ_PROC0_IRQn);
    // Enable SEVONPEND for core 0, needed for spinlocks that still allow
    // for interrupt servicing
    SCB->SCR|=SCB_SCR_SEVONPEND_Msk;
    // Wait until core 1 is done
    __DSB();
    while(!(sio_hw->spinlock_st & (1<<HwLocks::RP2040InitCoreSync))) ;
    sio_hw->spinlock[HwLocks::RP2040InitCoreSync]=1;
}

void IRQinvokeSchedulerOnCore(unsigned char core) noexcept
{
    // Is this already the right core?
    if(core==getCurrentCoreId()) { IRQinvokeScheduler(); return; }
    // If not, trigger the IPI
    flags[core]|=IPIFlags::InvokeScheduler;
    __DSB();
    sio_hw->fifo_wr=0;
}

void IRQcallOnCore(GlobalIrqLock& lock, unsigned char core, void (*f)(void *),
                   void *arg) noexcept
{
    // Is this already the right core?
    if(core==getCurrentCoreId()) { f(arg); return; }
    // If there is a pending call on core, panic because this is impossible
    if(flags[core]&IPIFlags::CallOnCore) errorHandler(Error::UNEXPECTED);
    // Save the invocation which will be read by the call on core later
    invocations[core].func=f;
    invocations[core].arg=arg;
    invocations[core].waiting=Thread::getCurrentThread();
    // Trigger the IPI and wait for it to be served
    flags[core]|=IPIFlags::CallOnCore;
    __DSB();
    sio_hw->fifo_wr=0;
    do {
        Thread::IRQglobalIrqUnlockAndWait(lock);
    } while(invocations[core].waiting);
}

void IRQlockupOtherCores() noexcept
{
    flags[1-getCurrentCoreId()]|=IPIFlags::HangUp;
    __DSB();
    sio_hw->fifo_wr=0;
}

} // namespace miosix

#endif //WITH_SMP
