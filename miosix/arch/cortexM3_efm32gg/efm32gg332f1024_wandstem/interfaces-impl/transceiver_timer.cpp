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

#include "transceiver_timer.h"

using namespace miosix;

Thread* TransceiverTimer::tWaiting=nullptr;

long long TransceiverTimer::getValue() const{
     return b.getCurrentTick();
 }  
            
void TransceiverTimer::wait(long long tick){
    Thread::nanoSleep(tc.tick2ns(tick));
}

bool TransceiverTimer::absoluteWait(long long tick){
    if(b.getCurrentTick()>=tick){
	return true;
    }
    Thread::nanoSleepUntil(tc.tick2ns(tick));
    return false;
}

bool TransceiverTimer::absoluteWaitTrigger(long long tick){
    FastInterruptDisableLock dLock;
    if(b.IRQsetNextTransceiverInterrupt(tick)==WaitResult::WAKEUP_IN_THE_PAST){
	return true;
    }
    b.setModeTransceiverTimer(false);
    do {
	tWaiting=Thread::IRQgetCurrentThread();
	Thread::IRQwait();
	{
	    FastInterruptEnableLock eLock(dLock);
	    Thread::yield();
	}
    } while(tWaiting && tick>b.getCurrentTick());
    return false;
}

bool TransceiverTimer::absoluteWaitTimeoutOrEvent(long long tick){
    FastInterruptDisableLock dLock;
    if(b.IRQsetTransceiverTimeout(tick)==WaitResult::WAKEUP_IN_THE_PAST){
	HighResolutionTimerBase::aux=1;
        return true;
    }
    b.setModeTransceiverTimer(true);
    b.cleanBufferTrasceiver();
    b.enableCC0Interrupt(false);
    b.enableCC0InterruptTim2(true);
    do {
        tWaiting=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
	    Thread::yield();
        }
    } while(tWaiting && tick>b.getCurrentTick());
    
    if(tWaiting==nullptr){
        HighResolutionTimerBase::aux=2;
	return false;
    }else{
        HighResolutionTimerBase::aux=3;
	return true;
    }
}

bool TransceiverTimer::waitTimeoutOrEvent(long long tick){
    return absoluteWaitTimeoutOrEvent(b.getCurrentTick()+tick);
}

long long TransceiverTimer::tick2ns(long long tick){
    return tc.tick2ns(tick);
}

long long TransceiverTimer::ns2tick(long long ns){
    return tc.ns2tick(ns);
}
            
unsigned int TransceiverTimer::getTickFrequency() const{
    return b.getTimerFrequency();
}
	    
long long TransceiverTimer::getExtEventTimestamp() const{
    return b.IRQgetSetTimeTransceiver()-stabilizingTime;
}
	 
const int TransceiverTimer::stabilizingTime=7;

TransceiverTimer::TransceiverTimer():b(HighResolutionTimerBase::instance()),tc(b.getTimerFrequency()) {}

TransceiverTimer& TransceiverTimer::instance(){
    static TransceiverTimer instance;
    return instance;
}

TransceiverTimer::~TransceiverTimer() {}

