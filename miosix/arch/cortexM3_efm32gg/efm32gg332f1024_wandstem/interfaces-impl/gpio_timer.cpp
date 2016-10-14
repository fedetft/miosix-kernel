/* 
 * File:   gpio_timer.cpp
 * Author: fabiuz
 * 
 * Created on September 27, 2016, 6:30 AM
 */

#include "gpio_timer.h"
#include "high_resolution_timer_base.h"
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

GPIOtimer::GPIOtimer(): b(HighResolutionTimerBase::instance()) {
    setPinMode(false);
    isInput=true;
}

Thread* GPIOtimer::tWaitingGPIO=nullptr;

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

WaitResult GPIOtimer::absoluteWaitTimeoutOrEvent(long long tick){
    FastInterruptDisableLock dLock;
    if(tick<b.getCurrentTick()){
	return WaitResult::WAKEUP_IN_THE_PAST;
    }
    
    b.setModeGPIOTimer(true);
    setPinMode(true);
    b.setCCInterrupt2(false);
    b.setCCInterrupt2Tim1(true);
    long long ctick=0;
    do {
        tWaitingGPIO=Thread::IRQgetCurrentThread();
        Thread::IRQwait();
        {
            FastInterruptEnableLock eLock(dLock);
	    Thread::yield();
        }
	ctick = b.getCurrentTick();
    } while(tWaitingGPIO && tick>ctick);
    
    if(tWaitingGPIO==nullptr){
	return WaitResult::EVENT;
    }else{
	return WaitResult::WAIT_COMPLETED;
    }
}

WaitResult GPIOtimer::waitTimeoutOrEvent(long long tick){
    return absoluteWaitTimeoutOrEvent(b.getCurrentTick()+tick);
}

/*
 Not blocking function!
 */
bool GPIOtimer::absoluteWaitTrigger(long long tick){
    FastInterruptDisableLock dLock;
    b.setModeGPIOTimer(false);    //output timer 
    setPinMode(false);	    //output pin
    if(!b.IRQsetNextInterrupt2(tick)){
	return false;
    }
    return true;
}

bool GPIOtimer::waitTrigger(long long tick){
    return absoluteWaitTrigger(b.getCurrentTick()+tick);
}

bool GPIOtimer::absoluteSyncWaitTrigger(long long tick){
    {	
	FastInterruptDisableLock dLock;
	b.setModeGPIOTimer(false);	//output timer 
	setPinMode(false);		//output pin
	if(!b.IRQsetNextInterrupt2(tick)){
	    return false;
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
    return true;
}

bool GPIOtimer::syncWaitTrigger(long long tick){
    return absoluteSyncWaitTrigger(b.getCurrentTick()+tick); 
}

long long GPIOtimer::aux1=0;
GPIOtimer::~GPIOtimer() {}

