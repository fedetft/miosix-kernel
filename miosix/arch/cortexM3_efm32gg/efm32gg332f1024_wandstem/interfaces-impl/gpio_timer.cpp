/***************************************************************************
 *   Copyright (C) 2016 by Fabiano Riccardi                                *
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

#include "gpio_timer.h"
#include "hwmapping.h"
#include "miosix.h"
#include <cstdlib>
#include <cstdio>
#include "../../../../debugpin.h"

using namespace miosix;

Thread* GPIOtimer::tWaiting=nullptr;

long long GPIOtimer::getValue() const{
    return b.getCurrentTick();
}

unsigned int GPIOtimer::getTickFrequency() const{
    return b.getTimerFrequency();
}

void GPIOtimer::wait(long long tick){
    Thread::nanoSleep(tc.tick2ns(tick));
}

bool GPIOtimer::absoluteWait(long long tick){
    if(b.getCurrentTick()>=tick){
	return true;
    }
    Thread::nanoSleepUntil(tc.tick2ns(tick));
    return false;
}

//NOTE: Think about how to set the right ms32chkp related to the captured timestamp
long long GPIOtimer::getExtEventTimestamp() const{
    return b.IRQgetSetTimeGPIO() - stabilizingTime;
}

bool GPIOtimer::absoluteWaitTimeoutOrEvent(long long tick){
    FastInterruptDisableLock dLock;
    if(tick<b.getCurrentTick()){
	return true;
    }
    if(!isInput){
	b.setModeGPIOTimer(true);
	expansion::gpio10::mode(Mode::INPUT);
	isInput=true;
    }
    b.cleanBufferGPIO();
    b.enableCC2Interrupt(false);
    b.enableCC2InterruptTim1(true);
    
    do {
        tWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
	    Thread::yield();
        }
    } while(tWaiting && tick>b.getCurrentTick());
    
    if(tWaiting==nullptr){
	return false;
    }else{
	return true;
    }
}

bool GPIOtimer::waitTimeoutOrEvent(long long tick){
    return absoluteWaitTimeoutOrEvent(b.getCurrentTick()+tick);
}

/*
 Not blocking function!
 */
bool GPIOtimer::absoluteWaitTrigger(long long tick){
    FastInterruptDisableLock dLock;
    if(isInput){
	b.setModeGPIOTimer(false);		//output timer 
	expansion::gpio10::mode(Mode::OUTPUT);	//output pin
	expansion::gpio10::low();
	isInput=false;
    }
    if(b.IRQsetNextGPIOInterrupt(tick)==WaitResult::WAKEUP_IN_THE_PAST){
	return true;
    }
    return false;
}

bool GPIOtimer::waitTrigger(long long tick){
    return absoluteWaitTrigger(b.getCurrentTick()+tick);
}

/*
 * This takes about 5us to be executed
 */
bool GPIOtimer::absoluteSyncWaitTrigger(long long tick){
    {
	FastInterruptDisableLock dLock;
	if(isInput){
	    b.setModeGPIOTimer(false);			//output timer 
	    expansion::gpio10::mode(Mode::OUTPUT);	//output pin
	    expansion::gpio10::low();
	    isInput=false;
	}
	if(b.IRQsetNextGPIOInterrupt(tick)==WaitResult::WAKEUP_IN_THE_PAST){
	    return true;
	}
	do {
	    tWaiting=Thread::IRQgetCurrentThread();
	    Thread::IRQwait();
	    {
		FastInterruptEnableLock eLock(dLock);
		Thread::yield();
	    }
	} while(tWaiting && tick>b.getCurrentTick());
    }
    return false;
}

bool GPIOtimer::syncWaitTrigger(long long tick){
    return absoluteSyncWaitTrigger(b.getCurrentTick()+tick); 
}

long long GPIOtimer::tick2ns(long long tick){
    return tc.tick2ns(tick);
}

long long GPIOtimer::ns2tick(long long ns){
    return tc.ns2tick(ns);
}

GPIOtimer::~GPIOtimer() {}

GPIOtimer& GPIOtimer::instance(){
    static GPIOtimer instance;
    return instance;
}

const int GPIOtimer::stabilizingTime = 4;

GPIOtimer::GPIOtimer(): b(HighResolutionTimerBase::instance()),tc(b.getTimerFrequency()) {
    b.setModeGPIOTimer(true);
    expansion::gpio10::mode(Mode::INPUT);
    isInput=true;
}