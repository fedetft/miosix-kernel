/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   high_resolution_timer_base.h
 * Author: fabiuz
 *
 * Created on September 27, 2016, 2:20 AM
 */

#include "miosix.h"

#ifndef HIGH_RESOLUTION_TIMER_BASE_H
#define HIGH_RESOLUTION_TIMER_BASE_H

namespace miosix {

class HighResolutionTimerBase {
public:
    
    static HighResolutionTimerBase& instance();
    
    /**
     * \return the timer frequency in Hz
     */
    unsigned int getTimerFrequency() const
    {
        return timerFreq;
    }
    
    /**
     * Set the next interrupt.
     * Can be called with interrupts disabled or within an interrupt.
     * \param tick the time when the interrupt will be fired, in timer ticks
     */
    bool IRQsetNextInterrupt0(long long tick);
    void IRQsetNextInterrupt1(long long tick);
    bool IRQsetNextInterrupt2(long long tick);
    
    /**
     * \return the time when the next interrupt will be fired.
     * That is, the last value passed to setNextInterrupt().
     */
    long long IRQgetSetTimeCCV0();
    long long IRQgetSetTimeCCV1();
    long long IRQgetSetTimeCCV2();
    /**
     * Could be call both when the interrupts are enabled/disabled!
     * TODO: investigate if it's possible to remove the possibility to call
     * this with IRQ disabled and use IRQgetCurrentTick() instead
     * \return the current tick count of the timer
     */
    long long getCurrentTick();
    
    void setCCInterrupt0(bool enable);
    void setCCInterrupt1(bool enable);
    void setCCInterrupt2(bool enable);
    void setCCInterrupt2Tim1(bool enable);
    void setModeGPIOTimer(bool input);
    /**
     * \return the current tick count of the timer.
     * Can only be called with interrupts disabled or within an IRQ
     */
    long long IRQgetCurrentTick();
    
    virtual ~HighResolutionTimerBase();
    
private:
    HighResolutionTimerBase();
    unsigned int timerFreq;
};

}//end miosix namespace
#endif /* HIGH_RESOLUTION_TIMER_BASE_H */

