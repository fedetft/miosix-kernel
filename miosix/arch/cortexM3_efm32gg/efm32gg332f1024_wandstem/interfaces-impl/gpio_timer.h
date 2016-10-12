/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gpio_timer.h
 * Author: fabiuz
 *
 * Created on September 27, 2016, 6:30 AM
 */

#include "high_resolution_timer_base.h"
#include "hwmapping.h"

#ifndef GPIO_TIMER_H
#define GPIO_TIMER_H

static volatile int aux=0;

namespace miosix {
    enum class WaitResult
    {
        WAKEUP_IN_THE_PAST,
        WAIT_COMPLETED,
        EVENT
    };
    
    class GPIOtimer{    
    public:
        static Thread *tWaitingGPIO;
        static GPIOtimer& instance();
        virtual ~GPIOtimer();
        WaitResult waitTimeoutOrEvent(long long value);
        WaitResult absoluteWaitTimeoutOrEvent(long long value);
        
        /**
        * Set the timer interrupt to occur at an absolute value and put the 
        * thread in wait of this. 
        * When the timer interrupt will occur, the associated GPIO passes 
        * from a low logic level to a high logic level for few us.
        * \param value absolute value when the interrupt will occur, expressed in 
        * number of tick of the count rate of timer.
        * If value of absolute time is in the past no waiting will be set
        * and function return immediately. In this case, the GPIO will not be
        * pulsed
        * \return true if the wait time was in the past, in this case the GPIO
        * has not been pulsed
        */
        bool absoluteWaitTrigger(long long tick);
        bool waitTrigger(long long tick);
        
        
        
        long long getExtEventTimestamp() const;
        
        bool getMode();
        
    private:
        GPIOtimer();
        void setPinMode(bool inputMode);
        HighResolutionTimerBase& b;
        bool isInput; 
    };

}

#endif /* GPIO_TIMER_H */

