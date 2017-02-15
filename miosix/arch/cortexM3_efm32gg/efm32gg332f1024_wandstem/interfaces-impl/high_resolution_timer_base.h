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

#ifndef HIGH_RESOLUTION_TIMER_BASE_H
#define HIGH_RESOLUTION_TIMER_BASE_H

#include "miosix.h"
#include "../../../../e20/e20.h"
namespace miosix {

enum class WaitResult
{
    WAKEUP_IN_THE_PAST,
    WAIT_COMPLETED,
    EVENT,
    WAITING
};
    
class HRTB {
    public:

        static HRTB& instance();

        /**
         * \return the timer frequency in Hz
         */
        unsigned int getTimerFrequency() const{
            return HRTB::freq;
        }

        /**
         * Set the next interrupt.
         * Can be called with interrupts disabled or within an interrupt.
         * \param tick the time when the interrupt will be fired, in timer ticks
         */
        WaitResult IRQsetNextTransceiverInterrupt(long long tick);
        void IRQsetNextInterruptCS(long long tick);
        WaitResult IRQsetNextGPIOInterrupt(long long tick);

        /**
         * \return the time when the next interrupt will be fired.
         * That is, the last value passed to setNextInterrupt(), or the value
         * captured in input mode.
         */
        long long IRQgetSetTimeTransceiver() const;
        long long IRQgetSetTimeCS() const;
        long long IRQgetSetTimeGPIO() const;

        /*
         * Clean buffer in TIMER used by GPIOTimer, necessary when input capture 
         * mode is enabled
         */
        void cleanBufferGPIO();
        void cleanBufferTrasceiver();

        /**
         * Could be call both when the interrupts are enabled/disabled!
         * TODO: investigate if it's possible to remove the possibility to call
         * this with IRQ disabled and use IRQgetCurrentTime() instead
         * \return the current tick count of the timer
         */
        long long getCurrentTick();
        long long getCurrentTickCorrected();
        /**
         * \return the current tick count of the timer.
         * Can only be called with interrupts disabled or within an IRQ
         */
        long long IRQgetCurrentTick();
        long long IRQgetCurrentTickCorrected();
        long long IRQgetCurrentTickVht();

        /**
         * Return the VHT timestamp, let's say the captured result after a pulse of RTC
         * @return 
         */
        long long getVhtTimestamp();
        
        void enableCC0Interrupt(bool enable);
        void enableCC1Interrupt(bool enable);
        void enableCC2Interrupt(bool enable);
        void enableCC0InterruptTim1(bool enable);
        void enableCC2InterruptTim1(bool enable);
        void enableCC0InterruptTim2(bool enable);
        void enableCC1InterruptTim2(bool enable);

        /**
         * Function to prepare the timers to works in a given mode. For Transceiver,
         * it use 2 different low channel, so we can set both of them at the 
         * beginning of our code.
         * \param input true to set the input/capture mode, false to set the output 
         * mode
         */
        void setModeGPIOTimer(bool input);
        void setModeTransceiverTimer(bool input);
        
        WaitResult IRQsetGPIOtimeout(long long tick);
        WaitResult IRQsetTransceiverTimeout(long long tick);
        
        Thread* IRQgpioWait(long long tick,FastInterruptDisableLock* dLock);
        Thread* IRQtransceiverWait(long long tick,FastInterruptDisableLock *dLock);
        
        void initTransceiver();
        bool transceiverAbsoluteWaitTimeoutOrEvent(long long tick);
        bool transceiverAbsoluteWaitTrigger(long long tick);
        
        void initGPIO();
        bool gpioAbsoluteWaitTimeoutOrEvent(long long tick);
        bool gpioAbsoluteWaitTrigger(long long tick);
        
        virtual ~HRTB();

        /**
        These 4 variables are used to manage the correction of the timers.
        * vhtBase (high frequency): keeps the last sync point
        * vhtSyncPointRtc (low frequency): keeps the last sync point, just a simple conversion from vhtBase
        * vhtSyncPointVht (high frequency: keeps the precise value of last sync point
        * vhtOffset (high frequency): keeps the difference between the actual time and the counter value
        */
        static long long syncPointHrtMaster;
        static long long syncPointHrtSlave;
        static long long syncPointHrtTheoretical;
        static long long vhtOffset;
        static long long syncPeriodRtc;
        static long long syncPointRtc;
        static long long nextSyncPointRtc;
        static long long syncPeriodHrt;
        static long long clockCorrection;
        static long long clockCorrectionFlopsync;
        
        /**
         * Debug variables
         */
        static int aux;
        static long long aux1,aux2,aux3,aux4,error;
        static FixedEventQueue<50,16> queue;
        static Thread *flopsyncThread;
        static long long diffs[100];
    
    private:
        HRTB();
        static const unsigned int freq;
};

}//end miosix namespace
#endif /* HIGH_RESOLUTION_TIMER_BASE_H */

