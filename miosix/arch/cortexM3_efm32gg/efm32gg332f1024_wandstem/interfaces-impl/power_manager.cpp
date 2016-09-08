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
#include "interfaces/bsp.h"
#include <stdexcept>

using namespace std;

namespace miosix {

PowerManager& PowerManager::instance()
{
    static PowerManager singleton;
    return singleton;
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
        //safe side if the regulator is heavily loaded
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

} //namespace miosix
