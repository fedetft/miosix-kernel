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

#include "interfaces/delays.h"
#include "board_settings.h"

namespace miosix {

static inline void delayUsImpl(unsigned int useconds)
{
    unsigned int count;
    static_assert(cpuFrequency==32000000||
                  cpuFrequency==24000000||
                  cpuFrequency==16000000, "Delays uncalibrated");
    if(cpuFrequency>16000000)
    {
        //1 wait state
        if     (cpuFrequency==32000000) count=4*useconds;
        else if(cpuFrequency==24000000) count=3*useconds;
        //In internal Flash at 32MHz each loop iteration takes exactly 0.25us
        asm volatile("    .align 2         \n" //4-byte aligned inner loop 
                     "1:  nop              \n"
                     "    nop              \n"
                     "    nop              \n"
                     "    subs  %0, %0, #1 \n"
                     "    bpl   1b         \n":"+r"(count)::"cc");
    } else {
        //0 wait state
        if(cpuFrequency==16000000) count=2*useconds;
        asm volatile("    .align 2         \n" //4-byte aligned inner loop 
                     "1:  nop              \n"
                     "    nop              \n"
                     "    nop              \n"
                     "    nop              \n"
                     "    nop              \n"
                     "    subs  %0, %0, #1 \n"
                     "    bpl   1b         \n":"+r"(count)::"cc");
    }    
}

void delayMs(unsigned int mseconds)
{
    for(unsigned int i=0;i<mseconds;i++) delayUsImpl(1000);
}

void delayUs(unsigned int useconds)
{
    if(useconds) delayUsImpl(useconds);
}

} //namespace miosix
