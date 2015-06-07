/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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

#ifndef SOFTWARE_I2C_H
#define	SOFTWARE_I2C_H

#include "interfaces/gpio.h"
#include "interfaces/delays.h"

namespace miosix {

/**
 * Software I2C class.
 * \param SDA SDA gpio pin. Pass a Gpio<P,N> class
 * \param SCL SCL gpio pin. Pass a Gpio<P,N> class
 * \param timeout for clock stretching, in milliseconds
 */
template <typename SDA, typename SCL, unsigned stretchTimeout=50>
class SoftwareI2C
{
public:
    /**
     * Initializes the SPI software peripheral
     */
    static void init();

    /**
     * Send a start condition
     */
    static void sendStart();

    /**
     * Send a stop condition
     */
    static void sendStop();

    /**
     * Send a byte to a device.
     * \param data byte to send
     * \return true if the device acknowledged the byte
     */
    static bool send(unsigned char data);

    /**
     * Receive a byte from a device. Always acknowledges back.
     * \return the received byte
     */
    static unsigned char recvWithAck();

    /**
     * Receive a byte from a device. Never acknowledges back.
     * \return the received byte
     */
    static unsigned char recvWithNack();

private:
    SoftwareI2C();//Disallow creating instances, class is used via typedefs
    
    /**
     * Wait if the slave asserts SCL low. This is called clock stretching.
     * \return true on success, false on timeout
     */
    static bool waitForClockStretching();
};

template <typename SDA, typename SCL, unsigned stretchTimeout>
void SoftwareI2C<SDA, SCL, stretchTimeout>::init()
{
    SDA::mode(Mode::OPEN_DRAIN);
    SCL::mode(Mode::OPEN_DRAIN);
    SDA::high();
    SCL::high();
}

template <typename SDA, typename SCL, unsigned stretchTimeout>
void SoftwareI2C<SDA, SCL, stretchTimeout>::sendStart()
{
    SDA::low();
    delayUs(3);
    SCL::low();
    delayUs(3);
}

template <typename SDA, typename SCL, unsigned stretchTimeout>
void SoftwareI2C<SDA, SCL, stretchTimeout>::sendStop()
{
    SDA::low();
    delayUs(3);
    SCL::high();
    delayUs(3);
    waitForClockStretching();
    SDA::high();
    delayUs(3);
}

template <typename SDA, typename SCL, unsigned stretchTimeout>
bool SoftwareI2C<SDA, SCL, stretchTimeout>::send(unsigned char data)
{
    for(int i=0;i<8;i++)
    {
        if(data & 0x80) SDA::high(); else SDA::low();
        delayUs(3);
        SCL::high();
        data<<=1;
        delayUs(5);//Double delay
        waitForClockStretching();
        SCL::low();
        delayUs(3);
    }
    SDA::high();
    delayUs(3);
    SCL::high();
    delayUs(3);
    bool timeout=waitForClockStretching();
    bool result=(SDA::value()==0);
    delayUs(3);
    SCL::low();
    delayUs(3);
    return result && timeout;
}

template <typename SDA, typename SCL, unsigned stretchTimeout>
unsigned char SoftwareI2C<SDA, SCL, stretchTimeout>::recvWithAck()
{
    SDA::high();
    unsigned char result=0;
    for(int i=0;i<8;i++)
    {
        result<<=1;
        if(SDA::value()) result |= 1;
        delayUs(3);
        SCL::high();
        delayUs(5);//Double delay
        waitForClockStretching();
        SCL::low();
        delayUs(3);
    }
    SDA::low();
    delayUs(3);
    SCL::high();
    delayUs(5);//Double delay
    waitForClockStretching();
    SCL::low();
    delayUs(3);
    return result;
}

template <typename SDA, typename SCL, unsigned stretchTimeout>
unsigned char SoftwareI2C<SDA, SCL, stretchTimeout>::recvWithNack()
{
    SDA::high();
    unsigned char result=0;
    for(int i=0;i<8;i++)
    {
        result<<=1;
        if(SDA::value()) result |= 1;
        delayUs(3);
        SCL::high();
        delayUs(5);//Double delay
        waitForClockStretching();
        SCL::low();
        delayUs(3);
    }
    delayUs(3);
    SCL::high();
    delayUs(5);//Double delay
    waitForClockStretching();
    SCL::low();
    delayUs(3);
    return result;
}

template <typename SDA, typename SCL, unsigned stretchTimeout>
bool SoftwareI2C<SDA, SCL, stretchTimeout>::waitForClockStretching()
{
    if(SCL::value()==1) return true;
    for(unsigned int i=0;i<stretchTimeout;i++)
    {
        Thread::sleep(1);
        if(SCL::value()==1)
        {
            delayUs(4); //Need to wait at least 4us from rising edge of SCK
            return true;
        }
    }
    return false;
}

} //namespace miosix

#endif	//SOFTWARE_I2C_H
