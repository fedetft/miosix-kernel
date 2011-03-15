/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010 by Terraneo Federico                   *
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
 //Miosix kernel

#include "LPC213x.h"
#include "interfaces/portability.h"
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "miosix.h"
#include "portability_impl.h"
#include "kernel/scheduler/scheduler.h"
#include "kernel/scheduler/tick_interrupt.h"
#include "board_settings.h"
#include <algorithm>

using namespace std;

//Used by the BSP. TODO: move this code into the "kernel thread"
void tickHook();

namespace miosix_private {

/**
 * \internal
 * Called by the timer interrupt, preempt to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions.
 * Function is not static because otherwise the compiler optimizes it out...
 */
void ISR_preempt() __attribute__((noinline));
void ISR_preempt()
{
	T0IR=0x1;//Clear interrupt
    VICVectAddr=0xff;//Restart VIC
    
    IRQstackOverflowCheck();
    miosix::IRQtickInterrupt();

    tickHook();
}

/**
 * \internal
 * timer interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in kernel_IRQ_Routine()
 */
void kernel_IRQ_Routine()   __attribute__ ((interrupt("IRQ"),naked));
void kernel_IRQ_Routine()
{
    saveContextFromIrq();
    //Call ISR_preempt(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private11ISR_preemptEv");
    restoreContext();
}

/**
 * \internal
 * Called by the software interrupt, yield to next thread
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions.
 * Function is not static because otherwise the compiler optimizes it out...
 */
void ISR_yield() __attribute__((noinline));
void ISR_yield()
{
    IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();
}

/**
 * \internal
 * software interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
extern "C" void kernel_SWI_Routine()   __attribute__ ((interrupt("SWI"),naked));
extern "C" void kernel_SWI_Routine()
{
    saveContextFromSwi();
	//Call ISR_yield(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private9ISR_yieldEv");
    restoreContext();
}

#ifdef SCHED_TYPE_CONTROL_BASED
/**
 * \internal
 * Auxiliary timer interupt routine.
 * Used for variable lenght bursts in control based scheduler.
 * Declared noinline to avoid the compiler trying to inline it into the caller,
 * which would violate the requirement on naked functions.
 * Function is not static because otherwise the compiler optimizes it out...
 */
void ISR_auxTimer() __attribute__((noinline));
void ISR_auxTimer()
{
    T1IR=0x1;//Clear interrupt
    VICVectAddr=0xff;//Restart VIC

    IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();//If the kernel is running, preempt
    if(miosix::kernel_running!=0) miosix::tick_skew=true;//The kernel is not running
}

/**
 * \internal
 * Auxiliary timer interupt routine.
 * Used for variable lenght bursts in control based scheduler.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
void kernel_auxTimer_Routine()   __attribute__ ((interrupt("IRQ"),naked));
void kernel_auxTimer_Routine()
{
    saveContextFromIrq();
    //Call ISR_auxTimer(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private12ISR_auxTimerEv");
    restoreContext();
}
#endif //SCHED_TYPE_CONTROL_BASED

void IRQstackOverflowCheck()
{
    const unsigned int watermarkSize=miosix::WATERMARK_LEN/sizeof(unsigned int);
    for(unsigned int i=0;i<watermarkSize;i++)
    {
        if(miosix::cur->watermark[i]!=miosix::WATERMARK_FILL)
            miosix::errorHandler(miosix::STACK_OVERFLOW);
    }
    if(miosix::cur->ctxsave[13] < reinterpret_cast<unsigned int>(
            miosix::cur->watermark+watermarkSize))
        miosix::errorHandler(miosix::STACK_OVERFLOW);
}

void IRQsystemReboot()
{
    //Jump to reset vector
    asm volatile("ldr pc, =0"::);
}

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
            void *argv)
{
    ctxsave[0]=(unsigned int)pc;// First function arg is passed in r0
    ctxsave[1]=(unsigned int)argv;
    ctxsave[2]=0;
    ctxsave[3]=0;
    ctxsave[4]=0;
    ctxsave[5]=0;
    ctxsave[6]=0;
    ctxsave[7]=0;
    ctxsave[8]=0;
    ctxsave[9]=0;
    ctxsave[10]=0;
    ctxsave[11]=0;
    ctxsave[12]=0;
    ctxsave[13]=(unsigned int)sp;//Initialize the thread's stack pointer
    ctxsave[14]=0xffffffff;//threadLauncher never returns, so lr is not important
    //Initialize the thread's program counter to the beginning of the entry point
    ctxsave[15]=(unsigned int)&miosix::Thread::threadLauncher;
    ctxsave[16]=0x1f;//thread starts in system mode with irq and fiq enabled.
}

void IRQportableStartKernel()
{
    PCONP|=(1<<1);//Enable TIMER0
    //Initialize the tick timer (timer0)
    T0TCR=0x0;//Stop timer
    T0CTCR=0x0;//Select "timer mode"
    T0TC=0x0;//Counter starts from 0
    T0PR=0x0;//No prescaler
    T0PC=0x0;//Prescaler counter starts from 0
    //Match value
    T0MR0=(miosix::TIMER_CLOCK/miosix::TICK_FREQ);
    T0MCR=0x3;//Reset and interrupt on match
    T0CCR=0;//No capture
    T0EMR=0;//No pin toggle on match
    //Init VIC
    VICSoftIntClr=(1<<4);//Clear timer interrupt flag (if previously set)
    VICIntSelect&=~(1<<4);//Timer0=IRQ
    VICIntEnable=(1<<4);//Timer0 interrupt ON
    VICVectCntl0=0x20 | 0x4;//Slot 0 of VIC used by Timer0
    VICVectAddr0=(unsigned long)&kernel_IRQ_Routine;
    T0TCR=0x1;//Start timer

    #ifdef SCHED_TYPE_CONTROL_BASED
    AuxiliaryTimer::IRQinit();
    #endif //SCHED_TYPE_CONTROL_BASED

    //create a temporary space to save current registers. This data is useless since there's no
    //way to stop the sheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];
    ctxsave=s_ctxsave;//make global ctxsave point to it
    miosix::Thread::yield();//Note that this automatically enables interrupts
    //Never reaches here
}

//IDL bit in PCON register
#define IDL (1<<0)

void sleepCpu()
{
    PCON|=IDL;
}

int atomicSwap(int newval, int *var)
{
    //ARM calling conventions say the 1st function arg is in r0, the second in r1
    //and the return value is in r0
    register int retval asm("r0");
    asm volatile(	"swp	r0, r0, [r1]	");
    return retval;
}

#ifdef SCHED_TYPE_CONTROL_BASED
void AuxiliaryTimer::IRQinit()
{
    //Timer1 configuration
    //Timer1 is used by the new control-based scheduler to implement preemption
    PCONP|=(1<<2);//Enable TIMER1
    T1TCR=0x0;//Stop timer
    T1CTCR=0x0;//Select "timer mode"
    T1TC=0x0;//Counter starts from 0
    T1PR=(miosix::TIMER_CLOCK/miosix::AUX_TIMER_CLOCK)-1;
    T1PC=0x0;//Prescaler counter starts from 0
    //Match value
    T1MR0=0xffffffff;//Will be set up properly the first time IRQFindNextThread
                     //is called
    T1MCR=0x1;//Generate interrupt on match, do NOT reset timer.
    T1CCR=0;//No capture
    T1EMR=0;//No pin toggle on match
    //Init VIC
    VICSoftIntClr=(1<<5);//Clear timer1 interrupt flag (if previously set)
    VICIntSelect&=~(1<<5);//Timer1=IRQ
    VICIntEnable=(1<<5);//Timer1 interrupt ON
    VICVectCntl1=0x20 | 0x5;//Slot 1 of VIC used by Timer1
    VICVectAddr1=(unsigned long)&kernel_auxTimer_Routine;
    T1TCR=0x1;//Start timer
}

int AuxiliaryTimer::IRQgetValue()
{
    return static_cast<int>(min<unsigned int>(T1TC,miosix::AUX_TIMER_MAX));
}

void AuxiliaryTimer::IRQsetValue(int x)
{
    T1TC=0;
    T1MR0=static_cast<unsigned int>(x);
    //The above instructions cause a spurious if not called within the
    //timer 1 IRQ (This happens if called from an SVC).
    //Clearing the pending bit prevents this spurious interrupt
    T1IR=0x1;//Clear interrupt
    VICSoftIntClr=(1<<5); //Clear timer1 interrupt flag
}
#endif //SCHED_TYPE_CONTROL_BASED

}; //namespace miosix_private
