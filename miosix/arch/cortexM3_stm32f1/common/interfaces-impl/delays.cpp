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
    #ifndef __CODE_IN_XRAM
    #ifdef SYSCLK_FREQ_72MHz
    register const unsigned int count=12000; //Flash 2 wait state
    #elif SYSCLK_FREQ_56MHz
    register const unsigned int count=9333;  //Flash 2 wait state
    #elif SYSCLK_FREQ_48MHz
    register const unsigned int count=12000; //Flash 1 wait state
    #elif SYSCLK_FREQ_36MHz
    register const unsigned int count=9000;  //Flash 1 wait state
    #elif SYSCLK_FREQ_24MHz
    register const unsigned int count=8000;  //Flash 0 wait state
    #else
    register const unsigned int count=2678;  //Flash 0 wait state
    #endif
    #else //__CODE_IN_XRAM
    //These delays are calibrated on an stm3210e-eval, and are only correct when
    //running from ram memories with similar access timings
    #ifdef SYSCLK_FREQ_72MHz
    register const unsigned int count=1889; //Linear scaling, factor 26.236
    #elif SYSCLK_FREQ_56MHz
    register const unsigned int count=1469;
    #elif SYSCLK_FREQ_48MHz
    register const unsigned int count=1259;
    #elif SYSCLK_FREQ_36MHz
    register const unsigned int count=945;
    #elif SYSCLK_FREQ_24MHz
    register const unsigned int count=630;
    #else
    register const unsigned int count=210;
    #endif
    #endif //__CODE_IN_XRAM
    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimizations
        asm volatile("    movs  r1, %0     \n"
                     "    .align 2         \n" //4-byte aligned inner loop
                     "1:  subs  r1, r1, #1 \n"
                     "    bpl   1b         \n"::"r"(count):"r1");
    }
}

void delayUs(unsigned int useconds)
{
    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimizations
    #ifndef __CODE_IN_XRAM
    #ifdef SYSCLK_FREQ_72MHz
    asm volatile("    movs  r1, #12    \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_56MHz
    asm volatile("    movs  r1, #10    \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_48MHz
    asm volatile("    movs  r1, #12    \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_36MHz
    asm volatile("    movs  r1, #9     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_24MHz
    asm volatile("    movs  r1, #8     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #else
    //+13% error for a 100us delay
    asm volatile("    movs  r1, #3     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #endif
    #else //__CODE_IN_XRAM
    //These delays are calibrated on an stm3210e-eval, and are only correct when
    //running from ram memories with similar access timings
    #ifdef SYSCLK_FREQ_72MHz
    asm volatile("    movs  r1, #2     \n"
                 "    mul   r1, r1, %0 \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  r1, r1, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_56MHz
    asm volatile("    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  %0, %0, #1 \n"
                 "    nop              \n"
                 "    bpl   1b         \n"::"r"(useconds));
    #elif SYSCLK_FREQ_48MHz
    asm volatile("    adds  %0, %0, %0, lsr 2 \n"
                 "    .align 2                \n" //4-byte aligned inner loop
                 "1:  subs  %0, %0, #1        \n"
                 "    bpl   1b                \n"::"r"(useconds));
    #elif SYSCLK_FREQ_36MHz
    asm volatile("    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  %0, %0, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds));
    #elif SYSCLK_FREQ_24MHz
    asm volatile("    adds  %0, %0, %0, lsr 2 \n"
                 "    lsrs  %0, %0, 1         \n"
                 "    .align 2                \n" //4-byte aligned inner loop
                 "1:  subs  %0, %0, #1        \n"
                 "    bpl   1b                \n"::"r"(useconds));
    #else
    //+35% error for a 100us delay
    asm volatile("    lsrs  %0, %0, 2  \n"
                 "    .align 2         \n" //4-byte aligned inner loop
                 "1:  subs  %0, %0, #1 \n"
                 "    bpl   1b         \n"::"r"(useconds));
    #endif
    #endif //__CODE_IN_XRAM
}

} //namespace miosix

