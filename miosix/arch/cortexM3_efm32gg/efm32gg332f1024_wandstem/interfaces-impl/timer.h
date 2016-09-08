/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *   Copyright (C) 2013, 2014 by Terraneo Federico and Luigi Rinaldi       *
 *   Copyright (C) 2015, 2016 by Terraneo Federico, Luigi Rinaldi and      *
 *   Silvano Seva                                                          *
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

#ifndef TIMER_H
#define TIMER_H

namespace miosix {

/**
 * Pure virtual class defining the RadioTimer interface.
 * This interface includes support for an input capture module, used by the
 * Transceiver driver to timestamp packet reception, and an output compare
 * module, used again by the Transceiver to send packets deterministically.
 */
class HardwareTimer
{
public:
    /**
     * \return the timer counter value
     */
    virtual long long getValue() const=0;

    /**
     * Set the RTC counter value
     * \param value new RTC value
     */
    virtual void setValue(long long value)=0;
    
    /**
     * Put thread in wait for the specified relative time.
     * This function wait for a relative time passed as parameter.
     * \param value relative time to wait, expressed in number of tick of the 
     * count rate of timer.
     */
    virtual void wait(long long value)=0;
    
    /**
     * Puts the thread in wait for the specified absolute time.
     * \param value absolute time in which the thread is waiting.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     * \return true if the wait time was in the past
     */
    virtual bool absoluteWait(long long value)=0;
    
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
    virtual bool absoluteWaitTrigger(long long value)=0;
    
    /**
     * Put thread in waiting of timeout or extern event.
     * \param value timeout expressed in number of tick of the 
     * count rate of timer.
     * \return true in case of timeout
     */
    virtual bool waitTimeoutOrEvent(long long value)=0;
    
    /**
     * Put thread in waiting of timeout or extern event.
     * \param value absolute timeout expressed in number of tick of the 
     * count rate of timer.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     * \return true in case of timeout, or if the wait time is in the past.
     * In the corner case where both the timeout and the event are in the past,
     * return false.
     */
    virtual bool absoluteWaitTimeoutOrEvent(long long value)=0;
    
    /**
     * \return the precise time when the IRQ signal of the event was asserted
     */
    virtual long long getExtEventTimestamp() const=0;
};

/**
 * Manages the hardware timer that runs also in low power mode.
 * This class is not safe to be accessed by multiple threads simultaneously.
 */
class Rtc : public HardwareTimer
{
public:
    /**
     * \return an instance to the RTC (singleton)
     */
    static Rtc& instance();
    
    /**
     * \return the timer counter value
     */
    long long getValue() const;

    /**
     * Set the RTC counter value
     * \param value new RTC value
     */
    void setValue(long long value);
    
    /**
     * Put thread in wait for the specified relative time.
     * This function wait for a relative time passed as parameter.
     * \param value relative time to wait, expressed in number of tick of the 
     * count rate of timer.
     */
    void wait(long long value);
    
    /**
     * Puts the thread in wait for the specified absolute time.
     * \param value absolute time in which the thread is waiting.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     * \return true if the wait time was in the past
     */
    bool absoluteWait(long long value);
    
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
    bool absoluteWaitTrigger(long long value);
    
    /**
     * Put thread in waiting of timeout or extern event.
     * \param value timeout expressed in number of tick of the 
     * count rate of timer.
     * \return true in case of timeout
     */
    bool waitTimeoutOrEvent(long long value);
    
    /**
     * Put thread in waiting of timeout or extern event.
     * \param value absolute timeout expressed in number of tick of the 
     * count rate of timer.
     * If value of absolute time is in the past no waiting will be set
     * and function return immediately.
     * \return true in case of timeout, or if the wait time is in the past.
     * In the corner case where both the timeout and the event are in the past,
     * return false.
     */
    bool absoluteWaitTimeoutOrEvent(long long value);
    
    /**
     * \return the precise time when the IRQ signal of the event was asserted
     */
    long long getExtEventTimestamp() const;
    
    static const unsigned int frequency=32768; ///< The RTC frequency in Hz
    
private:
    /**
     * Constructor
     */
    Rtc();  
};

} //namespace miosix

#endif //TIMER_H
