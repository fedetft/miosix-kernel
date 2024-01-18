/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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

#include "interfaces/arch_registers.h"

//There is just one GPIO port on RP2040, with 30 lines
const unsigned int GPIOA_BASE=0;

namespace miosix {

/**
 * This class just encapsulates the Mode_ enum so that the enum names don't
 * clobber the global namespace.
 */
class Mode
{
public:
    /**
     * GPIO mode (INPUT, OUTPUT, ...)
     * \code pin::mode(Mode::INPUT);\endcode
     */
    enum Mode_
    {
        INPUT                        = 0x0,
        OUTPUT                       = 0x1,
    };
private:
    Mode(); //Just a wrapper class, disallow creating instances
};

/**
 * This class allows to easiliy pass a Gpio as a parameter to a function.
 * Accessing a GPIO through this class is slower than with just the Gpio,
 * but is a convenient alternative in some cases. Also, an instance of this
 * class occupies a few bytes of memory, unlike the Gpio class.
 * 
 * To instantiate classes of this type, use Gpio<P,N>::getPin()
 * \code
 * typedef Gpio<PORTA_BASE,0> led;
 * GpioPin ledPin=led::getPin();
 * \endcode
 */
class GpioPin
{
public:
    /**
     * \internal
     * Constructor. Don't instantiate classes through this constructor,
     * rather caller Gpio<P,N>::getPin().
     * \param port port struct
     * \param n which pin (0 to 15)
     */
    GpioPin(void *port, unsigned char n) {}
    
    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode_
     */
    void mode(Mode::Mode_ m) {};

    /**
     * Set the pin to 1, if it is an output
     */
    void high() {}

    /**
     * Set the pin to 0, if it is an output
     */
    void low() {}
    
    /**
     * Toggle pin, if it is an output
     */
    void toggle() {}

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    int value() { return 0; }
    
    /**
     * \return the pin port. One of the constants PORTA_BASE, PORTB_BASE, ...
     */
    unsigned int getPort() const { return 0; }
    
    /**
     * \return the pin number, from 0 to 15
     */
    unsigned char getNumber() const { return 0; }
    
private:
};

/**
 * Gpio template class
 * \param P GPIOA_BASE, GPIOB_BASE, ...
 * \param N which pin (0 to 15)
 * The intended use is to make a typedef to this class with a meaningful name.
 * \code
 * typedef Gpio<PORTA_BASE,0> green_led;
 * green_led::mode(Mode::OUTPUT);
 * green_led::high();//Turn on LED
 * \endcode
 */
template<unsigned int P, unsigned char N>
class Gpio
{
public:
    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode_
     */
    static void mode(Mode::Mode_ m) {}

    /**
     * Set the pin to 1, if it is an output
     */
    static void high() {}

    /**
     * Set the pin to 0, if it is an output
     */
    static void low() {}
    
    /**
     * Toggle pin, if it is an output
     */
    static void toggle() {}

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    static int value() { return 0; }
    
    /**
     * \return this Gpio converted as a GpioPin class 
     */
    static GpioPin getPin()
    {
        return GpioPin(nullptr, 0);
    }
    
    /**
     * \return the pin port. One of the constants PORTA_BASE, PORTB_BASE, ...
     */
    unsigned int getPort() const { return 0; }
    
    /**
     * \return the pin number, from 0 to 15
     */
    unsigned char getNumber() const { return 0; }

private:
    Gpio();//Only static member functions, disallow creating instances
};

} //namespace miosix

