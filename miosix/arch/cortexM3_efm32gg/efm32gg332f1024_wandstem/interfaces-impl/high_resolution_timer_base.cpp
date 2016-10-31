/***************************************************************************
 *   Copyright (C) 2016 by Fabiano Riccardi, Sasan                         *
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

#include "interfaces/arch_registers.h"
#include "kernel/kernel.h"
#include "kernel/scheduler/timer_interrupt.h"
#include "high_resolution_timer_base.h"
#include "kernel/timeconversion.h"
#include "gpio_timer.h"
#include "transceiver_timer.h"
#include "../../../../debugpin.h"

using namespace miosix;

const unsigned int timerBits=32;
const unsigned long long overflowIncrement=(1LL<<timerBits);
const unsigned long long lowerMask=overflowIncrement-1;
const unsigned long long upperMask=0xFFFFFFFFFFFFFFFFLL-lowerMask;

static long long ms32time = 0;		//most significant 32 bits of counter
static long long ms32chkp[3] = {0,0,0}; //most significant 32 bits of check point
static TimeConversion* tc;

static int faseGPIO=0;
static int faseTransceiver=0;

static inline unsigned int IRQread32Timer(){
    unsigned int high=TIMER3->CNT;
    unsigned int low=TIMER1->CNT;
    unsigned int high2=TIMER3->CNT;
    if(high==high2){
        return (high<<16)|low;
    }
    return high2<<16|TIMER1->CNT;
}

/*
 This takes almost 50ticks=1us
 */
static inline long long IRQgetTick(){
    //PENDING BIT TRICK
    unsigned int counter=IRQread32Timer();
    if((TIMER3->IF & _TIMER_IFC_OF_MASK) && IRQread32Timer()>=counter)
        return (ms32time | static_cast<long long>(counter)) + overflowIncrement;
    return ms32time | static_cast<long long>(counter);
}

void falseRead(volatile uint32_t *p){
    *p;
}

inline void interruptGPIOTimerRoutine(){
    if(TIMER1->CC[2].CTRL & TIMER_CC_CTRL_MODE_OUTPUTCOMPARE){
	TIMER1->CC[2].CTRL = (TIMER1->CC[2].CTRL & ~_TIMER_CC_CTRL_CMOA_MASK) | TIMER_CC_CTRL_CMOA_CLEAR;
	//10 tick are enough to execute this line
	TIMER1->CC[2].CCV = static_cast<unsigned short>(TIMER1->CNT+10);//static_cast<unsigned int>(tick & 0xFFFF);
    }else if(TIMER1->CC[2].CTRL & TIMER_CC_CTRL_MODE_INPUTCAPTURE){
	ms32chkp[2]=ms32time;
	//really in the past, the overflow of TIMER3 is occurred but the timer wasn't updated
	long long a=ms32chkp[2] | TIMER3->CC[2].CCV<<16 | TIMER1->CC[2].CCV;;
	long long c=IRQgetTick();
	if(a-c< -48000000){ 
	    ms32chkp[2]+=overflowIncrement;
	}
    }
    //Reactivating the thread that is waiting for the event.
    if(GPIOtimer::tWaiting){
	GPIOtimer::tWaiting->IRQwakeup();
	if(GPIOtimer::tWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
	    Scheduler::IRQfindNextThread();
	GPIOtimer::tWaiting=nullptr;
    }
    
}

/*
 * FIXME: The following codition could be always true, they aren't exclusive
 */
inline void interruptTransceiverTimerRoutine(){
    //Reactivating the thread that is waiting for the event.
    if(TransceiverTimer::tWaiting){
	TransceiverTimer::tWaiting->IRQwakeup();
	if(TransceiverTimer::tWaiting->IRQgetPriority() > Thread::IRQgetCurrentThread()->IRQgetPriority())
	    Scheduler::IRQfindNextThread();
	TransceiverTimer::tWaiting=nullptr;
    }
}

static  void callScheduler(){
    TIMER1->IEN &= ~TIMER_IEN_CC1;
    TIMER3->IEN &= ~TIMER_IEN_CC1;
    TIMER1->IFC = TIMER_IFC_CC1;
    TIMER3->IFC = TIMER_IFC_CC1;
    long long tick = tc->tick2ns(IRQgetTick());
    IRQtimerInterrupt(tick);
}

static void setupTimers(){
    // We assume that this function is called only when the checkpoint is in future
    if (ms32chkp[1] == ms32time){
        // If the most significant 32bit matches, enable TIM3
        TIMER3->IFC = TIMER_IFC_CC1;
        TIMER3->IEN |= TIMER_IEN_CC1;
	unsigned short temp=static_cast<unsigned short>(TIMER3->CC[1].CCV) + 1;
        if (static_cast<unsigned short>(TIMER3->CNT) >= temp){
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

void __attribute__((naked)) TIMER2_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z10cstirqhnd2v");
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
	unsigned short temp=static_cast<unsigned short>(TIMER3->CC[1].CCV) + 1;
        if (static_cast<unsigned short>(TIMER3->CNT) >= temp){
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
}


void __attribute__((used)) cstirqhnd2(){
    //CC0 listening for received packet --> input mode
    if ((TIMER2->IEN & TIMER_IEN_CC0) && (TIMER2->IF & TIMER_IF_CC0) ){
	TIMER2->IEN &= ~ TIMER_IEN_CC0;
	TIMER2->IFC = TIMER_IFC_CC0;
	
	ms32chkp[0]=ms32time;
	//really in the past, the overflow of TIMER3 is occurred but the timer wasn't updated
	long long a=ms32chkp[0] | TIMER3->CC[0].CCV<<16 | TIMER2->CC[0].CCV;;
	long long c=IRQgetTick();
	if(a-c< -48000000){ 
	    ms32chkp[0]+=overflowIncrement;
	}
	interruptTransceiverTimerRoutine();
    }
    
    //CC1 for output/trigger the sending packet event
    if ((TIMER2->IEN & TIMER_IEN_CC1) && (TIMER2->IF & TIMER_IF_CC1) ){
	TIMER2->IFC = TIMER_IFC_CC1;
	if(faseTransceiver==0){
	    //get nextInterrupt
	    long long t=ms32chkp[0]|TIMER2->CC[1].CCV;
	    long long diff=t-IRQgetTick();
	    if(diff<=0xFFFF){
		TIMER2->CC[1].CTRL = (TIMER2->CC[1].CTRL & ~_TIMER_CC_CTRL_CMOA_MASK) | TIMER_CC_CTRL_CMOA_SET;
		faseTransceiver=1;
	    }
	}else{
	    TIMER2->IEN &= ~TIMER_IEN_CC1;
	    TIMER2->CC[1].CTRL = (TIMER2->CC[1].CTRL & ~_TIMER_CC_CTRL_CMOA_MASK) | TIMER_CC_CTRL_CMOA_CLEAR;
	    TIMER2->CC[1].CCV = static_cast<unsigned short>(TIMER2->CNT+10);
	    interruptTransceiverTimerRoutine();
	}
    }
}

/*
 * This takes about 2.5us to execute, so the Pin can't stay high for less than this value, 
 * and we can't set more interrupts in a period of 2.5us+
 */
void __attribute__((used)) cstirqhnd1(){
    if ((TIMER1->IEN & TIMER_IEN_CC1) && (TIMER1->IF & TIMER_IF_CC1)){
        TIMER1->IFC = TIMER_IFC_CC1;
	callScheduler();
    }
    
    //This if is used to manage the case of GPIOTimer, both INPUT and OUTPUT mode, in INPUT it goes direclty in the "else branch"
    if ((TIMER1->IEN & TIMER_IEN_CC2) && (TIMER1->IF & TIMER_IF_CC2)){
        TIMER1->IFC = TIMER_IFC_CC2;
	if(faseGPIO==0){
	    //get nextInterrupt
	    long long t=ms32chkp[2]|TIMER1->CC[2].CCV;
	    long long diff=t-IRQgetTick();
	    if(diff<=0xFFFF){
		TIMER1->CC[2].CTRL = (TIMER1->CC[2].CTRL & ~_TIMER_CC_CTRL_CMOA_MASK) | TIMER_CC_CTRL_CMOA_SET;
		faseGPIO=1;
	    }
	}else{
	    TIMER1->IEN &= ~TIMER_IEN_CC2;
	    interruptGPIOTimerRoutine();
	}
    }
}

long long HighResolutionTimerBase::IRQgetSetTimeTransceiver() const{
    return ms32chkp[0] | TIMER3->CC[0].CCV<<16 | TIMER2->CC[0].CCV;
}
long long HighResolutionTimerBase::IRQgetSetTimeCS() const{
    return ms32chkp[1] | TIMER3->CC[1].CCV<<16 | TIMER1->CC[1].CCV;
}
long long HighResolutionTimerBase::IRQgetSetTimeGPIO() const{
    return ms32chkp[2] | TIMER3->CC[2].CCV<<16 | TIMER1->CC[2].CCV;
}

long long HighResolutionTimerBase::IRQgetCurrentTick(){
    return IRQgetTick();
}

void HighResolutionTimerBase::enableCC0Interrupt(bool enable){
    if(enable){
        TIMER3->IEN|=TIMER_IEN_CC0;
    }else{
        TIMER3->IEN&=~TIMER_IEN_CC0;
    }
}
void HighResolutionTimerBase::enableCC1Interrupt(bool enable){
    if(enable)
        TIMER3->IEN|=TIMER_IEN_CC1;
    else
        TIMER3->IEN&=~TIMER_IEN_CC1;
}
void HighResolutionTimerBase::enableCC2Interrupt(bool enable){
    if(enable)
        TIMER3->IEN|=TIMER_IEN_CC2;
    else
        TIMER3->IEN&=~TIMER_IEN_CC2;
}

void HighResolutionTimerBase::enableCC2InterruptTim1(bool enable){
    if(enable){
	TIMER1->IFC= TIMER_IF_CC2;
        TIMER1->IEN|=TIMER_IEN_CC2;
    }else
        TIMER1->IEN&=~TIMER_IEN_CC2;
}

void HighResolutionTimerBase::enableCC0InterruptTim2(bool enable){
    if(enable)
        TIMER2->IEN|=TIMER_IEN_CC0;
    else
        TIMER2->IEN&=~TIMER_IEN_CC0;
}

void HighResolutionTimerBase::enableCC1InterruptTim2(bool enable){
    if(enable){
	TIMER2->IFC= TIMER_IF_CC1;
        TIMER2->IEN|=TIMER_IEN_CC1;
    }else
        TIMER2->IEN&=~TIMER_IEN_CC1;
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

WaitResult HighResolutionTimerBase::IRQsetNextTransceiverInterrupt(long long tick){
    long long curTick = IRQgetTick(); // This require almost 1us about 50ticks
    long long diff=tick-curTick;
    tick--;
    // 150 are enough to make sure that this routine ends and the timer IEN is enabled. 
    //NOTE: this is really dependent on compiler, optimization and other stuff
    if(diff>200){
	faseTransceiver=0;
	unsigned short t1=static_cast<unsigned short>(tick & 0xFFFF);
	//ms32chkp[0] is going to store even the middle part, because we don't need to use TIMER3
	ms32chkp[0] = tick & (upperMask | 0xFFFF0000);
	TIMER2->CC[1].CCV = t1;

	enableCC1InterruptTim2(true);
	diff=tick-IRQgetTick();
	//0xFFFF because it's the roundtrip of timer
	if(diff<=0xFFFF){
	    TIMER2->CC[1].CTRL = (TIMER2->CC[1].CTRL & ~_TIMER_CC_CTRL_CMOA_MASK) | TIMER_CC_CTRL_CMOA_SET;
	    faseTransceiver=1; //if phase=1, this means that we have to shutdown the pin next time that TIMER1 triggers
	}
	return WaitResult::WAITING;
    }else{
	return WaitResult::WAKEUP_IN_THE_PAST;
    }
} 

void HighResolutionTimerBase::IRQsetNextInterruptCS(long long tick){
    // First off, disable timers to prevent unnecessary IRQ
    // when IRQsetNextInterrupt is called multiple times consecutively.
    TIMER1->IEN &= ~TIMER_IEN_CC1;
    TIMER3->IEN &= ~TIMER_IEN_CC1;

    long long curTick = IRQgetTick();
    if(curTick >= tick){
	// The interrupt is in the past => call timerInt immediately
	callScheduler(); //TODO: It could cause multiple invocations of sched.
    }else{
	// Apply the new interrupt on to the timer channels
	TIMER1->CC[1].CCV = static_cast<unsigned int>(tick & 0xFFFF);
	TIMER3->CC[1].CCV = static_cast<unsigned int>((tick & 0xFFFF0000)>>16)-1;
	ms32chkp[1] = tick & upperMask;
	setupTimers();
    }
}

/*
 * Return true if the pin is going to raise, otherwise false, the pin remain low because the command arrived too late
 * Should be called at least 4us before the next interrupt, otherwise it returns with WAKEUP_IN_THE_PAST
 */
WaitResult HighResolutionTimerBase::IRQsetNextGPIOInterrupt(long long tick){
    long long curTick = IRQgetTick(); // This require almost 1us about 50ticks
    long long diff=tick-curTick;
    tick--; //to trigger at the RIGHT time!
    
    // 150 are enough to make sure that this routine ends and the timer IEN is enabled. 
    //NOTE: this is really dependent on compiler, optimization and other stuff
    if(diff>200){
	faseGPIO=0;
	unsigned short t1=static_cast<unsigned short>(tick & 0xFFFF);
	//ms32chkp[2] is going to store even the middle part, because we don't need to use TIMER3
	ms32chkp[2] = tick & (upperMask | 0xFFFF0000);
	TIMER1->CC[2].CCV = t1;

	enableCC2InterruptTim1(true);
	diff=tick-IRQgetTick();
	//0xFFFF because it's the roundtrip of timer
	if(diff<=0xFFFF){
	    TIMER1->CC[2].CTRL = (TIMER1->CC[2].CTRL & ~_TIMER_CC_CTRL_CMOA_MASK) | TIMER_CC_CTRL_CMOA_SET;
	    faseGPIO=1; //if phase=1, this means that we have to shutdown the pin next time that TIMER1 triggers
	}
	return WaitResult::WAITING;
    }else{
	return WaitResult::WAKEUP_IN_THE_PAST;
    }
}

// In this function I prepare the timer, but i don't enable the timer.
// Set true to get the input mode: wait for the raising of the pin and timestamp the time in which occurs
// Set false to get the output mode: When the time set is reached, raise the pin!
void HighResolutionTimerBase::setModeGPIOTimer(bool input){    
    //Connect TIMER1->CC2 to PIN PE12, meaning GPIO10 on wandstem
    TIMER1->ROUTE=TIMER_ROUTE_CC2PEN
	    | TIMER_ROUTE_LOCATION_LOC1;
    if(input){
	//Configuro la modalità input
	TIMER1->CC[2].CTRL = TIMER_CC_CTRL_MODE_INPUTCAPTURE |
			  TIMER_CC_CTRL_ICEDGE_RISING |
                          TIMER_CC_CTRL_INSEL_PIN; 
	
	//Config PRS: Timer3 has to be a consumer, Timer1 a producer, TIMER3 keeps the most significative part
	//TIMER1->CC2 as producer, i have to specify the event i'm interest in    
	PRS->CH[0].CTRL|=PRS_CH_CTRL_SOURCESEL_TIMER1
			|PRS_CH_CTRL_SIGSEL_TIMER1CC2;

	//TIMER3->CC2 as consumer
	TIMER3->CC[2].CTRL= TIMER_CC_CTRL_PRSSEL_PRSCH0
			|   TIMER_CC_CTRL_INSEL_PRS
			|   TIMER_CC_CTRL_ICEDGE_RISING  //NOTE: when does the output get low?
			|   TIMER_CC_CTRL_MODE_INPUTCAPTURE;
	faseGPIO=1;
    }else{
        TIMER1->CC[2].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
        TIMER3->CC[2].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE; 
    } 
}

void HighResolutionTimerBase::cleanBufferGPIO(){
    falseRead(&TIMER3->CC[2].CCV);
    falseRead(&TIMER1->CC[2].CCV);
    falseRead(&TIMER3->CC[2].CCV);
    falseRead(&TIMER1->CC[2].CCV);
}

void HighResolutionTimerBase::cleanBufferTrasceiver(){
    falseRead(&TIMER3->CC[0].CCV);
    falseRead(&TIMER2->CC[0].CCV);
    falseRead(&TIMER3->CC[0].CCV);
    falseRead(&TIMER2->CC[0].CCV);
}

void HighResolutionTimerBase::setModeTransceiverTimer(){
    //For input capture feature:
	//Connect TIMER2->CC0 to pin PA8
	TIMER2->ROUTE |= TIMER_ROUTE_CC0PEN
		| TIMER_ROUTE_LOCATION_LOC0;	
	//Gpio<GPIOA_BASE,8>  excChB;
	//Configuro la modalità input
	TIMER2->CC[0].CTRL = TIMER_CC_CTRL_MODE_INPUTCAPTURE |
			  TIMER_CC_CTRL_ICEDGE_RISING |
                          TIMER_CC_CTRL_INSEL_PIN; 
	
	//Config PRS: Timer3 has to be a consumer, Timer2 a producer, TIMER3 keeps the most significative part
	//TIMER2->CC0 as producer, i have to specify the event i'm interest in    
	PRS->CH[1].CTRL|= PRS_CH_CTRL_SOURCESEL_TIMER2
			| PRS_CH_CTRL_SIGSEL_TIMER2CC0;

	//TIMER3->CC2 as consumer
	TIMER3->CC[0].CTRL=TIMER_CC_CTRL_PRSSEL_PRSCH1
			|   TIMER_CC_CTRL_INSEL_PRS
			|   TIMER_CC_CTRL_ICEDGE_RISING
			|   TIMER_CC_CTRL_MODE_INPUTCAPTURE;
    
    //For output capture feature // Gpio<GPIOA_BASE,9>  stxon
	//Connect TIMER2->CC1 to pin PA9
	TIMER2->ROUTE |= TIMER_ROUTE_CC1PEN
		| TIMER_ROUTE_LOCATION_LOC0;
	TIMER2->CC[1].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
        TIMER3->CC[1].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE; 
}

HighResolutionTimerBase& HighResolutionTimerBase::instance(){
    static HighResolutionTimerBase hrtb;
    return hrtb;
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
    NVIC_SetPriority(TIMER2_IRQn,3);
    NVIC_SetPriority(TIMER3_IRQn,3);
    NVIC_ClearPendingIRQ(TIMER1_IRQn);
    NVIC_ClearPendingIRQ(TIMER2_IRQn);
    NVIC_ClearPendingIRQ(TIMER3_IRQn);
    NVIC_EnableIRQ(TIMER1_IRQn);
    NVIC_EnableIRQ(TIMER2_IRQn);
    NVIC_EnableIRQ(TIMER3_IRQn);
    
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