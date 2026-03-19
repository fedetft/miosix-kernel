/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
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

#include "board_settings.h"
#include "interfaces/delays.h"

namespace miosix {

void delayMs(unsigned int mseconds)
{
    //Note: flash wait state don't matter because of icache
    const unsigned int count=cpuFrequency/1000;
    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimizations
        asm volatile("    movs  r1, %0     \n"
                     "    .align 2         \n" //4-byte aligned inner loop
                     "1:  subs  r1, r1, #1 \n"
                     "    bpl   1b         \n"::"r"(count):"r1","cc");
    }
}

void delayUs(unsigned int useconds)
{
    const unsigned int factor=cpuFrequency/1000000;
    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimizations
    //NOTE: can't use movs because some compilers (GCC on windows but not GCC on
    //Linux) can't produce the opcode to move a 16bit immediate, go figure...
    asm volatile("    mov   r1, %1     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds),"i"(factor):"r1","cc");
}

} //namespace miosix

