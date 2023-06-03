/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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

#warning "TODO Delays only coarsely calibrated"

namespace miosix {

void delayMs(unsigned int mseconds)
{    
    #ifdef SYSCLK_FREQ_32MHz
    register const unsigned int count=6400;
    #endif
    
    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimizations
        asm volatile("              mov   r1, #0       \n"
                     "___loop_m:    cmp   r1, %0       \n"
                     "              bge   __loop_m_exit\n"
                     "              add   r1, r1, #1   \n"
                     "              b     ___loop_m    \n"
                     "__loop_m_exit:                   \n"::"r"(count):"r1");
    }
}

void delayUs(unsigned int useconds)
{
    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimizations    
    #ifdef SYSCLK_FREQ_32MHz
    asm volatile("              mov   r1, #3       \n"
                 "              mul   r1, %0, r1   \n"
                 "              mov   r2, #0       \n"
                 "___loop_u:    cmp   r2, r1       \n"
                 "              bge   __loop_u_exit\n"
                 "              add   r2, r2, #1   \n"
                 "              b     ___loop_u    \n"
                 "__loop_u_exit:                   \n"::"r"(useconds):"r1","r2");
    #else
    #error "delayUs not implemented"
    #endif    
}

} //namespace miosix

