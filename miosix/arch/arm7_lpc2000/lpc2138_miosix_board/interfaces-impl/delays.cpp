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

#include "interfaces/delays.h"

namespace miosix {

void delayMs(unsigned int mseconds)
{
    register const unsigned int count=14746;

    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // if running with a 58982400Hz CPU clock. It is written in assembler
        // to be independent on compiler optimization settings
        asm volatile("    movs  r1, %0     \n"
                     "    .align 2         \n" //4-byte aligned inner loop
                     "1:  subs  r1, r1, #1 \n" //Loop takes 4 cycles
                     "    bpl   1b         \n"::"r"(count):"r1");
    }
}

void delayUs(unsigned int useconds)
{
    // This delay has been calibrated to take x microseconds
    // if running with a 58982400Hz CPU clock. It is written in assembler
    // to be independent on compiler optimization settings
    asm volatile("    movs  r1, #15    \n"
                 "    mul   r1, %0, r1 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n" //Loop takes 4 cycles
                 "    bpl   1b         \n"::"r"(useconds):"r1");
}

} //namespace miosix

