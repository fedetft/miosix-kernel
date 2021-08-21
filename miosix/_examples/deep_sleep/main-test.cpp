/***************************************************************************
 *   Copyright (C) 2019 by Marsella Daniele                                *
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
#include "miosix.h"

using namespace std;
using namespace miosix;

int main()
{
    long long int timeBefore, timeAfter;
    using sleepLed = Gpio<GPIOD_BASE, 13>;     // orange
    using deepSleepLed = Gpio<GPIOD_BASE, 15>; // blue
    sleepLed::mode(Mode::OUTPUT);
    deepSleepLed::mode(Mode::OUTPUT);
    int i=1;
    for(;;)
    {
        printf("Going into deep sleep\n");
        fflush(stdout);
        delayMs(500);
        deepSleepLed::high();
        timeBefore = getTime();
        Thread::sleep(i * 1000);
        timeAfter = getTime();
        deepSleepLed::low();
        printf("Elapsed time: %lld \n", timeAfter - timeBefore);
        {
            DeepSleepLock dl;
            printf("Going into normal sleep\n");
            fflush(stdout);
            delayMs(500);
            sleepLed::high();
            timeBefore = getTime();
            Thread::sleep(i * 1000);
            timeAfter = getTime();
            sleepLed::low();
            printf("Elapsed time: %lld \n", timeAfter - timeBefore);
        }
        i++;
    }
}
