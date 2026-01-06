/***************************************************************************
 *   Copyright (C) 2010-2024 by Terraneo Federico                          *
 *   Copyright (C) 2026 by Alain Carlucci                                  *
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
    // Formula is SystemCoreClockInMhz*250
    #ifdef SYSCLK_FREQ_160MHz
    constexpr unsigned int count=40000-1;
    #elif SYSCLK_FREQ_110MHz
    constexpr unsigned int count=27500-1;
    #elif SYSCLK_FREQ_48MHz
    constexpr unsigned int count=12000-1;
    #elif SYSCLK_FREQ_24MHz
    constexpr unsigned int count=6000-1;
    #else
    #warning "Delays are uncalibrated for this clock frequency"
    #endif

    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimizations
        asm volatile("    movs  r1, %0     \n"
                     "    .align 2         \n" //4-byte aligned inner loop
                     "1:  nop              \n" //Bring the loop time to 4 cycles
                     "    subs  r1, r1, #1 \n"
                     "    bpl   1b         \n"::"r"(count):"r1","cc");
    }
}

void delayUs(unsigned int useconds)
{
    if(useconds == 0) return;
    useconds -= 1;
    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimizations
    #ifdef SYSCLK_FREQ_160MHz
    asm volatile("    movs  r1, #40    \n"
                 "    mul   r1, r1, %0 \n"
                 "    adds  r1, 32     \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1","cc");
    #elif SYSCLK_FREQ_110MHz
    asm volatile("   movs  r1, #28    \n"
                "    mul   r1, r1, %0 \n"
                "    adds  r1, 21     \n"
                "    .align 2         \n" //4-byte aligned inner loop
                "1:  nop              \n" //Bring the loop time to 4 cycles
                "    subs  r1, r1, #1 \n"
                "    bpl   1b         \n"::"r"(useconds):"r1","cc");
    #elif SYSCLK_FREQ_48MHz
    asm volatile("   movs  r1, #12    \n"
                "    mul   r1, r1, %0 \n"
                "    adds  r1, 8      \n"
                "    .align 2         \n" //4-byte aligned inner loop
                "1:  nop              \n" //Bring the loop time to 4 cycles
                "    subs  r1, r1, #1 \n"
                "    bpl   1b         \n"::"r"(useconds):"r1","cc");
    #elif SYSCLK_FREQ_24MHz
    asm volatile("   movs  r1, #6     \n"
                "    mul   r1, r1, %0 \n"
                "    adds  r1, 2      \n"
                "    .align 2         \n" //4-byte aligned inner loop
                "1:  nop              \n" //Bring the loop time to 4 cycles
                "    subs  r1, r1, #1 \n"
                "    bpl   1b         \n"::"r"(useconds):"r1","cc");
    #endif
}

} //namespace miosix

