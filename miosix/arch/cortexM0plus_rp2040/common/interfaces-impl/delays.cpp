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
#include "interfaces/delays.h"

namespace miosix {

void delayMs(unsigned int ms)
{
#if CLK_SYS_FREQ == 133000000
    while (ms != 0)
    {
        uint32_t iter_count = 1000 * 19;
        asm volatile(
            ".align 2      \n"
            "1:  nop       \n"
            "    nop       \n"
            "    nop       \n"
            "    sub %0, #1\n"
            "    cmp %0, #0\n"
            "    bne 1b    \n":"+r"(iter_count)::);
        ms--;
    }
#else
    #error "Unsupported sys_clk"
#endif
}

void delayUs(unsigned int us)
{
#if CLK_SYS_FREQ == 133000000
    // 7 cycles per iteration @ 133 MHz = 1/19 us per iteration
    uint32_t iter_count = us * 19;
    asm volatile(
        ".align 2      \n"
        "1:  nop       \n"
        "    nop       \n"
        "    nop       \n"
        "    sub %0, #1\n"
        "    cmp %0, #0\n"
        "    bne 1b    \n":"+r"(iter_count)::);
#else
    #error "Unsupported sys_clk"
#endif
}

} //namespace miosix

