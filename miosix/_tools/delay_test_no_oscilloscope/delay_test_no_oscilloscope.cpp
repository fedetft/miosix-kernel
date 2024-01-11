/***************************************************************************
 *   Copyright (C) 2023, 2024 by Daniele Cattaneo                          *
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

#include <cstdio>
#include <cmath>
#include "miosix.h"

using namespace std;
using namespace miosix;

// Define this macro if interrupts cannot be left disabled for extended periods
// of time. This is necessary for platforms where the hardware timer
// would overflow more than once during a delay test, but causes a noticeable
// skew in the test results.
//#define NO_INT_DISABLE_DURING_TEST

void chronometerTest();
void delayUsTest();
void delayMsTest();
void delayUsMsContinuousTest();

int main()
{
    for (;;)
    {
        iprintf("Core clock = %ld Hz\n", SystemCoreClock);
        puts("IRQgetTime/delayUs/delayMs no-oscilloscope calibration\n"
            "Type:\n"
            " '1' for IRQgetTime/chronometer\n"
            " '2' for delayUs/IRQgetTime\n"
            " '3' for delayMs/IRQgetTime\n"
            " '4' for delayUs/delayMs/IRQgetTime (continuous)");
        char c,junk;
        do c=getchar(); while (c=='\n');
        do junk=getchar(); while (junk!='\n');
        switch(c)
        {
            case '1':
                chronometerTest();
                break;
            case '2':
                delayUsTest();
                break;
            case '3':
                delayMsTest();
                break;
            case '4':
                delayUsMsContinuousTest();
                break;
            default:
                puts("Unrecognized command");
        }
    }
}

void chronometerTest()
{
    puts("Rough accuracy check for IRQgetTime:\n"
         "  Type newline and simultaneously start a chronometer, then type\n"
         "  newline again and simultaneously stop it. Check the value measured\n"
         "  through IRQgetTime and the value shown by the chronometer.");
    getchar();
    auto start=IRQgetTime();
    getchar();
    long long delta=IRQgetTime()-start;
    long long s=delta/1000000000;
    long long frac=delta%1000000000;
    iprintf("IRQgetTime=%lld.%09lld s\n", s, frac);
}

void delayUsTest()
{
    #ifdef NO_INT_DISABLE_DURING_TEST
    puts("WARNING: results may be inaccurate due to interrupt handling\n"
         "overhead. Use delay_test (the one requiring an oscilloscope) instead!");
    #endif
    for(int i=0; i<10000; i+=100)
    {
        long long delta;
        {
            #ifndef NO_INT_DISABLE_DURING_TEST
            FastInterruptDisableLock dLock;
            #endif
            auto start=IRQgetTime();
            delayUs(i);
            delta=IRQgetTime()-start;
        }
        iprintf("IRQgetTime=%9lld delayUs=%4d", delta, i);
        if(i>0)
        {
            long long expected = (long long)i * 1000LL;
            long long error = std::abs(expected-delta);
            long long percError = (error * 1000LL) / expected;
            iprintf(" error[%%]=%3d.%01d\n", (int)(percError/10), (int)(percError%10));
        } else {
            putchar('\n');
        }
    }
}

void delayMsTest()
{
    #ifdef NO_INT_DISABLE_DURING_TEST
    puts("WARNING: results may be inaccurate due to interrupt handling\n"
         "overhead. Use delay_test (the one requiring an oscilloscope) instead!");
    #endif
    for(int i=0; i<1000; i+=50)
    {
        long long delta;
        {
            #ifndef NO_INT_DISABLE_DURING_TEST
            FastInterruptDisableLock dLock;
            #endif
            auto start=IRQgetTime();
            delayMs(i);
            delta=IRQgetTime()-start;
        }
        iprintf("IRQgetTime=%9lld delayMs=%4d", delta, i);
        if(i>0)
        {
            long long expected = (long long)i * 1000000LL;
            long long error = std::abs(expected-delta);
            long long percError = (error * 1000LL) / expected;
            iprintf(" error[%%]=%3d.%01d\n", (int)(percError/10), (int)(percError%10));
        } else {
            putchar('\n');
        }
    }
}

void delayUsMsContinuousTest()
{
    for(int i=0;;i++)
    {
        delayUsTest();
        delayMsTest();
        iprintf("Loops done: %d\n\n", i+1);
    }
}
