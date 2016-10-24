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

bool GPIOtimer::getMode(){
    return isInput;
}

//NOTE: Think about how to set the right ms32chkp related to the captured timestamp
long long GPIOtimer::getExtEventTimestamp() const{
    return b.IRQgetSetTimeCCV2();
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
    if(isInput){
	b.setModeGPIOTimer(false);		//output timer 
	expansion::gpio10::mode(Mode::OUTPUT);	//output pin
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
	expansion::gpio1::high();
	FastInterruptDisableLock dLock;
	if(isInput){
	    b.setModeGPIOTimer(false);			//output timer 
	    expansion::gpio10::mode(Mode::OUTPUT);	//output pin
	    isInput=false;
	}
	if(b.IRQsetNextGPIOInterrupt(tick)==WaitResult::WAKEUP_IN_THE_PAST){
	    return true;
	}
	expansion::gpio1::low();
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

GPIOtimer& GPIOtimer::instance(){
    static GPIOtimer instance;
    return instance;
}

GPIOtimer::GPIOtimer(): b(HighResolutionTimerBase::instance()),tc(b.getTimerFrequency()) {
    b.setModeGPIOTimer(true);
    expansion::gpio10::mode(Mode::INPUT);
    isInput=true;
}