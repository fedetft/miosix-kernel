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

//
// Tool for inspecting alternate function spans used in stm32xxx_serial.cpp
//

#include <stdio.h>

struct STM32SerialAltFunc
{
    struct Span
    {
        // when the span ends (exclusive)
        unsigned char port:8, pin:4; 
        unsigned char af:4; // alternate function ID
    };
    const Span *spans;

    unsigned char lookup(unsigned char port, unsigned char pin) const
    {
        int i;
        for(i=0; spans[i].port!=0 || spans[i].pin!=0; i++) {
            if(spans[i].port>port || (spans[i].port==port && spans[i].pin>pin))
               return spans[i].af;
        }
        return spans[i].af;
    }
};

static const STM32SerialAltFunc::Span spans[]=
    {{0,6,6},{0,13,4},{1,1,6},{1,12,4},{1,13,2},{2,0,4},{2,4,6},{2,10,2},{0,0,0}};
static const STM32SerialAltFunc af={spans};

int main(int argc, char *argv[])
{
    printf("pin |");
    for(int pin=0;pin<16;pin++) printf(" %2d",pin);
    putchar('\n');
    printf("----+");
    for(int pin=0;pin<16;pin++) printf("---");
    putchar('\n');
    
    for(int port=0;port<9;port++)
    {
        printf(" P%c |",port+'A');
        for(int pin=0;pin<16;pin++) printf(" %2d",af.lookup(port, pin));
        putchar('\n');
    }
}

