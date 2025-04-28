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

#include <cstdint>
#include "config/miosix_settings.h"
#include "interfaces/delays.h"

namespace miosix {

static inline __attribute__((always_inline)) void delayUsImpl(unsigned int us)
{
    static_assert(cpuFrequency==125000000
                  || cpuFrequency==133000000
                  || cpuFrequency==200000000,
                  "Unsupported clock frequency");
    if(us == 0) return;
    if(cpuFrequency==133000000)
    {
        // 7 cycles per iteration @ 133 MHz = 1/19 us per iteration
        unsigned int iterCount = us*19-2;
        asm volatile(
            ".syntax unified\n"
            ".align 2       \n"
            "1:  nop        \n"
            "    nop        \n"
            "    nop        \n"
            "    subs %0, #1\n"
            "    cmp  %0, #0\n"
            "    bne  1b    \n":"+r"(iterCount)::"cc");
    } else if(cpuFrequency==125000000 || cpuFrequency==200000000) {
        // 5 cycles per iteration
        unsigned int iterCount;
        if(cpuFrequency==125000000) iterCount=us*25-2; // 1/25 us per iteration
        else iterCount=us*40-2; // 1/40 us per iteration
        asm volatile(
            ".syntax unified\n"
            ".align 2       \n"
            "1:  nop        \n"
            "    subs %0, #1\n"
            "    cmp  %0, #0\n"
            "    bne  1b    \n":"+r"(iterCount)::"cc");
    }
}

void delayUs(unsigned int us)
{
    delayUsImpl(us);
}

void delayMs(unsigned int ms)
{
    for(; ms!=0; ms--) delayUsImpl(1000);
}

} //namespace miosix

