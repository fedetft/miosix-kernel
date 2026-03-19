/***************************************************************************
 *   Copyright (C) 2025 by Terraneo Federico                               *
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

#include "nrf52_gpio.h"

namespace miosix {

void GpioBase::modeImpl(NRF_GPIO_Type *p, unsigned char n, Mode m)
{
    auto cnf=static_cast<unsigned int>(m);
    // Preserve drive strength level by reading old value of PIN_CNF
    if(p->PIN_CNF[n] & (1<<8))
    {
        if(cnf & (1<<10)) cnf |= 0b01<<8;
        else cnf |= 0b11<<8;
    }
    p->PIN_CNF[n]=cnf;
}

void GpioBase::strengthImpl(NRF_GPIO_Type *p, unsigned char n, int s)
{
    auto cnf=p->PIN_CNF[n];
    // Bit 9 is dual-purpose, it means drive strength if bit 10 is 0,
    // or it means open source/drain if bit 10 is 1...
    if(s)
    {
        if(cnf & (1<<10)) cnf |= 0b01<<8;
        else cnf |= 0b11<<8;
    } else {
        if(cnf & (1<<10)) cnf &= ~(0b01<<8);
        else cnf &= ~(0b11<<8);
    }
    p->PIN_CNF[n]=cnf;
}

} //namespace miosix
