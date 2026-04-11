/***************************************************************************
 *   Copyright (C) 2025 by Terraneo Federico                               *
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
 *   by the GNU General Public License. However the suorce code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "nrf52_clock.h"
#include <miosix_settings.h>
#include <interfaces/arch_registers.h>
#include <interfaces/delays.h>
#include "drivers/mpu/cortexMx_mpu.h"

extern "C" {
unsigned int SystemCoreClock = miosix::cpuFrequency;
}

namespace miosix {

void IRQmemoryAndClockInit()
{
    NRF_NVMC->ICACHECNF=1; // Enable FLASH cache

    // Added for safety to connect debugger if wrong options selected
    delayMs(1000);

    switch(oscillatorType)
    {
        case OscillatorType::HFINT: break; // Default option at boot
        case OscillatorType::HFXO:
            NRF_CLOCK->EVENTS_HFCLKSTARTED=0;
            NRF_CLOCK->TASKS_HFCLKSTART=1;
            while(NRF_CLOCK->EVENTS_HFCLKSTARTED==0) ;
            break;
    }
    NRF_CLOCK->TASKS_LFCLKSTOP=1;
    switch(rtcOscillatorType)
    {
        case RtcOscillatorType::NONE: break;
        case RtcOscillatorType::LFRC:   NRF_CLOCK->LFCLKSRC=0; break;
        case RtcOscillatorType::LFXO:   NRF_CLOCK->LFCLKSRC=1; break;
        case RtcOscillatorType::LFSYNT: NRF_CLOCK->LFCLKSRC=2; break;
    }
    if(rtcOscillatorType!=RtcOscillatorType::NONE)
    {
        NRF_CLOCK->EVENTS_LFCLKSTARTED=0;
        NRF_CLOCK->TASKS_LFCLKSTART=1;
        while(NRF_CLOCK->EVENTS_LFCLKSTARTED==0) ;
    }
    switch(vregType)
    {
        case VoltageRegulatorType::LDO: break; // Default option at boot
        case VoltageRegulatorType::SWITCHING:
            NRF_POWER->DCDCEN=1;
            break;
    }

    //See Cortex M4 TRM: enable the FPU in privileged and user mode
    SCB->CPACR |= 0xf<<20;
    
    // Architecture has MPU, enable kernel-level W^X protection
    IRQconfigureMPU();
}

} // namespace miosix
