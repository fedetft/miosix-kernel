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

namespace miosix {

// nRF chips name ports with a number, thus P0, P1, ... and provide peripheral
// base address values with the macros NRF_P0_BASE, NRF_P1_BASE, ...
// Provide shorthands for use with the Miosix Gpio class in the miosix namespace
const unsigned int P0 = NRF_P0_BASE;
const unsigned int P1 = NRF_P1_BASE;

/**
 * GPIO mode (INPUT, OUTPUT, ...)
 * \code pin::mode(Mode::INPUT);\endcode
 */
enum class Mode : unsigned int
{
    OFF                 = 0b00000000010, ///< Disconnected
    INPUT               = 0b00000000000, ///< Floating input
    INPUT_PULL_DOWN     = 0b00000000100, ///< Pulldown Input
    INPUT_PULL_UP       = 0b00000001100, ///< Pullup   Input
    OUTPUT              = 0b00000000001, ///< Push Pull Output
    OUTPUT_OS           = 0b10000000001, ///< Output open source
    OUTPUT_OD           = 0b11000000001, ///< Output open drain
    OUTPUT_OS_PULL_DOWN = 0b10000000101, ///< Output open source pulldown
    OUTPUT_OD_PULL_UP   = 0b11000001001, ///< Output open drain  pullup
};

/**
 * \internal
 * Base class to implement non template-dependent functions that, if inlined,
 * would significantly increase code size
 *
 * This is an implementation detail, application code should only use the
 * Gpio and PioPin classes
 */
class GpioBase
{
protected:
    static void modeImpl(NRF_GPIO_Type *p, unsigned char n, Mode m);
    static void strengthImpl(NRF_GPIO_Type *p, unsigned char n, int s);
};

/**
 * Unlike the Gpio class that stores the port and pin data as a compile-time
 * template parameter, the GpioPin class stores the port and pin data in RAM
 * inside the class instance variable.
 *
 * Accessing a GPIO through this class is slower than with the Gpio class,
 * but is a convenient alternative when a GPIO pin needs to be passed as a
 * parameter to a function.
 *
 * To get an instance of this class you have to start from a Gpio class and call
 * getPin() on it. You can then mix and match method calls to both classes.
 * \code
 * using greenLed=Gpio<P0,0>;
 * greenLed::mode(Mode::OUTPUT); // Calling methods of class Gpio requires ::
 * GpioPin greenLedPin=greenLed::getPin();
 * greenLedPin.high(); // Calling methods of class GpioPin requires .
 * \endcode
 */
class GpioPin : private GpioBase
{
public:
    /**
     * \internal
     * Constructor
     * \param p P0, P1, ...
     * \param n which pin (0 to 31)
     */
    GpioPin(unsigned int p, unsigned char n) : packed(p | n) {}

    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode
     */
    void mode(Mode m) { modeImpl(ptr(),getNumber(),m); }

    /**
     * Set the GPIO drive strength
     * \param strength GPIO drive strength, from 0 to 1, with 0 being the lowest
     * and 1 being the highest.
     *
     * Default for an unconfigured GPIO is 0.
     *
     * NOTE: the hardware allows to configure drive strength for only the 0 or
     * 1 level in some modes, but this feature seems of little practical utility
     * so this driver does not support it
     */
    void driveStrength(int s) { strengthImpl(ptr(),getNumber(),s); }

    /**
     * Set the pin to 1, if it is an output
     */
    void high() { ptr()->OUTSET=1<<getNumber(); }

    /**
     * Set the pin to 0, if it is an output
     */
    void low() { ptr()->OUTCLR=1<<getNumber(); }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    int value() { return (ptr()->IN & 1<<getNumber()) ? 1 : 0; }

    /**
     * \return the pin port. One of the constants P0, P1, ...
     */
    inline unsigned int getPort() const { return packed & ~0x1f; }

    /**
     * \return 0 if the GPIO belongs to P0, or 1 if it belongs to P1
     */
    inline unsigned int getPortNumber() const
    {
        if((packed & ~0x1f)==P0) return 0; else return 1;
    }

    /**
     * \return the pin number, from 0 to 31
     */
    inline unsigned char getNumber() const { return packed & 0x1f; }

private:
    /**
     * \return the GPIO struct pointer
     */
    inline NRF_GPIO_Type *ptr()
    {
        return reinterpret_cast<NRF_GPIO_Type *>(getPort());
    }

    unsigned int packed; ///< Packed pin port and number for efficiency
};

/**
 * Gpio template class
 * \param P P0, P1, ...
 * \param N which pin (0 to 31)
 * The intended use is to make a typedef to this class with a meaningful name.
 * \code
 * using greenLed=Gpio<P0,0>;
 * greenLed::mode(Mode::OUTPUT);
 * greenLed::high(); //Turn on LED
 * \endcode
 */
template<unsigned int P, unsigned char N>
class Gpio : private GpioBase
{
public:
    Gpio() = delete; //Disallow creating instances

    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode
     */
    static void mode(Mode m) { modeImpl(ptr(),getNumber(),m); }

    /**
     * Set the GPIO drive strength
     * \param strength GPIO drive strength, from 0 to 1, with 0 being the lowest
     * and 1 being the highest.
     *
     * Default for an unconfigured GPIO is 0.
     *
     * NOTE: the hardware allows to configure drive strength only for the 0 or
     * 1 level in some modes, but this feature seems of little practical utility
     * so this driver does not support it
     */
    static void driveStrength(int s) { strengthImpl(ptr(),getNumber(),s); }

    /**
     * Set the pin to 1, if it is an output
     */
    static void high() { ptr()->OUTSET=1<<getNumber(); }

    /**
     * Set the pin to 0, if it is an output
     */
    static void low() { ptr()->OUTCLR=1<<getNumber(); }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    static int value() { return (ptr()->IN & 1<<getNumber()) ? 1 : 0; }

    /**
     * \return this Gpio converted as a GpioPin class 
     */
    static GpioPin getPin() { return GpioPin(P,N); }

    /**
     * \return the pin port. One of the constants P0, P1, ...
     */
    static inline unsigned int getPort() { return P; }

    /**
     * \return 0 if the GPIO belongs to P0, or 1 if it belongs to P1
     */
    inline unsigned int getPortNumber() const
    {
        if(P==P0) return 0; else return 1;
    }
    
    /**
     * \return the pin number, from 0 to 31
     */
    static inline unsigned char getNumber() { return N; }

private:
    /**
     * \return the GPIO struct pointer
     */
    static inline NRF_GPIO_Type *ptr()
    {
        return reinterpret_cast<NRF_GPIO_Type *>(getPort());
    }
};

} //namespace miosix
