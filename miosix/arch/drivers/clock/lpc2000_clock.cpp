/***************************************************************************
 *   Copyright (C) 2008-2026 by Terraneo Federico                          *
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

#include "lpc2000_clock.h"
// #include "board_settings.h"
#include "miosix_settings.h"

namespace miosix {

//
// System PLL
//

/**
 * \internal
 * Generates the PLL feed sequence
 */
static inline void feed()
{
    PLLFEED=0xAA;
    PLLFEED=0x55;
}

void IRQsetPllFreq(PllValues r)
{
    const unsigned int PLOCK=0x400;
    PLLCON=0x0;//PLL OFF
    feed();
    switch(r)
    {
        case PLL_2X://PLL=2*F
            // Enabling MAM
            MAMCR=0x0;//MAM Disabled
            MAMTIM=0x2;//Flash access is 2 cclk
            MAMCR=0x2;//MAM Enabled
            // Setting PLL
            PLLCFG=0x41;// M=2 P=4 ( FCCO=14.7456*M*2*P=235MHz )
            feed();
            PLLCON=0x1;//PLL Enabled
            feed();
            while(!(PLLSTAT & PLOCK));//Wait for PLL to lock
            PLLCON=0x3;//PLL Connected
            feed();
            break;
        case PLL_4X://PLL=4*F
            // Enabling MAM
            MAMCR=0x0;//MAM Disabled
            MAMTIM=0x3;//Flash access is 3 cclk
            MAMCR=0x2;//MAM Enabled
            // Setting PLL
            PLLCFG=0x23;// M=4 P=2 ( FCCO=14.7456*M*2*P=235MHz )
            feed();
            PLLCON=0x1;//PLL Enabled
            feed();
            while(!(PLLSTAT & PLOCK));//Wait for PLL to lock
            PLLCON=0x3;//PLL Connected
            feed();
            break;
        default://PLL OFF
            // Enabling MAM
            MAMCR=0x0;//MAM Disabled
            MAMTIM=0x1;//Flash access is 1 cclk
            MAMCR=0x2;//MAM Enabled
            break;
    }
}

PllValues IRQgetPllFreq()
{
    //If "pll off" or "pll not connected" return PLL_OFF
    if((PLLSTAT & 0x300)!=0x300) return PLL_OFF;
    switch(PLLSTAT & 0x7f)
    {
        case 0x41: return PLL_2X;
        case 0x23: return PLL_4X;
        default:   return PLL_UNDEF;//PLL undefined value
    }
}

void IRQmemoryAndClockInit()
{
    static_assert(cpuFrequency<=60000000, "CPU cpuFreqeuncy outside range");
    static_assert(
           cpuFrequency==oscillatorFrequency
        || cpuFrequency==oscillatorFrequency*2
        || cpuFrequency==oscillatorFrequency*4, "Unsupported PLL multiplier");
    switch(cpuFrequency)
    {
        case oscillatorFrequency*2:
            IRQsetPllFreq(PLL_2X);
            break;
        case oscillatorFrequency*4:
            IRQsetPllFreq(PLL_4X);
            break;
        default:
            IRQsetPllFreq(PLL_OFF);
            break;
    }

    static_assert(
           peripheralFrequency==cpuFrequency
        || peripheralFrequency==cpuFrequency/2
        || peripheralFrequency==cpuFrequency/4, "Unsupported PLL multiplier");
    switch(peripheralFrequency)
    {
        case cpuFrequency/2:
            IRQsetApbRatio(APB_DIV_2);
            break;
        case cpuFrequency/4:
            IRQsetApbRatio(APB_DIV_4);
            break;
        default:
            IRQsetApbRatio(APB_DIV_1);
            break;
    }
}

} //namespace miosix
