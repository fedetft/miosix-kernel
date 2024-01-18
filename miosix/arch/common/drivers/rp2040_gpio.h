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

class Mode
{
public:
    /**
     * GPIO pad mode (INPUT, OUTPUT, ...)
     * \code pin::mode(Mode::INPUT);\endcode
     */
    enum Mode_
    {
        DISABLED                     = 0b10000000,
        PULL_UP                      = 0b00001000,
        PULL_DOWN                    = 0b00000100,
        INPUT                        = 0b11000000,
        INPUT_PULL_UP                = 0b11001000,
        INPUT_PULL_DOWN              = 0b11000100,
        INPUT_SCHMITT_TRIG           = 0b11000010,
        INPUT_SCHMITT_TRIG_PULL_UP   = 0b11001010,
        INPUT_SCHMITT_TRIG_PULL_DOWN = 0b11000110,
        OUTPUT                       = 0b01000000,
    };
private:
    Mode(); //Just a wrapper class, disallow creating instances
};

class DriveStrength
{
public:
    /**
     * Drive strength for GPIO pads
     */
    enum DriveStrength_
    {
        HIGHER   = 3, ///< 12mA max
        HIGH     = 2, ///<  8mA max
        STANDARD = 1, ///<  4mA max
        LOW      = 0  ///<  2mA max
    };
private:
    DriveStrength(); //Just a wrapper class, disallow creating instances
};

class Function
{
public:
    /**
     * GPIO function
     */
    enum Function_
    {
        SPI     = IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SPI0_RX,
        UART    = IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_UART0_TX,
        I2C     = IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_I2C0_SDA,
        PWM     = IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PWM_A_0,
        // SIO = Single cycle IO, a silly name for normal CPU-driven GPIOs
        SIO     = IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0,
        GPIO    = SIO,
        PIO0    = IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO0_0,
        PIO1    = IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO1_0,
        // Host USB VDD monitoring
        USBMON  = IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_USB_MUXING_OVERCURR_DETECT,
    };
private:
    Function(); //Just a wrapper class, disallow creating instances
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
    GpioPin(unsigned int port, unsigned char n): P(port), N(n) {}
    
    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode_
     */
    void mode(Mode::Mode_ m)
    {
        padsbank0_hw->io[N] = (padsbank0_hw->io[N] & ~0b11001110) | m;
    }

    /**
     * Set the speed of the GPIO to fast.
     */
    void fast() { hw_set_bits(&padsbank0_hw->io[N], 1); }

    /**
     * Set the speed of the GPIO to slow.
     */
    void slow() { hw_clear_bits(&padsbank0_hw->io[N], 1); }

    /**
     * Set the drive strength of the GPIO.
     * \param s Desired drive strength.
     */
    void strength(DriveStrength::DriveStrength_ s)
    {
        padsbank0_hw->io[N] = (padsbank0_hw->io[N] & ~0b00110000) | s;
    }

    /**
     * Set the function for the GPIO.
     * \param f The desired function.
     * \note To use a GPIO pin directly, set the function to GPIO first.
     */
    void function(Function::Function_ f) { iobank0_hw->io[N].ctrl = f; }

    /**
     * Set the pin to 1, if it is an output
     */
    void high() { sio_hw->gpio_set = 1UL << N; }

    /**
     * Set the pin to 0, if it is an output
     */
    void low() { sio_hw->gpio_clr = 1UL << N; }
    
    /**
     * Toggle pin, if it is an output
     */
    void toggle() { sio_hw->gpio_togl = 1UL << N; }

    /**
     * Sets the value of the GPIO (high or low)
     * \param v The value (zero for low, non-zero for high)
     */
    void write(int v) { if (v) high(); else low(); }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    int value() { return !!(sio_hw->gpio_out & (1UL << N)); }
    
    /**
     * \return the pin port. One of the constants PORTA_BASE, PORTB_BASE, ...
     */
    unsigned int getPort() const { return P; }
    
    /**
     * \return the pin number, from 0 to 15
     */
    unsigned char getNumber() const { return N; }
    
private:
    const unsigned int P = GPIOA_BASE;
    const unsigned char N;
};

/**
 * Gpio template class
 * \param P GPIOA_BASE, GPIOB_BASE, ...
 * \param N which pin (0 to 29)
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
    static void mode(Mode::Mode_ m)
    {
        padsbank0_hw->io[N] = (padsbank0_hw->io[N] & ~0b11001110) | m;
    }

    /**
     * Set the speed of the GPIO to fast.
     */
    static void fast() { hw_set_bits(&padsbank0_hw->io[N], 1); }

    /**
     * Set the speed of the GPIO to slow.
     */
    static void slow() { hw_clear_bits(&padsbank0_hw->io[N], 1); }

    /**
     * Set the drive strength of the GPIO.
     * \param s Desired drive strength.
     */
    static void strength(DriveStrength::DriveStrength_ s)
    {
        padsbank0_hw->io[N] = (padsbank0_hw->io[N] & ~0b00110000) | s;
    }

    /**
     * Set the function for the GPIO.
     * \param f The desired function.
     * \note To use a GPIO pin directly, set the function to GPIO first.
     */
    static void function(Function::Function_ f)
    {
        iobank0_hw->io[N].ctrl = f;
    }

    /**
     * Set the pin to 1, if it is an output
     */
    static void high() { sio_hw->gpio_set = 1UL << N; }

    /**
     * Set the pin to 0, if it is an output
     */
    static void low() { sio_hw->gpio_clr = 1UL << N;}
    
    /**
     * Toggle pin, if it is an output
     */
    static void toggle() { sio_hw->gpio_togl = 1UL << N; }

    /**
     * Sets the value of the GPIO (high or low)
     * \param v The value (zero for low, non-zero for high)
     */
    static void write(int v) { if (v) high(); else low(); }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    static int value() { return !!(sio_hw->gpio_out & (1UL << N)); }
    
    /**
     * \return this Gpio converted as a GpioPin class 
     */
    static GpioPin getPin()
    {
        return GpioPin(P, N);
    }
    
    /**
     * \return the pin port. One of the constants GPIOA_BASE, GPIOB_BASE, ...
     */
    unsigned int getPort() const { return P; }
    
    /**
     * \return the pin number, from 0 to 29
     */
    unsigned char getNumber() const { return N; }

private:
    Gpio();//Only static member functions, disallow creating instances
};

} //namespace miosix

