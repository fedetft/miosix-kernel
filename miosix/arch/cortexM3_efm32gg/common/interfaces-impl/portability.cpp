/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014, 2015 by Terraneo Federico *
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

#include "interfaces/portability.h"
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "interfaces/bsp.h"
#include "kernel/scheduler/scheduler.h"
#include "kernel/scheduler/timer_interrupt.h"
#include <algorithm>
#include "fixme_dma.h"

#define ADCSAMPLES                        1024

extern volatile uint16_t ramBufferAdcData1[ADCSAMPLES];
extern volatile uint16_t ramBufferAdcData2[ADCSAMPLES];

DMA_DESCRIPTOR_TypeDef dmaControlBlock[DMACTRL_CH_CNT * 2] __attribute__ ((aligned(DMACTRL_ALIGNMENT)));

/**
 * \internal
 * software interrupt routine.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
void SVC_Handler() __attribute__((naked));
void SVC_Handler()
{
    saveContext();
	//Call ISR_yield(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private9ISR_yieldEv");
    restoreContext();
}

void DMA_IRQHandler()
{
    DMA_DESCRIPTOR_TypeDef *descr = (DMA_DESCRIPTOR_TypeDef *)(DMA->CTRLBASE);
    
    int                    channel;
    miosix_private::DMA_CB_TypeDef         *cb;
    uint32_t               pending;
    uint32_t               pendingPrio;
    uint32_t               prio;
    uint32_t               primaryCpy;
    int                    i;

    
    /* Get all pending interrupts */
    pending = DMA->IF;

    /* Check for bus error */
    if (pending & DMA_IF_ERR)
    {
      /* Loop here to enable the debugger to see what has happened */ 
      return;
    }
    
    prio        = DMA->CHPRIS;
    pendingPrio = pending & prio;
    for (i = 0; i < 2; i++)
    {
      channel = 0;
      while (pendingPrio)
      {
          
        if (pendingPrio & 1)
        {
          DMA->IFC = 1 << channel;
          cb = (miosix_private::DMA_CB_TypeDef *)(descr[channel].USER);
          if (cb)
          {
                
                primaryCpy   = cb->primary;
                cb->primary ^= 1;
                if (cb->cbFunc)
                {   
                    
                    cb->cbFunc(channel, (bool)primaryCpy, cb->userPtr);
                }
          }
        }

        pendingPrio >>= 1;
        channel++;
      }

      /* On second iteration, process default priority channels */
      pendingPrio = pending & ~prio;
    }
}


#ifdef SCHED_TYPE_CONTROL_BASED
/**
 * \internal
 * Auxiliary timer interupt routine.
 * Used for variable lenght bursts in control based scheduler.
 * Since inside naked functions only assembler code is allowed, this function
 * only calls the ctxsave/ctxrestore macros (which are in assembler), and calls
 * the implementation code in ISR_yield()
 */
void XXX_IRQHandler() __attribute__((naked));
void XXX_IRQHandler()
{
    saveContext();
    //Call ISR_auxTimer(). Name is a C++ mangled name.
    asm volatile("bl _ZN14miosix_private12ISR_auxTimerEv");
    restoreContext();
}
#endif //SCHED_TYPE_CONTROL_BASED

namespace miosix_private {
    
 DMA_CB_TypeDef cb;

void setup_DMA(DMA_CB_TypeDef cbtemp)
{   
    DMA_Init_TypeDef        dmaInit;
    DMA_CfgChannel_TypeDef  chnlCfg;
    DMA_CfgDescr_TypeDef    descrCfg;
    
    DMA_Reset();
    
    /* Initializing the DMA */  
    dmaInit.hprot        = 0;
    dmaInit.controlBlock = dmaControlBlock;
    DMA_Init(&dmaInit);
    
    /* Setup call-back function */
    cb.cbFunc  = cbtemp.cbFunc;
    cb.userPtr = NULL;
    
    /* Setting up channel */
    chnlCfg.highPri   = false;
    chnlCfg.enableInt = true;
    chnlCfg.select    = DMAREQ_ADC0_SINGLE;
    chnlCfg.cb        = &cb;
    DMA_CfgChannel(DMA_CHANNEL_ADC, &chnlCfg);
    
    /* Setting up channel descriptor */
    descrCfg.dstInc  = dmaDataInc2;	// tell dma to increment by a byte
    descrCfg.srcInc  = dmaDataIncNone;
    descrCfg.size    = dmaDataSize2;	// tell dma to read just one byte
    descrCfg.arbRate = dmaArbitrate1;
    descrCfg.hprot   = 0;

    DMA_CfgDescr(DMA_CHANNEL_ADC, true, &descrCfg); // configure primary descriptor
    DMA_CfgDescr(DMA_CHANNEL_ADC, false, &descrCfg);  // configure alternate descriptor
    
    DMA_begin();
}

void DMA_Pause(void)
{
    
    DMA_DESCRIPTOR_TypeDef *descr;
    descr = ((DMA_DESCRIPTOR_TypeDef *)(DMA->CTRLBASE));
    descr->CTRL &= 0xFFFFFFF8; 
}

void DMA_begin(void)
{
    miosix_private::DMA_ActivatePingPong(DMA_CHANNEL_ADC,
                                        false,
                                        (void *) ramBufferAdcData1,
                                        (void *) &(ADC0->SINGLEDATA),
                                        ADCSAMPLES - 1,
                                        (void *) ramBufferAdcData2,
                                        (void *) &(ADC0->SINGLEDATA),
                                        ADCSAMPLES - 1);
}
    
void DMA_Reset(void)
{
    int i;
    
    /* Disable DMA interrupts */
    NVIC_DisableIRQ(DMA_IRQn);

    /* Put the DMA controller into a known state, first disabling it. */
    DMA->CONFIG      = _DMA_CONFIG_RESETVALUE;
    DMA->CHUSEBURSTC = _DMA_CHUSEBURSTC_MASK;
    DMA->CHREQMASKC  = _DMA_CHREQMASKC_MASK;
    DMA->CHENC       = _DMA_CHENC_MASK;
    DMA->CHALTC      = _DMA_CHALTC_MASK;
    DMA->CHPRIC      = _DMA_CHPRIC_MASK;
    DMA->ERRORC      = DMA_ERRORC_ERRORC;
    DMA->IEN         = _DMA_IEN_RESETVALUE;
    DMA->IFC         = _DMA_IFC_MASK;

    /* Clear channel control flags */
    for (i = 0; i < DMA_CHAN_COUNT; i++)
    {
        DMA->CH[i].CTRL = _DMA_CH_CTRL_RESETVALUE;
    }
} 
    
    
void DMA_CfgDescr(unsigned int channel, bool primary,DMA_CfgDescr_TypeDef *cfg)
{
    DMA_DESCRIPTOR_TypeDef *temp;

    /* Find descriptor to configure */
    if (primary)
    {
        temp = (DMA_DESCRIPTOR_TypeDef *)DMA->CTRLBASE ;
    }
    else
    {
        temp = (DMA_DESCRIPTOR_TypeDef *)DMA->ALTCTRLBASE;
    }
    temp += channel;
    
    temp->CTRL =    (cfg->dstInc << _DMA_CTRL_DST_INC_SHIFT) |
                    (cfg->size << _DMA_CTRL_DST_SIZE_SHIFT)  |
                    (cfg->srcInc << _DMA_CTRL_SRC_INC_SHIFT) |
                    (cfg->size << _DMA_CTRL_SRC_SIZE_SHIFT)  |
                    ((uint32_t)(cfg->hprot) << _DMA_CTRL_SRC_PROT_CTRL_SHIFT) |
                    (cfg->arbRate << _DMA_CTRL_R_POWER_SHIFT)|
                    (0 << _DMA_CTRL_N_MINUS_1_SHIFT)         | /* Set when activated */
                    (0 << _DMA_CTRL_NEXT_USEBURST_SHIFT)     | /* Set when activated */
                    DMA_CTRL_CYCLE_CTRL_INVALID;               /* Set when activated */
}

void DMA_Init(DMA_Init_TypeDef *init)
{
    CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_DMA;
     DMA_Reset();
     
    NVIC_ClearPendingIRQ(DMA_IRQn);
    NVIC_EnableIRQ(DMA_IRQn);

    DMA->IEN = DMA_IEN_ERR;

    DMA->CTRLBASE = (uint32_t)(init->controlBlock);

    DMA->CONFIG = ((uint32_t)(init->hprot) << _DMA_CONFIG_CHPROT_SHIFT) | DMA_CONFIG_EN;
}

void DMA_CfgChannel(unsigned int channel, DMA_CfgChannel_TypeDef *cfg)
{
  DMA_DESCRIPTOR_TypeDef *descr;
  
  descr               = (DMA_DESCRIPTOR_TypeDef *)(DMA->CTRLBASE);
  descr[channel].USER = (uint32_t)(cfg->cb);

  /* Set to specified priority for channel */
  if (cfg->highPri)
  {
    DMA->CHPRIS = 1 << channel;
  }
  else
  {
    DMA->CHPRIC = 1 << channel;
  }

  /* Set DMA signal source select */
  DMA->CH[channel].CTRL = cfg->select;

  /* Enable/disable interrupt as specified */
  if (cfg->enableInt)
  {
    DMA->IFC  = (1 << channel);
    DMA->IEN |= (1 << channel);
  }
  else
  {
    DMA->IEN =  ((DMA->IEN &  ~(1 << channel)) & 0x3FF); 
  }
}

void DMA_Prepare(unsigned int channel,DMA_CycleCtrl_TypeDef cycleCtrl,
                bool primary,bool useBurst,void *dst,void *src,unsigned int nMinus1)
{
    DMA_DESCRIPTOR_TypeDef *descr;
    DMA_DESCRIPTOR_TypeDef *primDescr;
    DMA_CB_TypeDef         *cb;
    uint32_t               inc;
    uint32_t               chBit;
    uint32_t               tmp;

    primDescr = ((DMA_DESCRIPTOR_TypeDef *)(DMA->CTRLBASE)) + channel;

    /* Find descriptor to configure */
    if (primary)
    {
      descr = primDescr;
    }
    else
    {
      descr = ((DMA_DESCRIPTOR_TypeDef *)(DMA->ALTCTRLBASE)) + channel;
    }

    cb = (DMA_CB_TypeDef *)(primDescr->USER);
    if (cb)
    {
      cb->primary = (uint8_t)primary;
    }

    if (src)
    {
      inc = (descr->CTRL & _DMA_CTRL_SRC_INC_MASK) >> _DMA_CTRL_SRC_INC_SHIFT;
      if (inc == _DMA_CTRL_SRC_INC_NONE)
      {
        descr->SRCEND = src;
      }
      else
      {
        descr->SRCEND = (void *)((uint32_t)src + (nMinus1 << inc));
      }
    }

    if (dst)
    {
      inc = (descr->CTRL & _DMA_CTRL_DST_INC_MASK) >> _DMA_CTRL_DST_INC_SHIFT;
      if (inc == _DMA_CTRL_DST_INC_NONE)
      {
        descr->DSTEND = dst;
      }
      else
      {
        descr->DSTEND = (void *)((uint32_t)dst + (nMinus1 << inc));
      }
    }

    chBit = 1 << channel;
    if (useBurst)
    {
      DMA->CHUSEBURSTS = chBit;
    }
    else
    {
      DMA->CHUSEBURSTC = chBit;
    }

    if (primary)
    {
      DMA->CHALTC = chBit;
    }
    else
    {
      DMA->CHALTS = chBit;
    }

    /* Set cycle control */
    tmp         = descr->CTRL & ~(_DMA_CTRL_CYCLE_CTRL_MASK | _DMA_CTRL_N_MINUS_1_MASK);
    tmp        |= nMinus1 << _DMA_CTRL_N_MINUS_1_SHIFT;
    tmp        |= (uint32_t)cycleCtrl << _DMA_CTRL_CYCLE_CTRL_SHIFT;
    descr->CTRL = tmp;
}

void DMA_RefreshPingPong(unsigned int channel, bool primary,bool useBurst,
                         void *dst, void *src, unsigned int nMinus1,bool stop)
{
    DMA_CycleCtrl_TypeDef  cycleCtrl;
    DMA_DESCRIPTOR_TypeDef *descr;
    uint32_t               inc;
    uint32_t               chBit;
    uint32_t               tmp;
    
    /* The ping-pong DMA cycle may be stopped by issuing a basic cycle type */
    if (stop)
    {
      cycleCtrl = dmaCycleCtrlBasic;
    }
    else
    {
      cycleCtrl = dmaCycleCtrlPingPong;
    }

    /* Find descriptor to configure */
    if (primary)
    {
      descr = ((DMA_DESCRIPTOR_TypeDef *)(DMA->CTRLBASE)) + channel;
    }
    else
    {
      descr = ((DMA_DESCRIPTOR_TypeDef *)(DMA->ALTCTRLBASE)) + channel;
    }

    if (src)
    {
      inc = (descr->CTRL & _DMA_CTRL_SRC_INC_MASK) >> _DMA_CTRL_SRC_INC_SHIFT;
      if (inc == _DMA_CTRL_SRC_INC_NONE)
      {
        descr->SRCEND = src;
      }
      else
      {
        descr->SRCEND = (void *)((uint32_t)src + (nMinus1 << inc));
      }
    }

    if (dst)
    {
      inc = (descr->CTRL & _DMA_CTRL_DST_INC_MASK) >> _DMA_CTRL_DST_INC_SHIFT;
      if (inc == _DMA_CTRL_DST_INC_NONE)
      {
        descr->DSTEND = dst;
      }
      else
      {
        descr->DSTEND = (void *)((uint32_t)dst + (nMinus1 << inc));
      }
    }

    chBit = 1 << channel;
    if (useBurst)
    {
      DMA->CHUSEBURSTS = chBit;
    }
    else
    {
      DMA->CHUSEBURSTC = chBit;
    }

    /* Set cycle control */
    tmp         = descr->CTRL & ~(_DMA_CTRL_CYCLE_CTRL_MASK | _DMA_CTRL_N_MINUS_1_MASK);
    tmp        |= nMinus1 << _DMA_CTRL_N_MINUS_1_SHIFT;
    tmp        |= cycleCtrl << _DMA_CTRL_CYCLE_CTRL_SHIFT;
    descr->CTRL = tmp;
}

void DMA_ActivatePingPong(unsigned int channel,bool useBurst,
                          void *primDst,void *primSrc,unsigned int primNMinus1,
                          void *altDst,void *altSrc,unsigned int altNMinus1)
{
      /* Prepare alternate descriptor first */
      DMA_Prepare(channel,dmaCycleCtrlPingPong,false,useBurst,altDst,altSrc,altNMinus1);

      /* Prepare primary descriptor last in order to start cycle using it */
      DMA_Prepare(channel,dmaCycleCtrlPingPong,true,useBurst,primDst,primSrc,primNMinus1);

      /* Enable channel, request signal is provided by peripheral device */
      DMA->CHENS = 1 << channel;
}

    void LETIMER0_IRQHandler()
    {   
        if (LETIMER0->IFS & LETIMER_IFS_COMP0)
        {
            LETIMER0->IFC |= LETIMER_IFC_COMP0;
        }
    }
    
    void RTC_IRQHandler()
    {
        RTC->IFC=RTC_IFC_COMP0;
    }

    void waitRtc(int val)
    {   
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
        EMU->CTRL=0;
        
        RTC->COMP0=(RTC->CNT+val) & 0xffffff;
        RTC->IEN |= RTC_IEN_COMP0;
        
        while(RTC->SYNCBUSY & RTC_SYNCBUSY_COMP0) 
        {   
            __WFI();
  
            //oscillatorInit(); //FIXME
            
            CMU->OSCENCMD=CMU_OSCENCMD_HFXOEN;
            CMU->CMD=CMU_CMD_HFCLKSEL_HFXO; //This locks the CPU till clock is stable
            CMU->OSCENCMD=CMU_OSCENCMD_HFRCODIS;

            //if(NVIC_GetPendingIRQ(RTC_IRQn)) break;
        }
        
        SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
        RTC->IEN &= ~RTC_IEN_COMP0;
    }

    void initRtc()
    {   
        CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_LE; //Enable clock to low energy peripherals
        CMU->LFACLKEN0 |= CMU_LFACLKEN0_RTC;
        while(CMU->SYNCBUSY & CMU_SYNCBUSY_LFACLKEN0) ;

        RTC->CNT=0;

        RTC->CTRL=RTC_CTRL_EN;
        while(RTC->SYNCBUSY & RTC_SYNCBUSY_CTRL) ;

        NVIC_EnableIRQ(RTC_IRQn);
    }

    void IRQdeepSleep()
    {
       waitRtc(32);
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
    IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();
}

#ifdef SCHED_TYPE_CONTROL_BASED
/**
 * \internal
 * Auxiliary timer interupt routine.
 * Used for variable lenght bursts in control based scheduler.
 */
void ISR_auxTimer() __attribute__((noinline));
void ISR_auxTimer()
{
    IRQstackOverflowCheck();
    miosix::Scheduler::IRQfindNextThread();//If the kernel is running, preempt
    if(miosix::kernel_running!=0) miosix::tick_skew=true;
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
    if(miosix::cur->ctxsave[0] < reinterpret_cast<unsigned int>(
            miosix::cur->watermark+watermarkSize))
        miosix::errorHandler(miosix::STACK_OVERFLOW);
}

void IRQsystemReboot()
{
    NVIC_SystemReset();
}

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv)
{
    unsigned int *stackPtr=sp;
    stackPtr--; //Stack is full descending, so decrement first
    *stackPtr=0x01000000; stackPtr--;                                 //--> xPSR
    *stackPtr=reinterpret_cast<unsigned long>(
            &miosix::Thread::threadLauncher); stackPtr--;             //--> pc
    *stackPtr=0xffffffff; stackPtr--;                                 //--> lr
    *stackPtr=0; stackPtr--;                                          //--> r12
    *stackPtr=0; stackPtr--;                                          //--> r3
    *stackPtr=0; stackPtr--;                                          //--> r2
    *stackPtr=reinterpret_cast<unsigned long >(argv); stackPtr--;     //--> r1
    *stackPtr=reinterpret_cast<unsigned long >(pc);                   //--> r0

    ctxsave[0]=reinterpret_cast<unsigned long>(stackPtr);             //--> psp
    //leaving the content of r4-r11 uninitialized
}

#ifdef WITH_PROCESSES

void initCtxsave(unsigned int *ctxsave, void *(*pc)(void *), unsigned int *sp,
        void *argv, unsigned int *gotBase)
{
    unsigned int *stackPtr=sp;
    stackPtr--; //Stack is full descending, so decrement first
    *stackPtr=0x01000000; stackPtr--;                                 //--> xPSR
    *stackPtr=reinterpret_cast<unsigned long>(pc); stackPtr--;        //--> pc
    *stackPtr=0xffffffff; stackPtr--;                                 //--> lr
    *stackPtr=0; stackPtr--;                                          //--> r12
    *stackPtr=0; stackPtr--;                                          //--> r3
    *stackPtr=0; stackPtr--;                                          //--> r2
    *stackPtr=0; stackPtr--;                                          //--> r1
    *stackPtr=reinterpret_cast<unsigned long >(argv);                 //--> r0

    ctxsave[0]=reinterpret_cast<unsigned long>(stackPtr);             //--> psp
    ctxsave[6]=reinterpret_cast<unsigned long>(gotBase);              //--> r9 
    //leaving the content of r4-r8,r10-r11 uninitialized
}

#endif //WITH_PROCESSES

void IRQportableStartKernel()
{
    //Enable fault handlers
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk
            | SCB_SHCSR_MEMFAULTENA_Msk;
    //Enable traps for division by zero. Trap for unaligned memory access
    //was removed as gcc starting from 4.7.2 generates unaligned accesses by
    //default (https://www.gnu.org/software/gcc/gcc-4.7/changes.html)
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
    NVIC_SetPriorityGrouping(7);//This should disable interrupt nesting
    NVIC_SetPriority(SVCall_IRQn,3);//High priority for SVC (Max=0, min=15)

    #ifdef SCHED_TYPE_CONTROL_BASED
    AuxiliaryTimer::IRQinit();
    #endif //SCHED_TYPE_CONTROL_BASED
    
    //create a temporary space to save current registers. This data is useless
    //since there's no way to stop the sheduler, but we need to save it anyway.
    unsigned int s_ctxsave[miosix::CTXSAVE_SIZE];
    ctxsave=s_ctxsave;//make global ctxsave point to it
}

void IRQportableFinishKernelStartup()
{
    //Note, we can't use enableInterrupts() now since the call is not mathced
    //by a call to disableInterrupts()
    __enable_fault_irq();
    __enable_irq();
    miosix::Thread::yield();
    //Never reaches here
}

void sleepCpu()
{
    __WFI();
}

#ifdef SCHED_TYPE_CONTROL_BASED
#error "AUX_TIMER not yet implemented"
void AuxiliaryTimer::IRQinit()
{

}

int AuxiliaryTimer::IRQgetValue()
{
    
}

void AuxiliaryTimer::IRQsetValue(int x)
{
    
}
#endif //SCHED_TYPE_CONTROL_BASED

} //namespace miosix_private
