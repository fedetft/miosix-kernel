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
    #ifdef SYSCLK_FREQ_80MHz
    register const unsigned int count=20000;
    #elif defined(SYSCLK_FREQ_56MHz)
    register const unsigned int count=14000;
    #elif defined(SYSCLK_FREQ_48MHz)
    register const unsigned int count=12000;
    #elif defined(SYSCLK_FREQ_36MHz)
    register const unsigned int count=9000;
    #elif defined(SYSCLK_FREQ_24MHz)
    register const unsigned int count=6000;
    #elif defined(HSE_VALUE) && HSE_VALUE==16000000
    register const unsigned int count=4000;
    #elif defined(HSE_VALUE) && HSE_VALUE==8000000
    register const unsigned int count=2000;
    #elif defined(RUN_WITH_HSI)
    register const unsigned int count=4000;
    #elif defined(RUN_WITH_MSI)
    register const unsigned int count=1000;
    #else
    #error "Delays are uncalibrated for this clock frequency"
    #endif

    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimizations
        asm volatile("    movs  r1, %0     \n"
                     "    .align 2         \n" //4-byte aligned inner loop
                     "1:  nop              \n" //Bring the loop time to 4 cycles
                     "    subs  r1, r1, #1 \n"
                     "    bpl   1b         \n"::"r"(count):"r1");
    }
}

void delayUs(unsigned int useconds)
{
    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimizations
    #ifdef SYSCLK_FREQ_80MHz
    asm volatile("    movs  r1, #20    \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif defined(SYSCLK_FREQ_56MHz)
    asm volatile("    movs  r1, #14    \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif defined(SYSCLK_FREQ_48MHz)
    asm volatile("    movs  r1, #12    \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif defined(SYSCLK_FREQ_36MHz)
    asm volatile("    movs  r1, #9     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif defined(SYSCLK_FREQ_24MHz)
    asm volatile("    movs  r1, #6     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif defined(HSE_VALUE) && HSE_VALUE==16000000
    asm volatile("    movs  r1, #4     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif defined(HSE_VALUE) && HSE_VALUE==8000000
    asm volatile("    movs  r1, #2     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif defined(RUN_WITH_HSI)
    asm volatile("    movs  r1, #4     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif defined(RUN_WITH_MSI)
    asm volatile("    .align 2         \n" //4-byte aligned inner loop
                 "1:  nop              \n" //Bring the loop time to 4 cycles
                 "    subs  %0, %0, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #endif
}

} //namespace miosix

