/***************************************************************************
 *   Copyright (C) 2016 by Terraneo Federico                               *
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

#include "power_manager.h"
#include "board_settings.h"
#include "hardware_timer.h"
#include "transceiver.h"
#include "interfaces/bsp.h"
#include <stdexcept>
#include <sys/ioctl.h>

using namespace std;

namespace miosix {

//
// class PowerManager
//

PowerManager& PowerManager::instance()
{
    static PowerManager singleton;
    return singleton;
}

void PowerManager::deepSleepUntil(long long int when)
{
    Lock<FastMutex> l(powerMutex);
    
    Rtc& rtc=Rtc::instance();
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    PauseKernelLock pkLock;
    
    preDeepSleep();
    
    RTC->COMP1=when & 0xffffff;
    while(RTC->SYNCBUSY & RTC_SYNCBUSY_COMP1) ;
    
    {
        FastInterruptDisableLock dLock;
        
        //NOTE: this is very important, enabling the interrpt without clearing the
        //interrupt flag causes the function to return prematurely, sometimes
        RTC->IFC=RTC_IFC_COMP1;
        RTC->IEN |= RTC_IEN_COMP1;

        //NOTE: the corner case where the wakeup is now is considered "in the past"
        if(when<=rtc.IRQgetValue())
        {
            RTC->IFC=RTC_IFC_COMP1;
            RTC->IEN &= ~RTC_IEN_COMP1;
        } else {
            SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
            EMU->CTRL=0;
            for(;;)
            {
                //First, try to go to sleep. If an interrupt occurred since when
                //we have disabled them, this is executed as a nop
                __WFI();
                expansion::gpio0::high();//FIXME
                //Restart HFXO
                CMU->OSCENCMD=CMU_OSCENCMD_HFXOEN;
                CMU->CMD=CMU_CMD_HFCLKSEL_HFXO; //This locks the CPU till clock is stable
                CMU->OSCENCMD=CMU_OSCENCMD_HFRCODIS;
                //If the interrupt we want is now pending, everything's ok
                if(NVIC_GetPendingIRQ(RTC_IRQn))
                {
                    //Clear the interrupt (both in the RTC peripheral and NVIC),
                    //this is important because to do a WFI again, no interrupts must
                    //be pending
                    //FIXME sleeps of more that 512s still don't work due to Rtc
                    //class requirement of at least one read per period
                    RTC->IFC=RTC_IFC_COMP1;
                    NVIC_ClearPendingIRQ(RTC_IRQn);
                    if(when<=rtc.IRQgetValue()) break;
                } else {
                    //Else we are in an uncomfortable situation: we're waiting for
                    //a specific interrupt, but we didn't go to sleep as another
                    //interrupt occured. The core won't allow thw WFI to put us to
                    //sleep till we serve the interrupt, so let's do it.
                    //Note that since the kernel is paused the interrupt we're
                    //serving can't cause a context switch and fuck up things.
                    {
                        FastInterruptEnableLock eLock(dLock);
                        //Here interrupts are enabled, so the interrupt gets served
                        __NOP();
                    }
                }
            }
            SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
            RTC->IEN &= ~RTC_IEN_COMP1;
        }
    }
    
    postDeepSleep();
    expansion::gpio0::low();//FIXME
}

void PowerManager::enableTransceiverPowerDomain()
{   
    Lock<FastMutex> l(powerMutex);
    if(transceiverPowerDomainRefCount==0)
    {
        //Enable power domain
        transceiver::vregEn::high();
        
        transceiver::reset::low();
        
        //The voltage at the the flash rises in about 70us,
        //observed using an oscilloscope. The voltage at the
        //transceiver is even faster, but the spec says 100us
        delayUs(100);
        
        transceiver::cs::high();

        flash::cs::high();
        flash::hold::high();
        
        spi.enable();
    }
    transceiverPowerDomainRefCount++;
}

void PowerManager::disableTransceiverPowerDomain()
{
    Lock<FastMutex> l(powerMutex);
    transceiverPowerDomainRefCount--;
    if(transceiverPowerDomainRefCount==0)
    {
        //Disable power domain
        spi.disable();
        
        flash::hold::low();
        flash::cs::low();
        
        transceiver::reset::low();
        transceiver::cs::low();
        
        transceiver::vregEn::low();
    } else if(transceiverPowerDomainRefCount<0) {
        throw runtime_error("transceiver power domain refcount is negative");
    }
}

bool PowerManager::isTransceiverPowerDomainEnabled() const
{
    return transceiverPowerDomainRefCount>0;
}

void PowerManager::enableSensorPowerDomain()
{
    #if WANDSTEM_HW_REV>13
    Lock<FastMutex> l(powerMutex);
    if(sensorPowerDomainRefCount==0)
    {
        powerSwitch::high();
    }
    sensorPowerDomainRefCount++;
    #else
    //Nodes prior to rev 1.4 have no separate sensor power domain
    enableTransceiverPowerDomain();
    #endif
}

void PowerManager::disableSensorPowerDomain()
{
    #if WANDSTEM_HW_REV>13
    Lock<FastMutex> l(powerMutex);
    sensorPowerDomainRefCount--;
    if(sensorPowerDomainRefCount==0)
    {
        powerSwitch::low();
    } else if(sensorPowerDomainRefCount<0) {
        throw runtime_error("sensor power domain refcount is negative");
    }
    #else
    //Nodes prior to rev 1.4 have no separate sensor power domain
    disableTransceiverPowerDomain();
    #endif
}

bool PowerManager::isSensorPowerDomainEnabled() const
{
    #if WANDSTEM_HW_REV>13
    return sensorPowerDomainRefCount>0;
    #else
    //Nodes prior to rev 1.4 have no separate sensor power domain
    return isTransceiverPowerDomainEnabled();
    #endif
}

void PowerManager::enableHighRegulatorVoltage()
{
    //Nodes prior to rev 1.3 have no switching voltage regulator
    #if WANDSTEM_HW_REV>12
    Lock<FastMutex> l(powerMutex);
    if(regulatorVoltageRefCount==0)
    {
        voltageSelect::high();
        
        //The voltage regulator, under no load, takes about 500us to increase
        //the voltage, observed using an oscilloscope. Using 2ms to be on the
        //safe side if the regulator is heavily loaded. TODO: characterize it
        Thread::sleep(2);
    }
    regulatorVoltageRefCount++;
    #endif
}

void PowerManager::disableHighRegulatorVoltage()
{
    //Nodes prior to rev 1.3 have no switching voltage regulator
    #if WANDSTEM_HW_REV>12
    Lock<FastMutex> l(powerMutex);
    regulatorVoltageRefCount--;
    if(regulatorVoltageRefCount==0)
    {
        voltageSelect::low();
    } else if(regulatorVoltageRefCount<0) {
        throw runtime_error("voltage regulator refcount is negative");
    }
    #endif
}

bool PowerManager::isRegulatorVoltageHigh()
{
    #if WANDSTEM_HW_REV>12
    return regulatorVoltageRefCount>0;
    #else
    //Nodes prior to rev 1.3 have no switching voltage regulator
    return true;
    #endif
}

PowerManager::PowerManager()
    : transceiverPowerDomainRefCount(0),
      sensorPowerDomainRefCount(0),
      regulatorVoltageRefCount(0),
      spi(Spi::instance()) {}

void PowerManager::preDeepSleep()
{
    if(transceiverPowerDomainRefCount>0)
    {
        wasTransceiverTurnedOn=Transceiver::instance().isTurnedOn();
        
        //Disable power domain (this also disables the transceiver)
        spi.disable();
        
        flash::hold::low();
        flash::cs::low();
        
        transceiver::reset::low();
        transceiver::cs::low();
        
        transceiver::vregEn::low();
    }
    
    #if WANDSTEM_HW_REV>13
    if(sensorPowerDomainRefCount>0) powerSwitch::low();
    #endif
    
    //Not setting the voltage regulator to low voltage for now
}

void PowerManager::postDeepSleep()
{
    if(transceiverPowerDomainRefCount>0)
    {
        //Enable power domain
        transceiver::vregEn::high();
        
        //The voltage at the the flash rises in about 70us,
        //observed using an oscilloscope. The voltage at the
        //transceiver is even faster, but the spec says 100us
        delayUs(100); //TODO: power hungry

        flash::cs::high();
        flash::hold::high();
        
        spi.enable();
        
        if(wasTransceiverTurnedOn)
        {
            transceiver::reset::high();

            //wait until SO=1 (clock stable and running)
            expansion::gpio1::high();//FIXME
            while(internalSpi::miso::value()==0) ; //TODO: power hungry
            expansion::gpio1::low();//FIXME
            transceiver::cs::high();
            
            Transceiver::instance().configure();
        }
    }
    
    #if WANDSTEM_HW_REV>13
    if(sensorPowerDomainRefCount>0) powerSwitch::high();
    #endif
}

} //namespace miosix
