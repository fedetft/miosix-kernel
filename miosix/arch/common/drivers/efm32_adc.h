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

#pragma once

/**
 * Efm32 ADC driver
 * 
 * Example code
 * \code
 * {
 *      FastInterruptDisableLock dLock;
 *      //GPIO digital logic must be disabled to use as ADC input
 *      expansion::gpio0::mode(Mode::DISABLED); //GPIO0 is PD3 or ADC0_CH3
 *      expansion::gpio1::mode(Mode::OUTPUT_HIGH);
 *  }
 *  auto& adc=Adc::instance();
 *  adc.setPowerMode(Adc::OnDemand);
 *  for(;;)
 *  {
 *      iprintf("%d\n",adc.readChannel(3));
 *      Thread::sleep(500);
 *  }
 * \endcode
 */
class Adc
{
public:
    /** 
     * Possible ADC power modes
     */
    enum PowerMode
    {
        Off, On, OnDemand
    };

    enum VoltageRef
    {
        I1V25=0, I2V5=1, VDD=2, DIFF5V=3, EXTSINGLE=4, EXTDIFF2X=5, VDD2X=6
    };

    enum AcquisitionTime
    {
        c1=0, c2=1, c4=2, c8=3, c16=4, c32=5, c64=6, c128=7, c256=8
    };

    enum Oversampling
    {
        x1=0, x2=1, x4=2, x8=3, x16=4, x32=5, x64=6, x128=7, x256=8, x512=9,
        x1024=10, x2048=11, x4096=12
    };
    
    /**
     * \return an instance to the ADC (singleton)
     */
    static Adc& instance();
    
    /**
     * Change the ADC power mode
     * \param mode power mode, default is OnDemand
     */
    void setPowerMode(PowerMode mode);

    /**
     * Change ADC acquisition time
     * \param at acquisition time, default 256 cycles
     */
    void setAcquisitionTime(AcquisitionTime at);

    /**
     * Change ADC voltage reference
     * \param ref voltage reference selection, default is VDD
     */
    void setReference(VoltageRef ref);

    /**
     * Change ADC oversampling
     * \param ovs oversampling level, default is x1
     */
    void setOversampling(Oversampling ovs);
    
    /**
     * Read an ADC channel
     * \param channel channel to read
     * \return ADC sample, in the 0..4095 range
     */
    unsigned short readChannel(int channel);

public:
    Adc();
};
