/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   radio_timer.cpp
 * Author: fabiuz
 * 
 * Created on October 11, 2016, 11:17 AM
 */

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
    if(tick<b.getCurrentTick()){
	return true;
    }
    
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
	return false;
    }else{
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
    return b.IRQgetSetTimeCCV0();
}
	    
TransceiverTimer::TransceiverTimer():b(HighResolutionTimerBase::instance()),tc(b.getTimerFrequency()) {
    b.setModeTransceiverTimer();
}

TransceiverTimer& TransceiverTimer::instance(){
    static TransceiverTimer instance;
    return instance;
}

TransceiverTimer::~TransceiverTimer() {}

