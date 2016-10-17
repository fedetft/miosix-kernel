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

Thread* RadioTimer::tWaitingRadio=nullptr;

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
    return 0;
}

bool RadioTimer::waitTimeoutOrEvent(long long tick){
    return 0;
}
bool RadioTimer::absoluteWaitTimeoutOrEvent(long long tick){
    return 0;
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

