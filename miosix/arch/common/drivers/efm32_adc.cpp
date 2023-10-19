/***************************************************************************
 *   Copyright (C) 2023 by Terraneo Federico                               *
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

#include "efm32_adc.h"
#include "miosix.h"
#include <algorithm>

Adc& Adc::instance()
{
    static Adc singleton;
    return singleton;
}

void Adc::setPowerMode(PowerMode mode)
{
    ADC0->CTRL &= ~_ADC_CTRL_WARMUPMODE_MASK;
    //NOTE: from an implementation perspective Off is the same as OnDemand
    //but from a logical perspective Off is used to turn the ADC off after it
    //has been turned on, while OnDemand is used to pay the startup cost at
    //every conversion
    if(mode==On)
    {
        ADC0->CTRL |= ADC_CTRL_WARMUPMODE_KEEPADCWARM;
        while((ADC0->STATUS & ADC_STATUS_WARM)==0) ;
    }
}

void Adc::setReference(VoltageRef ref)
{
    if(ref<0 || ref>6) return;
    ADC0->SINGLECTRL = (ADC0->SINGLECTRL & ~_ADC_SINGLECTRL_REF_MASK)
                     | ref<<_ADC_SINGLECTRL_REF_SHIFT;
}

void Adc::setAcquisitionTime(AcquisitionTime at)
{
    if(at<0 || at>8) return;
    ADC0->SINGLECTRL = (ADC0->SINGLECTRL & ~_ADC_SINGLECTRL_AT_MASK)
                     | at<<_ADC_SINGLECTRL_AT_SHIFT;
}

void Adc::setOversampling(Oversampling ovs)
{
    if(ovs<0 || ovs>12) return;
    if(ovs==0) ADC0->SINGLECTRL &= ~_ADC_SINGLECTRL_RES_MASK; //1x
    {
        ADC0->SINGLECTRL |= ADC_SINGLECTRL_RES_OVS;
        ADC0->CTRL = (ADC0->CTRL & _ADC_CTRL_OVSRSEL_MASK)
                   | (ovs-1)<<_ADC_CTRL_OVSRSEL_SHIFT;
    }
}

unsigned short Adc::readChannel(int channel)
{
    //TODO: use interrupts if conversion is too long due to oversampling
    ADC0->SINGLECTRL = (ADC0->SINGLECTRL & ~_ADC_SINGLECTRL_INPUTSEL_MASK)
                     | (channel & 0xf)<<_ADC_SINGLECTRL_INPUTSEL_SHIFT;
    ADC0->CMD=ADC_CMD_SINGLESTART;
    while((ADC0->STATUS & ADC_STATUS_SINGLEDV)==0) ;
    return ADC0->SINGLEDATA;
}

Adc::Adc()
{
    {
        miosix::FastInterruptDisableLock dLock;
        CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_ADC0;
    }

    unsigned int hfperclk=SystemCoreClock;
    //EFM32 has separate prescalers for core and peripherals, so we start
    //from HFCORECLK, work our way up to HFCLK and then down to HFPERCLK
    hfperclk*=1<<(CMU->HFCORECLKDIV & _CMU_HFCORECLKDIV_HFCORECLKDIV_MASK);
    hfperclk/=1<<(CMU->HFPERCLKDIV & _CMU_HFPERCLKDIV_HFPERCLKDIV_MASK);
    unsigned int presc=(hfperclk+13000000-1)/13000000; //Highest freq < 13MHz
    unsigned int timebase=(hfperclk+1000000-1)/1000000;
    #ifdef EFM32_HFRCO_FREQ
    // If we're running from the RC oscillator increase timebase by 12.5% and
    // at least 1 to account for frequency errors. Maybe not needed
    timebase+=std::max<unsigned int>(1,timebase/8);
    #endif

    ADC0->CTRL = timebase<<16            //TIMEBASE
               | (presc-1)<<8            //PRESC
               | ADC_CTRL_LPFMODE_RCFILT
               | ADC_CTRL_WARMUPMODE_NORMAL;
    ADC0->SINGLECTRL = ADC_SINGLECTRL_AT_256CYCLES
                     | ADC_SINGLECTRL_REF_VDD
                     | ADC_SINGLECTRL_RES_12BIT
                     | ADC_SINGLECTRL_ADJ_RIGHT;
}
