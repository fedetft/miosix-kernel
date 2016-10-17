/* 
 * File:   gpio_timer.cpp
 * Author: fabiuz
 * 
 * Created on September 27, 2016, 6:30 AM
 */

#include "gpio_timer.h"
#include "hwmapping.h"
#include "miosix.h"
#include <cstdlib>
#include <cstdio>
#include "../../../../debugpin.h"
using namespace miosix;

GPIOtimer& GPIOtimer::instance(){
    static GPIOtimer instance;
    return instance;
}

GPIOtimer::GPIOtimer(): b(HighResolutionTimerBase::instance()),tc(b.getTimerFrequency()) {
    setPinMode(false);
    isInput=true;
}

Thread* GPIOtimer::tWaitingGPIO=nullptr;

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


void GPIOtimer::setPinMode(bool inputMode){
    if(inputMode){
        expansion::gpio10::mode(Mode::INPUT);
        isInput=true;
    }else{
        expansion::gpio10::mode(Mode::OUTPUT);
        isInput=false;
    }
}

bool GPIOtimer::getMode(){
    return isInput;
}

long long GPIOtimer::getExtEventTimestamp() const{
    return b.IRQgetSetTimeCCV2();
}

bool GPIOtimer::absoluteWaitTimeoutOrEvent(long long tick){
    FastInterruptDisableLock dLock;
    if(tick<b.getCurrentTick()){
	return true;
    }
    
    b.setModeGPIOTimer(true);
    setPinMode(true);
    b.setCCInterrupt2(false);
    b.setCCInterrupt2Tim1(true);
    do {
        tWaitingGPIO=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
	    Thread::yield();
        }
    } while(tWaitingGPIO && tick>b.getCurrentTick());
    
    if(tWaitingGPIO==nullptr){
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
    b.setModeGPIOTimer(false);    //output timer 
    setPinMode(false);	    //output pin
    if(b.IRQsetNextInterrupt2(tick)==WaitResult::WAKEUP_IN_THE_PAST){
	return true;
    }
    return false;
}

bool GPIOtimer::waitTrigger(long long tick){
    return absoluteWaitTrigger(b.getCurrentTick()+tick);
}

bool GPIOtimer::absoluteSyncWaitTrigger(long long tick){
    {	
	FastInterruptDisableLock dLock;
	b.setModeGPIOTimer(false);	//output timer 
	setPinMode(false);		//output pin
	if(b.IRQsetNextInterrupt2(tick)==WaitResult::WAKEUP_IN_THE_PAST){
	    return true;
	}
    
	do {
	    tWaitingGPIO=Thread::IRQgetCurrentThread();
	    Thread::IRQwait();
	    {
		FastInterruptEnableLock eLock(dLock);
		Thread::yield();
	    }
	} while(tWaitingGPIO && tick>b.getCurrentTick());
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

long long GPIOtimer::aux1=0;
GPIOtimer::~GPIOtimer() {}

