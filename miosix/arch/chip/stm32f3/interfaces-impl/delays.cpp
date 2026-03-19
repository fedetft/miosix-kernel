/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
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

//TODO: these need to be updated and recalibrated, but I don't have the board

void delayMs(unsigned int mseconds)
{
    const unsigned int count=cpuFrequency==72000000 ? 5350 :
                             cpuFrequency==72000000 ? 5350 :
                             cpuFrequency==72000000 ? 5350 :
                             cpuFrequency==72000000 ? 4010 :
                             cpuFrequency==72000000 ? 4010 :
                             2000;
    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimization
        asm volatile("           mov   r1, #0     \n"
                     "___loop_m: cmp   r1, %0     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_m  \n"::"r"(count):"r1","cc");
    }
}

void delayUs(unsigned int useconds)
{
    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimization
    static_assert(cpuFrequency!=72000000,"Not calibrated for 72MHz");
    if(cpuFrequency==72000000)
    {
        asm volatile("           mov   r2, #166   \n"//Preloop, constant delay
                     "           mov   r1, #0     \n"
                     "__loop_u2: cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   __loop_u2  \n"
                     "           mov   r1, #5     \n"//Actual loop
                     "           mul   r2, %0, r1 \n"
                     "           mov   r1, #0     \n"
                     "___loop_u: nop              \n"
                     "           nop              \n"
                     "           cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2","cc");
    } else if(cpuFrequency==56000000) {
        asm volatile("           mov   r2, #166   \n"//Preloop, constant delay
                     "           mov   r1, #0     \n"
                     "__loop_u2: cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   __loop_u2  \n"
                     "           mov   r1, #5     \n"//Actual loop
                     "           mul   r2, %0, r1 \n"
                     "           mov   r1, #0     \n"
                     "___loop_u: nop              \n"
                     "           nop              \n"
                     "           cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2","cc");
    } else if(cpuFrequency==48000000) {
        asm volatile("           mov   r2, #166   \n"//Preloop, constant delay
                     "           mov   r1, #0     \n"
                     "__loop_u2: cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   __loop_u2  \n"
                     "           mov   r1, #5     \n"//Actual loop
                     "           mul   r2, %0, r1 \n"
                     "           mov   r1, #0     \n"
                     "___loop_u: nop              \n"
                     "           nop              \n"
                     "           cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2","cc");
    } else if(cpuFrequency==36000000) {
        asm volatile("           mov   r1, #4     \n"
                     "           mul   r2, %0, r1 \n"
                     "           mov   r1, #0     \n"
                     "___loop_u: nop              \n"
                     "           nop              \n"
                     "           cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2","cc");
    } else if(cpuFrequency==24000000) {
        asm volatile("           mov   r1, #4     \n"
                     "           mul   r2, %0, r1 \n"
                     "           mov   r1, #0     \n"
                     "           nop              \n"
                     "___loop_u: cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2","cc");
    } else if(cpuFrequency==8000000) {
        asm volatile("           mov   r1, #2     \n"
                     "           mul   r2, %0, r1 \n"
                     "           mov   r1, #0     \n"
                     "___loop_u: cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2","cc");
    }
}

} //namespace miosix
