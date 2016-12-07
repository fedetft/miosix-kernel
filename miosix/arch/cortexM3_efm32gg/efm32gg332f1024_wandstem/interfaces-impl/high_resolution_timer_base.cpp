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

#include "kernel/kernel.h"
#include "kernel/scheduler/timer_interrupt.h"
#include "high_resolution_timer_base.h"
#include "kernel/timeconversion.h"
#include "gpio_timer.h"
#include "transceiver_timer.h"
#include "rtc.h"

using namespace miosix;

const unsigned int timerBits=32;
const unsigned long long overflowIncrement=(1LL<<timerBits);
const unsigned long long lowerMask=overflowIncrement-1;
const unsigned long long upperMask=0xFFFFFFFFFFFFFFFFLL-lowerMask;

static long long ms32time = 0;		//most significant 32 bits of counter
// Position 0: transceiver
// Position 1: CStimer
// Position 2: GpioTimer
static long long ms32chkp[3] = {0,0,0}; //most significant 32/48 bits of checkpoint
static TimeConversion* tc;

static int faseGPIO=0;
static int faseTransceiver=0;

//static volatile unsigned long long vhtBaseVht=0;        ///< Vht time corresponding to rtc time: theoretical
//static unsigned long long syncPointRtc=0;     ///< Rtc time corresponding to vht time : known with sum
//static volatile unsigned long long syncPointVht=0;     ///< Vht time corresponding to rtc time :timestanped
//static unsigned long long syncVhtRtcPeriod=3000;

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
	TIMER1->CC[2].CCV = static_cast<unsigned short>(TIMER1->CNT+10);
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

void __attribute__((naked)) CMU_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z10cmuhandlerv");
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

template<typename C>
C divisionRounded(C a, C b){
    return (a+b/2)/b;
}

void __attribute__((used)) cstirqhnd2(){
    //CC0 listening for received packet --> input mode
    if ((TIMER2->IEN & TIMER_IEN_CC0) && (TIMER2->IF & TIMER_IF_CC0) ){
	TIMER2->IEN &= ~ TIMER_IEN_CC0;
	TIMER2->IFC = TIMER_IFC_CC0;
        //disable the timeout
	TIMER2->IEN &= ~ TIMER_IEN_CC1;
	TIMER2->IFC = TIMER_IFC_CC1;
        
	ms32chkp[0]=ms32time;
	//really in the past, the overflow of TIMER3 is occurred but the timer wasn't updated
	long long a=ms32chkp[0] | TIMER3->CC[0].CCV<<16 | TIMER2->CC[0].CCV;
	long long c=IRQgetTick();
	if(a-c< -48000000){ 
	    ms32chkp[0]+=overflowIncrement;
	}
	interruptTransceiverTimerRoutine();
    }
    
    //CC1 for output/trigger the sending packet event
    if ((TIMER2->IEN & TIMER_IEN_CC1) && (TIMER2->IF & TIMER_IF_CC1) ){
	TIMER2->IFC = TIMER_IFC_CC1;
	
        //if PEN, it means that we want to trigger, otherwise we are in timeout for input/capture
	if(TIMER2->ROUTE & TIMER_ROUTE_CC1PEN){
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
	}else{//We are in the INPUT mode, this case is to check the timeout
	    long long t=ms32chkp[0]|TIMER2->CC[1].CCV;
	    long long diff=t-IRQgetTick();
	    if(diff<0){
		TIMER2->IEN &= ~TIMER_IEN_CC1;
                //Disable the input capture interrupt
		TIMER2->IEN &= ~TIMER_IEN_CC0;
		TIMER2->IFC = TIMER_IFC_CC0;
		//Reactivating the thread that is waiting for the event, WITHOUT changing the tWaiting
		if(TransceiverTimer::tWaiting){
		    TransceiverTimer::tWaiting->IRQwakeup();
		    if(TransceiverTimer::tWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
			Scheduler::IRQfindNextThread();
		}
	    }
	}
    }
    
    if ((TIMER2->IEN & TIMER_IEN_CC2) && (TIMER2->IF & TIMER_IF_CC2) ){
        static bool ft=true;
	static int ex;
	TIMER2->IFC = TIMER_IFC_CC2;
	//HighResolutionTimerBase::tWaiting->IRQwakeup();
	//Scheduler::IRQfindNextThread();
	
	//"Actual" times 
	HighResolutionTimerBase::syncPointRtc += HighResolutionTimerBase::syncPeriodVhtRtc;
        HighResolutionTimerBase::diffs[0]=RTC->CNT;
        //FIXME
        //actual times
        HighResolutionTimerBase::syncPointHrtTimestamped = ms32time | (TIMER3->CNT << 16) | TIMER2->CC[2].CCV;
	//Assuming that this routine is executed within 1.3ms after the interrupt
	if(HighResolutionTimerBase::syncPointHrtTimestamped > IRQgetTick()){
	    HighResolutionTimerBase::syncPointHrtTimestamped = ms32time | ((TIMER3->CNT-1) << 16) | TIMER2->CC[2].CCV;
	}
        int conversion=divisionRounded(static_cast<unsigned long long>(HighResolutionTimerBase::syncPeriodVhtRtc*48000000),static_cast<unsigned long long>(32768));
        HighResolutionTimerBase::syncPointHrtExpected += conversion + HighResolutionTimerBase::clockCorrection;
        
	//required to make a coarse estimation of error and avoid overflow in flopsync algorithm
	if(ft){
	    ft=false;
	    ex=HighResolutionTimerBase::syncPointHrtTimestamped - (HighResolutionTimerBase::syncPointHrtExpected);
	}
        HighResolutionTimerBase::error = HighResolutionTimerBase::syncPointHrtTimestamped-ex - (HighResolutionTimerBase::syncPointHrtExpected);
        redLed::toggle();
	
	
	HighResolutionTimerBase::tWaiting->IRQwakeup();
	Scheduler::IRQfindNextThread();
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
    
    //This if is used to manage the case of GPIOTimer, both INPUT and OUTPUT mode, in INPUT it goes directly in the "else branch"
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
	    TIMER1->IEN &= ~TIMER_IEN_CC0;
	    TIMER1->IFC = TIMER_IFC_CC0;
	    interruptGPIOTimerRoutine();
	}
    }
    
    if ((TIMER1->IEN & TIMER_IEN_CC0) && (TIMER1->IF & TIMER_IF_CC0)){
	TIMER1->IFC = TIMER_IFC_CC0;
	long long t=ms32chkp[2]|TIMER1->CC[0].CCV;
	long long diff=t-IRQgetTick();
	if(diff<0){
	    TIMER1->IEN &= ~TIMER_IEN_CC0;
	    TIMER1->IEN &= ~TIMER_IEN_CC2;
	    TIMER1->IFC = TIMER_IFC_CC2;
	    //Reactivating the thread that is waiting for the event, WITHOUT changing the tWaiting
	    if(GPIOtimer::tWaiting){
		GPIOtimer::tWaiting->IRQwakeup();
		if(GPIOtimer::tWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		    Scheduler::IRQfindNextThread();
	    }
	}
    }
}

void __attribute__((used)) cmuhandler(){
    static float y;
    static bool first=true;
    if(CMU->IF & CMU_IF_CALRDY){
	if(first){
	    y=CMU->CALCNT;
	    first=false;
	}else{
	    y=0.8f*y+0.2f*CMU->CALCNT;
	}
        CMU->IFC=CMU_IFC_CALRDY;
	bool hppw;
	HighResolutionTimerBase::queue.IRQpost([&](){printf("%lu\n",CMU->CALCNT);},hppw);
	if(hppw) Scheduler::IRQfindNextThread();
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

void HighResolutionTimerBase::enableCC0InterruptTim1(bool enable){
    if(enable){
	TIMER1->IFC= TIMER_IF_CC0;
        TIMER1->IEN|=TIMER_IEN_CC0;
    }else
        TIMER1->IEN&=~TIMER_IEN_CC0;
}

void HighResolutionTimerBase::enableCC2InterruptTim1(bool enable){
    if(enable){
	TIMER1->IFC= TIMER_IF_CC2;
        TIMER1->IEN|=TIMER_IEN_CC2;
    }else
        TIMER1->IEN&=~TIMER_IEN_CC2;
}

void HighResolutionTimerBase::enableCC0InterruptTim2(bool enable){
    if(enable){
        TIMER2->IFC= TIMER_IF_CC0;
        TIMER2->IEN|=TIMER_IEN_CC0;
    }else
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

// In this function I prepare the timer, but i don't enable it.
// Set true to get the input mode: wait for the raising of the pin and timestamp the time in which occurs
// Set false to get the output mode: When the time set is reached, raise the pin!
void HighResolutionTimerBase::setModeGPIOTimer(bool input){    
    //Connect TIMER1->CC2 to PIN PE12, meaning GPIO10 on wandstem
    TIMER1->ROUTE=TIMER_ROUTE_CC2PEN
	    | TIMER_ROUTE_LOCATION_LOC1;
    if(input){
	//Configuro la modalità input
	//The consumer are both timers
	TIMER1->CC[2].CTRL = TIMER_CC_CTRL_PRSSEL_PRSCH0
			|   TIMER_CC_CTRL_INSEL_PRS
			|   TIMER_CC_CTRL_ICEDGE_RISING  //NOTE: when does the output get low?
			|   TIMER_CC_CTRL_MODE_INPUTCAPTURE;
	TIMER3->CC[2].CTRL= TIMER_CC_CTRL_PRSSEL_PRSCH0
			|   TIMER_CC_CTRL_INSEL_PRS
			|   TIMER_CC_CTRL_ICEDGE_RISING  //NOTE: when does the output get low?
			|   TIMER_CC_CTRL_MODE_INPUTCAPTURE;
	
	// The producer is the PGIO12
	PRS->CH[0].CTRL = PRS_CH_CTRL_SOURCESEL_GPIOH
			|   PRS_CH_CTRL_SIGSEL_GPIOPIN12;
	//Configured for timeout
	TIMER1->CC[0].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
	//fase GPIO is required in OUTPUT Mode, here we have to set to 1
	faseGPIO=1;
    }else{
        TIMER1->CC[2].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
        TIMER3->CC[2].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE; 
    } 
}

void HighResolutionTimerBase::setModeTransceiverTimer(bool input){
    if(input){	
        //For input capture feature:
	//Connect TIMER2->CC0 to pin PA8 aka excChB
//	TIMER2->ROUTE |= TIMER_ROUTE_CC0PEN
//		| TIMER_ROUTE_LOCATION_LOC0;	
	
	//Config PRS: Timer3 has to be a consumer, Timer2 a producer, TIMER3 keeps the most significative part
	//TIMER2->CC0 as producer, i have to specify the event i'm interest in    
	PRS->CH[1].CTRL|= PRS_CH_CTRL_SOURCESEL_GPIOH
			|   PRS_CH_CTRL_SIGSEL_GPIOPIN8;

	//TIMER3->CC2 as consumer
	TIMER3->CC[0].CTRL=TIMER_CC_CTRL_PRSSEL_PRSCH1
			|   TIMER_CC_CTRL_INSEL_PRS
			|   TIMER_CC_CTRL_ICEDGE_RISING
			|   TIMER_CC_CTRL_MODE_INPUTCAPTURE;
	TIMER2->CC[0].CTRL=TIMER_CC_CTRL_PRSSEL_PRSCH1
			|   TIMER_CC_CTRL_INSEL_PRS
			|   TIMER_CC_CTRL_ICEDGE_RISING
			|   TIMER_CC_CTRL_MODE_INPUTCAPTURE;
        
        TIMER2->CC[1].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
        TIMER2->ROUTE &= ~TIMER_ROUTE_CC1PEN; //used as timeout the incoming event
    }else{
	//For output capture feature:
	//Connect TIMER2->CC1 to pin PA9 aka stxon
        TIMER2->ROUTE |= TIMER_ROUTE_CC1PEN; //used to trigger at a give time on the stxon
	TIMER2->CC[1].CTRL = TIMER_CC_CTRL_MODE_OUTPUTCOMPARE;
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

WaitResult HighResolutionTimerBase::IRQsetGPIOtimeout(long long tick){
    long long curTick = IRQgetTick(); // This require almost 1us about 50ticks
    long long diff=tick-curTick;
    tick--; //to trigger at the RIGHT time!
    
    // 150 are enough to make sure that this routine ends and the timer IEN is enabled. 
    //NOTE: this is really dependent on compiler, optimization and other stuff
    if(diff>200){
	unsigned short t1=static_cast<unsigned short>(tick & 0xFFFF);
	//ms32chkp[2] is going to store even the middle part, because we don't need to use TIMER3
	ms32chkp[2] = tick & (upperMask | 0xFFFF0000);
	TIMER1->CC[0].CCV = t1;

	enableCC0InterruptTim1(true);
	return WaitResult::WAITING;
    }else{
	return WaitResult::WAKEUP_IN_THE_PAST;
    }
}

WaitResult HighResolutionTimerBase::IRQsetTransceiverTimeout(long long tick){
    long long curTick = IRQgetTick(); // This require almost 1us about 50ticks
    long long diff=tick-curTick;
    tick--;
    
    // 150 are enough to make sure that this routine ends and the timer IEN is enabled. 
    //NOTE: this is really dependent on compiler, optimization and other stuff
    if(diff>200){
	unsigned short t1=static_cast<unsigned short>(tick & 0xFFFF);
	//ms32chkp[0] is going to store even the middle part, because we don't need to use TIMER3
	ms32chkp[0] = tick & (upperMask | 0xFFFF0000);
	TIMER2->CC[1].CCV = t1;

	enableCC1InterruptTim2(true);
	return WaitResult::WAITING;
    }else{
	return WaitResult::WAKEUP_IN_THE_PAST;
    }
} 

void HighResolutionTimerBase::resyncVht(){
    /*Rtc& rtc=Rtc::instance();
    long long rtcTime;
    int a=0,b=0,c=0,d=0,e=0;
    {
	FastInterruptDisableLock dLock;
	int prev=loopback32KHzIn::value();
	for(;;){
		int curr=loopback32KHzIn::value();
		if(curr-prev==-1) break;
		prev=curr;
	}
	int high=TIMER3->CNT;

	TIMER2->IFC = TIMER_IFC_CC2;
	TIMER2->CC[2].CTRL |= TIMER_CC_CTRL_MODE_INPUTCAPTURE;
	
	//Wait until we capture the first raising edge in the 32khz wave, 
	//it takes max half period, less than 16us -->732.4ticks @48000000Hz
	while((TIMER2->IF & TIMER_IF_CC2)==0) ;
	
	//Creating the proper value of vhtSyncPointVht
	int high2=TIMER3->CNT;
	//Turn off the input capture mode
	TIMER2->CC[2].CTRL &= ~_TIMER_CC_CTRL_MODE_MASK;
	//This is the exact point when the rtc is incremented to the actual value 
	if(high==high2){
	    vhtSyncPointVht = ms32time | high<<16 | TIMER2->CC[2].CCV; //FIXME:: add the highest part
	}else{
	    // 735 are really enough to be sure that we are in the new window
	    if(TIMER2->CC[2].CCV < 735){
		if(high==0xFFFF){
		    vhtSyncPointVht = (ms32time+overflowIncrement) | high2<<16 | TIMER2->CC[2].CCV;
		}else{
		    vhtSyncPointVht = ms32time | high2<<16 | TIMER2->CC[2].CCV;
		}
	    }else{
		vhtSyncPointVht = ms32time | high<<16 | TIMER2->CC[2].CCV;
	    }
	}
	
	vhtSyncPointRtc=rtc.IRQgetValue();
	{
            unsigned long long conversion=vhtSyncPointRtc;
            conversion*=HighResolutionTimerBase::freq;
            conversion+=HighResolutionTimerBase::freq/2; //Round to nearest
            conversion/=HighResolutionTimerBase::freq;
            vhtBase=conversion;
        }
    }
    printf("a=%d b=%d c=%d d=%d e=%d e-d=%d\n",a,b,c,d,e,e>=d ? e-d : e+65536-d);*/
}

void HighResolutionTimerBase::setAutoResyncVht(bool enable){
    if(enable){
	
    }else{
	
    }
}

void HighResolutionTimerBase::resyncClock(){
    CMU->CMD = CMU_CMD_CALSTART;
}
        
void HighResolutionTimerBase::setAutoAndStartResyncClocks(bool enable){
    if(enable){
	CMU->CTRL |= CMU_CALCTRL_CONT;
	CMU->CMD = CMU_CMD_CALSTART;
    }else{
	CMU->CTRL &= ~CMU_CALCTRL_CONT;
	CMU->CMD = CMU_CMD_CALSTOP;
    }
}

void HighResolutionTimerBase::initResyncCmu(){
    CMU->CALCTRL=CMU_CALCTRL_DOWNSEL_LFXO|CMU_CALCTRL_UPSEL_HFXO|CMU_CALCTRL_CONT;
    //due to hardware timer characteristic, the real counter trigger at value+1
    //tick of LFCO to yield the maximum from to up counter
    CMU->CALCNT=700; 
    //enable interrupt
    CMU->IEN=CMU_IEN_CALRDY;
    NVIC_SetPriority(CMU_IRQn,3);
    NVIC_ClearPendingIRQ(CMU_IRQn);
    NVIC_EnableIRQ(CMU_IRQn);
}

HighResolutionTimerBase& HighResolutionTimerBase::instance(){
    static HighResolutionTimerBase hrtb;
    return hrtb;
}

const unsigned int HighResolutionTimerBase::freq=48000000;
FixedEventQueue<100,12> HighResolutionTimerBase::queue;

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
    
    tc=new TimeConversion(HighResolutionTimerBase::freq);
    //Start timers
    TIMER1->CMD = TIMER_CMD_START;
    //Synchronization is required only when timers are to start.
    //If the sync is not disabled after start, start/stop on another timer
    //(e.g. TIMER0) will affect the behavior of context switch timer!
    TIMER1->CTRL &= ~TIMER_CTRL_SYNC;
    TIMER2->CTRL &= ~TIMER_CTRL_SYNC;
    TIMER3->CTRL &= ~TIMER_CTRL_SYNC;
    
    //Virtual high resolution timer, init without starting the input mode!
    TIMER2->CC[2].CTRL=TIMER_CC_CTRL_ICEDGE_RISING
			| TIMER_CC_CTRL_FILT_DISABLE
			| TIMER_CC_CTRL_INSEL_PIN
			| TIMER_CC_CTRL_MODE_OFF;
    
    initResyncCmu();
}

HighResolutionTimerBase::~HighResolutionTimerBase() {
    delete tc;
}

long long HighResolutionTimerBase::aux1=0;
long long HighResolutionTimerBase::aux2=0;
long long HighResolutionTimerBase::aux3=0;
long long HighResolutionTimerBase::aux4=0;
long long HighResolutionTimerBase::error=0;
unsigned long long HighResolutionTimerBase::syncPeriodVhtRtc=3000;
long long HighResolutionTimerBase::syncPointHrtExpected=0;
long long HighResolutionTimerBase::syncPointHrtTimestamped=0;
long long HighResolutionTimerBase::clockCorrection=0;
long long HighResolutionTimerBase::syncPointRtc=0;
long long HighResolutionTimerBase::diffs[100]={0};
Thread* HighResolutionTimerBase::tWaiting=nullptr;