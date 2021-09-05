/***************************************************************************
 *   Copyright (C) 2021 by Terraneo Federico                               *
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

#include "kernel/kernel.h"
#include "interfaces/os_timer.h"
#include "interfaces/arch_registers.h"

namespace miosix {

class ATSAM_AST_Timer : public TimerAdapter<ATSAM_AST_Timer, 32, 1>
{
public:
    static inline unsigned int IRQgetTimerCounter()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        return AST->AST_CV;
    }
    static inline void IRQsetTimerCounter(unsigned int v)
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_CV=v;
    }

    static inline unsigned int IRQgetTimerMatchReg()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        return AST->AST_AR0;
    }
    static inline void IRQsetTimerMatchReg(unsigned int v)
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_AR0=v;
    }

    static inline bool IRQgetOverflowFlag() { return AST->AST_SR & AST_SR_OVF; }
    static inline void IRQclearOverflowFlag()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_SCR=AST_SCR_OVF;
    }
    
    static inline bool IRQgetMatchFlag() { return AST->AST_SR & AST_SR_ALARM0; }
    static inline void IRQclearMatchFlag()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_SCR=AST_SCR_ALARM0;
    }
    
    static inline void IRQforcePendingIrq() { NVIC_SetPendingIRQ(AST_ALARM_IRQn); }

    static inline void IRQstopTimer()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_CR &= ~AST_CR_EN;
    }
    static inline void IRQstartTimer()
    {
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_CR |= AST_CR_EN;
    }
    
    static unsigned int IRQTimerFrequency() { return 16384; }
    
    static void IRQinitTimer()
    {
        start32kHzOscillator();

        //AST clock gating already enabled at boot
        PDBG|=PDBG_AST; //Stop AST during debugging
        
        //Select 32kHz clock
        AST->AST_CLOCK=AST_CLOCK_CSSEL_32KHZCLK;
        while(AST->AST_SR & AST_SR_CLKBUSY) ;
        AST->AST_CLOCK |= AST_CLOCK_CEN;
        while(AST->AST_SR & AST_SR_CLKBUSY) ;

        //Counter mode, not started, clear only on overflow, fastest prescaler
        AST->AST_CR=AST_CR_PSEL(0);
        
        //Initialize conter and match register
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_CV=0;
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_SCR=0xffffffff;

        //Interrupt on overflow or match
        AST->AST_IER=AST_IER_ALARM0 | AST_IER_OVF;
        //Wakeup from deep sleep on overflow or match
        while(AST->AST_SR & AST_SR_BUSY) ;
        AST->AST_WER=AST_WER_ALARM0 | AST_WER_OVF;
        
        //High priority for AST (Max=0, min=15)
        NVIC_SetPriority(AST_ALARM_IRQn,3); 
        NVIC_EnableIRQ(AST_ALARM_IRQn);
        NVIC_SetPriority(AST_OVF_IRQn,3);
        NVIC_EnableIRQ(AST_OVF_IRQn);
    }
};

static ATSAM_AST_Timer timer;
DEFAULT_OS_TIMER_INTERFACE_IMPLMENTATION(timer);
} //namespace miosix

void __attribute__((naked)) AST_ALARM_Handler()
{
    saveContext();
    asm volatile("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((naked)) AST_OVF_Handler()
{
    saveContext();
    asm volatile("bl _Z11osTimerImplv");
    restoreContext();
}

void __attribute__((used)) osTimerImpl()
{
    miosix::timer.IRQhandler();
}
