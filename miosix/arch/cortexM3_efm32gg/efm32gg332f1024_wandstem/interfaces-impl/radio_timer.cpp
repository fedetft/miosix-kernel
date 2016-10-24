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

#include "radio_timer.h"

using namespace miosix;

Thread* RadioTimer::tWaiting=nullptr;

long long RadioTimer::getValue() const{
     return b.getCurrentTick();
 }  
            
void RadioTimer::wait(long long tick){
    Thread::nanoSleep(tc.tick2ns(tick));
}

bool RadioTimer::absoluteWait(long long tick){
    if(b.getCurrentTick()>=tick){
	return true;
    }
    Thread::nanoSleepUntil(tc.tick2ns(tick));
    return false;
}

bool RadioTimer::absoluteWaitTrigger(long long tick){
    FastInterruptDisableLock dLock;
    b.setModeRadioTimer(false);			//output timer 
    if(b.IRQsetNextRadioInterrupt(tick)==WaitResult::WAKEUP_IN_THE_PAST){
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

bool RadioTimer::absoluteWaitTimeoutOrEvent(long long tick){
    FastInterruptDisableLock dLock;
    if(tick<b.getCurrentTick()){
	return true;
    }
    
    b.setModeRadioTimer(true);
    b.setCCInterrupt0(false);
    b.setCCInterrupt0Tim2(true);
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

long long RadioTimer::tick2ns(long long tick){
    return tc.tick2ns(tick);
}

long long RadioTimer::ns2tick(long long ns){
    return tc.ns2tick(ns);
}
            
unsigned int RadioTimer::getTickFrequency() const{
    return b.getTimerFrequency();
}

	    
long long RadioTimer::getExtEventTimestamp() const{
    return b.IRQgetSetTimeCCV0();
}
	    
RadioTimer::RadioTimer():b(HighResolutionTimerBase::instance()),tc(b.getTimerFrequency()) {}

RadioTimer& RadioTimer::instance(){
    static RadioTimer instance;
    return instance;
}

RadioTimer::~RadioTimer() {}

