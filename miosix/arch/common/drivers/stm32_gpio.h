/***************************************************************************
 *   Copyright (C) 2009-2024 by Terraneo Federico                          *
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

//Provide shorthands for GPIO port names (PA instead of GPIOA_BASE)
#ifdef GPIOA_BASE
constexpr unsigned int PA = GPIOA_BASE;
#endif
#ifdef GPIOB_BASE
constexpr unsigned int PB = GPIOB_BASE;
#endif
#ifdef GPIOC_BASE
constexpr unsigned int PC = GPIOC_BASE;
#endif
#ifdef GPIOD_BASE
constexpr unsigned int PD = GPIOD_BASE;
#endif
#ifdef GPIOE_BASE
constexpr unsigned int PE = GPIOE_BASE;
#endif
#ifdef GPIOF_BASE
constexpr unsigned int PF = GPIOF_BASE;
#endif
#ifdef GPIOG_BASE
constexpr unsigned int PG = GPIOG_BASE;
#endif
#ifdef GPIOH_BASE
constexpr unsigned int PH = GPIOH_BASE;
#endif
#ifdef GPIOI_BASE
constexpr unsigned int PI = GPIOI_BASE;
#endif
#ifdef GPIOJ_BASE
constexpr unsigned int PJ = GPIOJ_BASE;
#endif
#ifdef GPIOK_BASE
constexpr unsigned int PK = GPIOK_BASE;
#endif

/**
 * GPIO mode (INPUT, OUTPUT, ...)
 * \code pin::mode(Mode::INPUT);\endcode
 */
enum class Mode
{
    INPUT                  = 0b00000, ///Input Floating          (MODE=00 TYPE=0 PUP=00)
    INPUT_PULL_UP          = 0b00001, ///Input PullUp            (MODE=00 TYPE=0 PUP=01)
    INPUT_PULL_DOWN        = 0b00010, ///Input PullDown          (MODE=00 TYPE=0 PUP=10)
    INPUT_ANALOG           = 0b11000, ///Input Analog            (MODE=11 TYPE=0 PUP=00)
    OUTPUT                 = 0b01000, ///Push Pull  Output       (MODE=01 TYPE=0 PUP=00)
    OPEN_DRAIN             = 0b01100, ///Open Drain Output       (MODE=01 TYPE=1 PUP=00)
    OPEN_DRAIN_PULL_UP     = 0b01101, ///Open Drain Output PU    (MODE=01 TYPE=1 PUP=01)
    ALTERNATE              = 0b10000, ///Alternate function      (MODE=10 TYPE=0 PUP=00)
    ALTERNATE_PULL_UP      = 0b10001, ///Alternate PullUp        (MODE=10 TYPE=0 PUP=01)
    ALTERNATE_PULL_DOWN    = 0b10010, ///Alternate PullDown      (MODE=10 TYPE=0 PUP=10)
    ALTERNATE_OD           = 0b10100, ///Alternate Open Drain    (MODE=10 TYPE=1 PUP=00)
    ALTERNATE_OD_PULL_UP   = 0b10101, ///Alternate Open Drain PU (MODE=10 TYPE=1 PUP=01)
};

/**
 * GPIO speed
 * \code pin::speed(Speed::_50MHz);\endcode
 */
enum class Speed
{
    //Device-independent defines
    LOW       = 0x0,
    MEDIUM    = 0x1,
    HIGH      = 0x2,  //Same as LOW for STM32F0/F3
    VERY_HIGH = 0x3,
#if defined(_ARCH_CORTEXM0PLUS_STM32L0) || defined(_ARCH_CORTEXM3_STM32L1)
    _400KHz = 0x0,
    _2MHz   = 0x1,
    _10MHz  = 0x2,
    _40MHz  = 0x3
#elif defined(_ARCH_CORTEXM0_STM32F0) || defined(_ARCH_CORTEXM4_STM32F3)
    _2MHz   = 0x0,
    _10MHz  = 0x1,
    _50MHz  = 0x3
#elif defined(_ARCH_CORTEXM3_STM32F2) || defined(_ARCH_CORTEXM4_STM32F4) || \
      defined(_ARCH_CORTEXM7_STM32F7) || defined(_ARCH_CORTEXM7_STM32H7)
    _2MHz   = 0x0,
    _25MHz  = 0x1,
    _50MHz  = 0x2,
    _100MHz = 0x3
#elif defined(_ARCH_CORTEXM4_STM32L4)
    _5MHz   = 0x0,
    _25MHz  = 0x1,
    _50MHz  = 0x2,
    _100MHz = 0x3
#endif
};

//Convert enum classes to their bitmask representation
inline auto toUint(Speed s) { return static_cast<unsigned int>(s); }

/**
 * \internal
 * Base class to implement non template-dependent functions that, if inlined,
 * would significantly increase code size
 */
class GpioBase
{
protected:
    static void modeImpl(unsigned int p, unsigned char n, Mode m);
    static void afImpl(unsigned int p, unsigned char n, unsigned char af);
};

/**
 * This class allows to easiliy pass a Gpio as a parameter to a function.
 * Accessing a GPIO through this class is slower than with just the Gpio,
 * but is a convenient alternative in some cases. Also, an instance of this
 * class occupies a few bytes of memory, unlike the Gpio class.
 */
class GpioPin : private GpioBase
{
public:
    /**
     * Default constructor
     * Produces an invalid pin that shall not be used. The only safe method to
     * call is isValid() which returns false on a default constructed GpioPin
     */
    GpioPin() : p(pack(PA,0xff)) {}

    /**
     * Constructor
     * \param p PA, PB, ... as #define'd in stm32f2xx.h
     * \param n which pin (0 to 15)
     */
    GpioPin(unsigned int p, unsigned char n)
        : p(pack(p, n)) {}

    /**
     * \return whether the GpioPin is valid
     */
    bool isValid() const { return getNumber()<16; }
    
    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode
     */
    void mode(Mode m)
    {
        modeImpl(getPort(),getNumber(),m);
    }
    
    /**
     * Set the GPIO speed
     * \param s speed value
     */
    void speed(Speed s)
    {
        auto ptr=getPortDevice();
        ptr->OSPEEDR=(ptr->OSPEEDR & ~(3<<(getNumber()*2))) | toUint(s)<<(getNumber()*2);
    }
    
    /**
     * Select which of the many alternate functions is to be connected with the
     * GPIO pin.
     * \param af alternate function number, from 0 to 15
     */
    void alternateFunction(unsigned char af)
    {
        afImpl(getPort(),getNumber(),af);
    }

    /**
     * Set the pin to 1, if it is an output
     */
    void high()
    {
        getPortDevice()->BSRR = 1<<getNumber();
    }

    /**
     * Set the pin to 0, if it is an output
     */
    void low()
    {
        getPortDevice()->BSRR = 1<<(getNumber()+16);
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    int value()
    {
        return (getPortDevice()->IDR & (1<<getNumber())) ? 1 : 0;
    }
    
    /**
     * \return the pin port. One of the constants PA, PB, ...
     */
    unsigned int getPort() const { return p & ~0xff; }
    
    /**
     * \return the pin number, from 0 to 15
     */
    unsigned char getNumber() const
    {
        return static_cast<unsigned char>(p & 0xff);
    }
    
private:
    inline GPIO_TypeDef *getPortDevice() const
    {
        return reinterpret_cast<GPIO_TypeDef *>(getPort());
    }
    
    static inline unsigned long pack(unsigned long p, unsigned long n)
    {
        //We use the fact that GPIO device pointers are always 16-byte aligned
        //to binpack the port number together with the pointer
        return p | n;
    }

    unsigned long p;
};

/**
 * Gpio template class
 * \param P PA, PB, ... as #define'd in stm32f2xx.h
 * \param N which pin (0 to 15)
 * The intended use is to make a typedef to this class with a meaningful name.
 * \code
 * typedef Gpio<PA,0> green_led;
 * green_led::mode(Mode::OUTPUT);
 * green_led::high();//Turn on LED
 * \endcode
 */
template<unsigned int P, unsigned char N>
class Gpio : private GpioBase
{
public:
    /**
     * \return whether the Gpio is valid
     */
    bool isValid() const { return true; }

    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode
     */
    static void mode(Mode m)
    {
        modeImpl(P,N,m);
    }
    
    /**
     * Set the GPIO speed
     * \param s speed value
     */
    static void speed(Speed s)
    {
        auto ptr=reinterpret_cast<GPIO_TypeDef*>(P);
        ptr->OSPEEDR=(ptr->OSPEEDR & ~(3<<(N*2))) | toUint(s)<<(N*2);
    }
    
    /**
     * Select which of the many alternate functions is to be connected with the
     * GPIO pin.
     * \param af alternate function number, from 0 to 15
     */
    static void alternateFunction(unsigned char af)
    {
        afImpl(P,N,af);
    }

    /**
     * Set the pin to 1, if it is an output
     */
    static void high()
    {
        reinterpret_cast<GPIO_TypeDef*>(P)->BSRR = 1<<N;
    }

    /**
     * Set the pin to 0, if it is an output
     */
    static void low()
    {
        reinterpret_cast<GPIO_TypeDef*>(P)->BSRR = 1<<(N+16);
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    static int value()
    {
        return ((reinterpret_cast<GPIO_TypeDef*>(P)->IDR & 1<<N)? 1 : 0);
    }
    
    /**
     * \return this Gpio converted as a GpioPin class 
     */
    static GpioPin getPin()
    {
        return GpioPin(P,N);
    }
    
    /**
     * \return the pin port. One of the constants PA, PB, ...
     */
    unsigned int getPort() const { return P; }
    
    /**
     * \return the pin number, from 0 to 15
     */
    unsigned char getNumber() const { return N; }

    Gpio() = delete; //Only static member functions, disallow creating instances
};

} //namespace miosix
