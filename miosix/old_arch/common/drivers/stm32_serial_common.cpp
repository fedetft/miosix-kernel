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

#include "stm32_serial_common.h"

using namespace miosix;

#if defined(ALTFUNC_STM32F2_SPLIT)

void STM32SerialAltFunc::set(GpioPin& pin, bool pullUp) const
{
    //First we set the AF then the mode to avoid glitches
    pin.alternateFunction(lookup(pin));
    if(pullUp) pin.mode(Mode::ALTERNATE_PULL_UP);
    else pin.mode(Mode::ALTERNATE);
}

inline unsigned char STM32SerialAltFunc::lookup(GpioPin& gpio) const
{
    unsigned char port=(gpio.getPort()-GPIOA_BASE)/0x400;
    unsigned char pin=gpio.getNumber();
    int i;
    for(i=0; spans[i].port!=0 || spans[i].pin!=0; i++) {
        if(spans[i].port>port || (spans[i].port==port && spans[i].pin>pin))
            return spans[i].af;
    }
    return spans[i].af;
}

#endif
