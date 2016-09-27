/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
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

#ifndef TIMECONVERSION_H
#define TIMECONVERSION_H

namespace miosix {

/**
 * Multiplication between a 64 bit integer and a 32.32 fixed point number,
 * 
 * The caller must guarantee that the 64 bit number is positive and that the
 * result of the multiplication fits in 64 bits. Otherwise the behaviour is
 * unspecified.
 * 
 * \param a the 64 bit integer number.
 * \param bi the 32 bit integer part of the fixed point number
 * \param bf the 32 bit fractional part of the fixed point number
 * \return the result of the multiplication. The fractional part is discarded.
 */
long long mul64x32d32(long long a, unsigned int bi, unsigned int bf);

/**
 * This class holds a 32.32 fixed point number used for time conversion
 */
class TimeConversionFactor
{
public:
    /**
     * Default constructor. Leaves the factors unintialized.
     */
    TimeConversionFactor() {}
    
    /**
     * Constructor
     * \param i integer part
     * \param f fractional part
     */
    TimeConversionFactor(unsigned int i, unsigned int f) : i(i), f(f) {}

    /**
     * \return the integer part of the fixed point number
     */
    inline unsigned int integerPart() const { return i; }
    
    /**
     * \return the fractional part of the fixed point number
     */
    inline unsigned int fractionalPart() const { return f; }

private:
    unsigned int i;
    unsigned int f;
};

/**
 * Instances of this class can be used by timer drivers to convert from ticks
 * in the timer resolution to nanoseconds and back.
 */
class TimeConversion
{
public:
    /**
     * Constructor
     * Set the conversion factors based on the tick frequency.
     * \param hz tick frequency in Hz
     */
    TimeConversion(unsigned int hz);
    
    /**
     * \param tick time point in timer ticks
     * \return the equivalent time point in the nanosecond timescale
     */
    inline long long tick2ns(long long tick)
    {
        return mul64x32d32(tick,toNs.integerPart(),toNs.fractionalPart());
    }

    /**
     * \param ns time point in nanoseconds
     * \return the equivalent time point in the timer tick timescale
     */
    inline long long ns2tick(long long ns)
    {
        return mul64x32d32(ns,toTick.integerPart(),toTick.fractionalPart());
    }
    
    /**
     * \return the conversion factor from ticks to ns
     */
    inline TimeConversionFactor getTick2nsConversion() { return toNs; }
    
    /**
     * \return the conversion factor from ns to tick
     */
    inline TimeConversionFactor getNs2tickConversion() { return toTick; }
    
private:
    
    /**
     * \param float a floar number
     * \return the number in 32.32 fixed point format
     */
    TimeConversionFactor __attribute__((noinline)) floatToFactor(float x);
    
    TimeConversionFactor toNs, toTick;
};

} //namespace miosix

#endif //TIMECONVERSION_H
