/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   high_resolution_timer_base.cpp
 * Author: fabiuz
 * 
 * Created on September 27, 2016, 2:20 AM
 */

#include "interfaces/arch_registers.h"
#include "kernel/kernel.h"
#include "kernel/scheduler/timer_interrupt.h"
#include "kernel/timeconversion.h"
#include "../../../../../debugpin.h"
#include "CMSIS/Include/core_cm3.h"
#include "bsp_impl.h"
#include "high_resolution_timer_base.h"

using namespace miosix;

const unsigned int timerBits=32;
const unsigned long long overflowIncrement=(1LL<<timerBits);
const unsigned long long lowerMask=overflowIncrement-1;
const unsigned long long upperMask=0xFFFFFFFFFFFFFFFFLL-lowerMask;

static long long ms32time = 0; //most significant 32 bits of counter
static long long ms32chkp[3] = {0}; //most significant 32 bits of check point
static bool lateIrq=false;

void __attribute__((naked)) TIMER1_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z10cstirqhnd1v");
    restoreContext();
}
void __attribute__((naked)) TIMER2_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z10cstirqhnd2v");
    restoreContext();
}

void __attribute__((used)) cstirqhnd2(){}



static inline unsigned int IRQread32Timer(){
    unsigned int high=TIMER3->CNT;
    unsigned int low=TIMER1->CNT;
    unsigned int high2=TIMER3->CNT;
    if(high==high2){
        return (high<<16)|low;
    }
    return high2<<16|TIMER1->CNT;
}

HighResolutionTimerBase& HighResolutionTimerBase::instance(){
    static HighResolutionTimerBase hrtb;
    return hrtb;
}

inline long long HighResolutionTimerBase::IRQgetSetTimeCCV0(){
    return ms32chkp[0] | TIMER3->CC[0].CCV<<16 | TIMER1->CC[0].CCV;
}
inline long long HighResolutionTimerBase::IRQgetSetTimeCCV1(){
    return ms32chkp[1] | TIMER3->CC[1].CCV<<16 | TIMER1->CC[1].CCV;
}
inline long long HighResolutionTimerBase::IRQgetSetTimeCCV2(){
    return ms32chkp[2] | TIMER3->CC[2].CCV<<16 | TIMER1->CC[2].CCV;
}

HighResolutionTimerBase::HighResolutionTimerBase() {
    //Power the timers up and PRS system
    {
        InterruptDisableLock l;
        CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_TIMER1 | CMU_HFPERCLKEN0_TIMER2
                | CMU_HFPERCLKEN0_TIMER3 | CMU_HFPERCLKEN0_PRS;
    }
    //Configure Timers
    TIMER1->CTRL = TIMER_CTRL_MODE_UP | TIMER_CTRL_CLKSEL_PRESCHFPERCLK 
            | TIMER_CTRL_PRESC_DIV1 | TIMER_CTRL_SYNC;
    TIMER2->CTRL = TIMER_CTRL_MODE_UP | TIMER_CTRL_CLKSEL_PRESCHFPERCLK 
            | TIMER_CTRL_PRESC_DIV1 | TIMER_CTRL_SYNC;
    TIMER3->CTRL = TIMER_CTRL_MODE_UP | TIMER_CTRL_CLKSEL_TIMEROUF 
            | TIMER_CTRL_SYNC;

    //Enable necessary interrupt lines
    TIMER1->IEN = TIMER_IEN_CC1;
    TIMER3->IEN = /*TIMER_IEN_OF |*/ TIMER_IEN_CC1;

    TIMER1->CC[1].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
    TIMER3->CC[1].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;

    NVIC_SetPriority(TIMER1_IRQn,3);
    NVIC_SetPriority(TIMER3_IRQn,3);
    NVIC_ClearPendingIRQ(TIMER3_IRQn);
    NVIC_ClearPendingIRQ(TIMER1_IRQn);
    NVIC_EnableIRQ(TIMER3_IRQn);
    NVIC_DisableIRQ(TIMER1_IRQn);
    
    //Start timers
    TIMER1->CMD = TIMER_CMD_START;
    //Setup tick2ns conversion tool
    timerFreq = 48000000;
}

Thread* HighResolutionTimerBase::tWaiting=nullptr;

HighResolutionTimerBase::~HighResolutionTimerBase() {}

long long HighResolutionTimerBase::IRQgetCurrentTick(){
    //PENDING BIT TRICK
    unsigned int counter=IRQread32Timer();
    if((TIMER3->IF & _TIMER_IFC_OF_MASK) && IRQread32Timer()>=counter)
        return (ms32time | static_cast<long long>(counter)) + overflowIncrement;
    return ms32time | static_cast<long long>(counter);
}

void HighResolutionTimerBase::setCCInterrupt0(bool enable){
    if(enable){
        TIMER3->IEN|=TIMER_IEN_CC0;
    }else{
        TIMER3->IEN&=~TIMER_IEN_CC0;
    }
}
void HighResolutionTimerBase::setCCInterrupt1(bool enable){
    if(enable)
        TIMER3->IEN|=TIMER_IEN_CC1;
    else
        TIMER3->IEN&=~TIMER_IEN_CC1;
}
void HighResolutionTimerBase::setCCInterrupt2(bool enable){
    if(enable)
        TIMER3->IEN|=TIMER_IEN_CC2;
    else
        TIMER3->IEN&=~TIMER_IEN_CC2;
}

void HighResolutionTimerBase::setCCInterrupt2Tim1(bool enable){
    if(enable)
        TIMER1->IEN|=TIMER_IEN_CC2;
    else
        TIMER1->IEN&=~TIMER_IEN_CC2;
}

//	In this function I prepare the timer, but i don't enable the timer.
void HighResolutionTimerBase::setModeGPIOTimer(bool input){    
    if(input){
	TIMER1->ROUTE=TIMER_ROUTE_CC2PEN
                | TIMER_ROUTE_LOCATION_LOC1;
	//Configuro la modalità input
	TIMER1->CC[2].CTRL=TIMER_CC_CTRL_MODE_INPUTCAPTURE |
                          TIMER_CC_CTRL_ICEDGE_RISING |
                          TIMER_CC_CTRL_INSEL_PIN; 
	
	//Configuro PRS Timer 3 deve essere un consumer, timer1 producer, TIMER3 keeps the most significative part
	//TIMER1->CC2 as producer, i have to specify the event i'm interest in    
	PRS->CH[0].CTRL|=PRS_CH_CTRL_SOURCESEL_TIMER1
			|PRS_CH_CTRL_SIGSEL_TIMER1CC2;

	//TIMER3->CC2 as consumer
	TIMER3->CC[2].CTRL=TIMER_CC_CTRL_PRSSEL_PRSCH0
			|   TIMER_CC_CTRL_INSEL_PRS
			|   TIMER_CC_CTRL_ICEDGE_BOTH
			|   TIMER_CC_CTRL_MODE_INPUTCAPTURE;
	
    }else{
        TIMER1->CC[2].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
        TIMER3->CC[2].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
    }
    
}
long long HighResolutionTimerBase::getCurrentTick(){
    bool interrupts=areInterruptsEnabled();
    //TODO: optimization opportunity, if we can guarantee that no call to this
    //function occurs before kernel is started, then we can use
    //fastInterruptDisable())
    if(interrupts) disableInterrupts();
    long long result=IRQgetCurrentTick();
    if(interrupts) enableInterrupts();
    return result;

}

bool HighResolutionTimerBase::IRQsetNextInterrupt0(long long tick){
    ms32chkp[0] = tick & upperMask;
    TIMER3->CC[0].CCV = static_cast<unsigned int>((tick & 0xFFFF0000)>>16);
    TIMER1->CC[0].CCV = static_cast<unsigned int>(tick & 0xFFFF);
    /*
    In order to prevent back to back interrupts due to a checkpoint
    set in the past, raise lateIrq flag to indicate this IRQ should
    be processed only once
    */
    if(IRQgetCurrentTick() >= IRQgetSetTimeCCV0())
    {
	//Now we should set TIMER3->IFS=TIMER_IFS_CC0
        NVIC_SetPendingIRQ(TIMER3_IRQn);
        lateIrq=true;
	return false;
    }
    return true;
}

bool HighResolutionTimerBase::IRQsetNextInterrupt1(long long tick){
    ms32chkp[1] = tick & upperMask;
    TIMER3->CC[1].CCV = static_cast<unsigned int>((tick & 0xFFFF0000)>>16);
    TIMER1->CC[1].CCV = static_cast<unsigned int>(tick & 0xFFFF);
    /*
    In order to prevent back to back interrupts due to a checkpoint
    set in the past, raise lateIrq flag to indicate this IRQ should
    be processed only once
    */
    if(IRQgetCurrentTick() >= IRQgetSetTimeCCV1())
    {
        NVIC_SetPendingIRQ(TIMER3_IRQn);
	// TIMER3->IFS=TIMER_IFS_CC1; ????
        lateIrq=true;
	return false;
    }
    return true;
}

bool HighResolutionTimerBase::IRQsetNextInterrupt2(long long tick){
    ms32chkp[2] = tick & upperMask;
    TIMER3->CC[2].CCV = static_cast<unsigned int>((tick & 0xFFFF0000)>>16);
    TIMER1->CC[2].CCV = static_cast<unsigned int>(tick & 0xFFFF);
    /*
    In order to prevent back to back interrupts due to a checkpoint
    set in the past, raise lateIrq flag to indicate this IRQ should
    be processed only once
    */
    if(IRQgetCurrentTick() >= IRQgetSetTimeCCV2())
    {
        NVIC_SetPendingIRQ(TIMER3_IRQn);
	// TIMER3->IFS=TIMER_IFS_CC2; ???
        lateIrq=true;
	return false;
    }
    return true;
}


