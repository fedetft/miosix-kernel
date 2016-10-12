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
#include "high_resolution_timer_base.h"
#include "kernel/timeconversion.h"
#include "gpio_timer.h"

using namespace miosix;

const unsigned int timerBits=32;
const unsigned long long overflowIncrement=(1LL<<timerBits);
const unsigned long long lowerMask=overflowIncrement-1;
const unsigned long long upperMask=0xFFFFFFFFFFFFFFFFLL-lowerMask;

static long long ms32time = 0; //most significant 32 bits of counter
static long long ms32chkp[3] = {0,0,0}; //most significant 32 bits of check point
static bool lateIrq=false;
static TimeConversion* tc;

static inline unsigned int IRQread32Timer(){
    unsigned int high=TIMER3->CNT;
    unsigned int low=TIMER1->CNT;
    unsigned int high2=TIMER3->CNT;
    if(high==high2){
        return (high<<16)|low;
    }
    return high2<<16|TIMER1->CNT;
}

static inline long long IRQgetTick(){
    //PENDING BIT TRICK
    unsigned int counter=IRQread32Timer();
    if((TIMER3->IF & _TIMER_IFC_OF_MASK) && IRQread32Timer()>=counter)
        return (ms32time | static_cast<long long>(counter)) + overflowIncrement;
    return ms32time | static_cast<long long>(counter);
}

inline void interruptGPIOTimerRoutine(){
    if(TIMER1->CC[2].CTRL & TIMER_CC_CTRL_MODE_OUTPUTCOMPARE){
	expansion::gpio10::high();
    }else if(TIMER1->CC[2].CTRL & TIMER_CC_CTRL_MODE_INPUTCAPTURE){
	//Reactivating the thread that is waiting for the event.
	if(GPIOtimer::tWaitingGPIO)
        {
            GPIOtimer::tWaitingGPIO->IRQwakeup();
            if(GPIOtimer::tWaitingGPIO->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
                Scheduler::IRQfindNextThread();
            GPIOtimer::tWaitingGPIO=nullptr;
        }
    }
}

static inline void callScheduler(){
    TIMER1->IEN &= ~TIMER_IEN_CC1;
    TIMER3->IEN &= ~TIMER_IEN_CC1;
    TIMER1->IFC = TIMER_IFC_CC1;
    TIMER3->IFC = TIMER_IFC_CC1;
    long long tick = tc->tick2ns(IRQgetTick());
    IRQtimerInterrupt(tick);
}

static inline void setupTimers(){
    // We assume that this function is called only when the checkpoint is in future
    if (ms32chkp[1] == ms32time){
        // If the most significant 32bit matches, enable TIM3
        TIMER3->IFC = TIMER_IFC_CC1;
        TIMER3->IEN |= TIMER_IEN_CC1;
        if (static_cast<unsigned short>(TIMER3->CNT) >= static_cast<unsigned short>(TIMER3->CC[1].CCV) + 1){
            // If TIM3 matches by the time it is being enabled, disable it right away
            TIMER3->IFC = TIMER_IFC_CC1;
            TIMER3->IEN &= ~TIMER_IEN_CC1;
            // Enable TIM1 since TIM3 has been already matched
            TIMER1->IFC = TIMER_IFC_CC1;
            TIMER1->IEN |= TIMER_IEN_CC1;
            if (TIMER1->CNT >= TIMER1->CC[1].CCV){
                // If TIM1 matches by the time it is being enabled, call the scheduler right away
                callScheduler();
            }
        }
    }
    // If the most significant 32bit aren't match wait for TIM3 to overflow!
}

void __attribute__((naked)) TIMER3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z10cstirqhnd3v");
    restoreContext();
}

void __attribute__((naked)) TIMER1_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z10cstirqhnd1v");
    restoreContext();
}

void __attribute__((used)) cstirqhnd3(){
    //rollover
    if (TIMER3->IF & TIMER_IF_OF){
        TIMER3->IFC = TIMER_IFC_OF;
        ms32time += overflowIncrement;
        setupTimers();
    }
    //Checkpoint
    if ((TIMER3->IEN & TIMER_IEN_CC1) && (TIMER3->IF & TIMER_IF_CC1)){
        TIMER3->IFC = TIMER_IFC_CC1;
        if (static_cast<unsigned short>(TIMER3->CNT) >= static_cast<unsigned short>(TIMER3->CC[1].CCV) + 1){
            // Should happen if and only if most significant 32 bits have been matched
            TIMER3->IEN &= ~ TIMER_IEN_CC1;
            // Enable TIM1 since TIM3 has been already matched
            TIMER1->IFC = TIMER_IFC_CC1;
            TIMER1->IEN |= TIMER_IEN_CC1;
            if (TIMER1->CNT >= TIMER1->CC[1].CCV){
                // If TIM1 matches by the time it is being enabled, call the scheduler right away
                TIMER1->IFC = TIMER_IFC_CC1;
                callScheduler();
            }
        }
    } 
    
    //This if is to manage the GPIOtimer in OUTPUT Mode
    if((TIMER3->IEN & TIMER_IEN_CC2) && (TIMER3->IF & TIMER_IF_CC2)){
	if (ms32time == ms32chkp[2]){
	    TIMER3->IFC = TIMER_IFC_CC2;
	    TIMER3->IEN &= ~TIMER_IEN_CC2;
	    greenLed::high();
	
	    TIMER1->IFC = TIMER_IFC_CC2;
	    TIMER1->IEN |= TIMER_IEN_CC2;
	    if (TIMER1->CNT >= TIMER1->CC[2].CCV){
		TIMER1->IEN &= ~TIMER_IEN_CC2;
		TIMER1->IFC = TIMER_IFC_CC2;
		interruptGPIOTimerRoutine();
	    }
	}
    }
}
void __attribute__((used)) cstirqhnd1(){
    if ((TIMER1->IEN & TIMER_IEN_CC1) && (TIMER1->IF & TIMER_IF_CC1)){
        TIMER1->IFC = TIMER_IFC_CC1;
        // Should happen if and only if most significant 48 bits have been matched
        callScheduler();
    }
    
    //This if is used to manage the case of GPIOTimer, both INPUT and OUTPUT mode
    if ((TIMER1->IEN & TIMER_IEN_CC2) && (TIMER1->IF & TIMER_IF_CC2)){
        TIMER1->IEN &= ~TIMER_IEN_CC2;
	TIMER1->IFC = TIMER_IFC_CC2;
        interruptGPIOTimerRoutine();
    }
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

    //Code to entirely reset TIMER1, needed if you want run after the flash
    TIMER1->CMD=TIMER_CMD_STOP;
    TIMER1->CTRL=0;
    TIMER1->ROUTE=0;
    TIMER1->IEN=0;
    TIMER1->IFC=~0;
    TIMER1->TOP=0xFFFF;
    TIMER1->CNT=0;
    TIMER1->CC[0].CTRL=0;
    TIMER1->CC[0].CCV=0;
    TIMER1->CC[1].CTRL=0;
    TIMER1->CC[1].CCV=0;
    TIMER1->CC[2].CTRL=0;
    TIMER1->CC[2].CCV=0;

     
    
    //Enable necessary interrupt lines
    TIMER1->IEN = 0;
    TIMER3->IEN = TIMER_IEN_OF; //OF needed to increment the software counter (32-bit)

    TIMER1->CC[1].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
    TIMER3->CC[1].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;

    NVIC_SetPriority(TIMER1_IRQn,3);
    NVIC_SetPriority(TIMER3_IRQn,3);
    NVIC_ClearPendingIRQ(TIMER3_IRQn);
    NVIC_ClearPendingIRQ(TIMER1_IRQn);
    NVIC_EnableIRQ(TIMER3_IRQn);
    NVIC_EnableIRQ(TIMER1_IRQn);
    
    timerFreq=48000000;
    tc=new TimeConversion(timerFreq);
    //Start timers
    TIMER1->CMD = TIMER_CMD_START;
    //Synchronization is required only when timers are to start.
    //If the sync is not disabled after start, start/stop on another timer
    //(e.g. TIMER0) will affect the behavior of context switch timer!
    TIMER1->CTRL &= ~TIMER_CTRL_SYNC;
    TIMER2->CTRL &= ~TIMER_CTRL_SYNC;
    TIMER3->CTRL &= ~TIMER_CTRL_SYNC;
}

HighResolutionTimerBase::~HighResolutionTimerBase() {
    delete tc;
}

long long HighResolutionTimerBase::IRQgetCurrentTick(){
    return IRQgetTick();
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

// In this function I prepare the timer, but i don't enable the timer.
// Set true to get the input mode: wait for the raising of the pin and timestamp the time in which occurs
// Set false to get the output mode: When the time set is reached, raise the pin!
void HighResolutionTimerBase::setModeGPIOTimer(bool input){    
    if(input){
	//Connect TIMER1->CC2 to PIN PE12, meaning GPIO10 on wandstem
	TIMER1->ROUTE=TIMER_ROUTE_CC2PEN
                | TIMER_ROUTE_LOCATION_LOC1;
	//Configuro la modalità input
	TIMER1->CC[2].CTRL=TIMER_CC_CTRL_MODE_INPUTCAPTURE |
                          TIMER_CC_CTRL_ICEDGE_RISING |
                          TIMER_CC_CTRL_INSEL_PIN; 
	
	//Config PRS: Timer3 has to be a consumer, Timer1 a producer, TIMER3 keeps the most significative part
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
    // First off, clear and disable timers to prevent unnecessary IRQ
    // when IRQsetNextInterrupt is called multiple times consecutively.
    TIMER1->IEN &= ~TIMER_IEN_CC1;
    TIMER3->IEN &= ~TIMER_IEN_CC1;

    long long curTick = IRQgetTick();
    if(curTick >= tick)
    {
	// The interrupt is in the past => call timerInt immediately
	callScheduler(); //TODO: It could cause multiple invocations of sched.
	return false;
    }else{
	// Apply the new interrupt on to the timer channels
	TIMER1->CC[1].CCV = static_cast<unsigned int>(tick & 0xFFFF);
	TIMER3->CC[1].CCV = static_cast<unsigned int>((tick & 0xFFFF0000)>>16)-1;
	ms32chkp[1] = tick & upperMask;
	setupTimers();
	return true;
    }
}

bool HighResolutionTimerBase::IRQsetNextInterrupt2(long long tick){
    ms32chkp[2] = tick & upperMask;
    TIMER3->CC[2].CCV = static_cast<unsigned int>((tick & 0xFFFF0000)>>16)-1;
    TIMER1->CC[2].CCV = static_cast<unsigned int>(tick & 0xFFFF);
    /*
    In order to prevent back to back interrupts due to a checkpoint
    set in the past, raise lateIrq flag to indicate this IRQ should
    be processed only once
    */
    if(IRQgetCurrentTick() >= IRQgetSetTimeCCV2())
    {
	//TIMER3->IFS=TIMER_IFS_CC2;
        lateIrq=true;
	return false;
    }
    return true;
}


