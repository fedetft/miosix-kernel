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

#include "timeconversion.h"

namespace miosix {

/**
 * \param x 64 bit unsigned number
 * \return 32 unsigned number with the lower 32 bits of x
 */
static inline unsigned int lo(unsigned long long x) { return x & 0xffffffff; }

/**
 * \param x 64 bit unsigned number
 * \return 32 unsigned number with the upper 32 bits of x
 */
static inline unsigned int hi(unsigned long long x) { return x>>32; }

/**
 * \param a 32 bit unsigned number
 * \param b 32 bit unsigned number
 * \return a * b as a 64 unsigned number 
 */
static inline unsigned long long mul32x32to64(unsigned int a, unsigned int b)
{
    //Casts are to produce a 64 bit result. Compiles to a single asm instruction
    //in processors having 32x32 multiplication with 64 bit result
    return static_cast<unsigned long long>(a)*static_cast<unsigned long long>(b);
}

long long mul64x32d32(long long a, unsigned int bi, unsigned int bf)
{
    /*
     * The implemntation is a standard multiplication algorithm:
     *                               | 64bit         | 32bit         | Frac part
     * ----------------------------------------------------------------------
     *                               |  hi(a)        | lo(a)         | 0    x
     *                               |               | bi            | bf   =
     * ======================================================================
     *               |  hi(bi*lo(a)) |  lo(bi*lo(a)) | 0             | -
     *  hi(bi*hi(a)) | +lo(bi*hi(a)) |               |               |
     *               |               | +hi(bf*lo(a)) |  lo(bf*lo(a)) | 0
     *               | +hi(bf*hi(a)) | +lo(bf*hi(a)) |               |
     * ----------------------------------------------------------------------
     *  96bit        | 64bit         | 32bit         | Frac part
     *  (Discarded)  | <------ returned part ------> | (Disacrded)
     * 
     * Note that [hi(bi*lo(a))|lo(bi*lo(a))] and [hi(bf*hi(a))|lo(bf*hi(a))]
     * are two 64 bit numbers with the same alignment as the result, so
     * result can be rewritten as
     * bi*lo(a) + bf*hi(a) + hi(bf*lo(a)) + lo(bi*hi(a))<<32
     * 
     * The arithmetic rounding is implemented by adding one to the result if
     * lo(bf*lo(a)) has bit 31 set. That is, if the fractional part is >=0.5.
     * 
     * This code (without arithmetic rounding) takes around 30..40 clock cycles
     * on Cortex-A9 (arm/thumb2), Cortex-M3 (thumb2), Cortex-M4 (thumb2),
     * ARM7TDMI (arm), with GCC 4.7.3 and optimization levels -Os, -O2, -O3.
     * 
     * TODO: this code is *much* slower with architectures missing a 32x32
     * multiplication with 64 bit result, probably in the thousands of clock
     * cycles. An example arch is the Cortex-M0. How to deal with those arch?
     */

    //Negative values for a are not allowed, cast is safe
    unsigned long long au=static_cast<unsigned long long>(a);
    unsigned int aLo=lo(au);
    unsigned int aHi=hi(au);

    unsigned long long result=mul32x32to64(bi,aLo);
    result+=mul32x32to64(bf,aHi);

    unsigned long long bfaLo=mul32x32to64(bf,aLo);
    result+=hi(bfaLo);

    //Uncommenting this adds arithmetic rounding (round to nearest value).
    //Leaving it commented out always rounds towards lowest and makes the
    //algorithm significantly faster by not requiring a branch
    //if(bfaLo & 0x80000000) result++;

    //This is a 32x32 -> 32 multiplication (upper 32 bits disacrded), with
    //the 32 bits shifted to the upper word of a 64 bit number
    result+=static_cast<unsigned long long>(bi*aHi)<<32;

    //Caller is responsible to never call this with values that produce overflow
    return static_cast<long long>(result);
}

//
// class TimeConversion
//

void TimeConversion::setTimerFrequency(unsigned int hz)
{
    float hzf=static_cast<float>(hz);
    toNs=floatToFactor(1e9f/hzf);
    toTick=floatToFactor(hzf/1e9f);
}

TimeConversionFactor TimeConversion::floatToFactor(float x)
{
    const float twoPower32=4294967296.f; //2^32 as a float
    unsigned int i=x;
    unsigned int f=(x-i)*twoPower32;
    return TimeConversionFactor(i,f);
}

//FIXME: uncomment: TimeConversionFactor TimeConversion::toNs, TimeConversion::toTick;

//Testsuite for multiplication algorithm and factor computation. Compile with:
// g++ -std=c++11 -O2 -DTEST_ALGORITHM -o test timeconversion.cpp; ./test
#ifdef TEST_ALGORITHM

#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

#define P(x) cout<<#x<<'='<<x<<' ';

using namespace std;

const double twoPower32=4294967296.; //2^32 as a double

void test(double b, unsigned int bi, unsigned int bd, double iterations)
{
    //Recompute rounded value using those coefficients. The aim of this test
    //is to assess the multiplication algorithm, so we use integer and
    //fractional part back-converted to a long double that contains the exact
    //same value as the fixed point number
    long double br=static_cast<long double>(bd)/twoPower32
                  +static_cast<long double>(bi);
    P(bi);
    P(bd);
    //This is the error of the 32.32 representation and factor computation
    double errorPPM=(b-br)/b*1e6;
    P(errorPPM);
    cout<<endl<<"===================="<<endl;

    srand(0);
    for(int i=0;i<iterations;i++)
    {
        unsigned int aHi=rand() & 0xffff;
        unsigned int aLo=rand() & 0xffffffff;
        unsigned long long a=static_cast<unsigned long long>(aHi)<<32
                           | static_cast<unsigned long long>(aLo);
        unsigned long long result=mul64x32d32(a,bi,bd);
        //Our reference is a long double multiplication
        unsigned long long reference=static_cast<long double>(a)*br;
        assert(llabs(result-reference)<2);
    }
    cout<<endl;
}

int main()
{
    vector<double> freqs={8e6, 24e6, 48e6, 72e6, 120e6, 168e6, 180e6, 400e6};
    for(double d : freqs)
    {
        TimeConversion::setTimerFrequency(d);
        double b;
        unsigned int bi, bd;
        
        b=1e9/d;
        bi=TimeConversion::getTick2nsConversion().integerPart();
        bd=TimeConversion::getTick2nsConversion().fractionalPart();
        test(b,bi,bd,1000000);

        b=d/1e9;
        bi=TimeConversion::getNs2tickConversion().integerPart();
        bd=TimeConversion::getNs2tickConversion().fractionalPart();
        test(b,bi,bd,1000000);
    }
}

#endif //TEST_ALGORITHM

} //namespace miosix
