/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva                                    *
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

#pragma once

#include "interfaces/gpio.h"

namespace miosix {

typedef Gpio<PA, 5>  vbat;
typedef Gpio<PA, 6>  pushbutton;
typedef Gpio<PA, 12> poweroff;

typedef Gpio<PB, 7> greenLed;
typedef Gpio<PB, 8> redLed;

namespace frontend
{
    typedef Gpio<PA, 1> meas_out;
    typedef Gpio<PA, 2> pullup_hygro;
    typedef Gpio<PA, 3> spst2_2;
    typedef Gpio<PA, 4> frontend_unk_1;
    typedef Gpio<PA, 7> heat_hum_1;

    typedef Gpio<PB, 1> frontend_unk_2;
    typedef Gpio<PB, 3> spdt1_2;
    typedef Gpio<PB, 4> spdt2_2;
    typedef Gpio<PB, 5> spdt3_2;
    typedef Gpio<PB, 6> spst1_2;
    typedef Gpio<PB, 9> heat_hum_2;

    typedef Gpio<PB, 12> pullup_temp;

    typedef Gpio<PC, 14> spst3_2;
    typedef Gpio<PC, 15> spst4_2;
}

namespace nfc
{
    typedef Gpio<PA, 11> in1;
    typedef Gpio<PA, 0>  in2;
    typedef Gpio<PB, 0>  out;
}

namespace gps
{
    typedef Gpio<PA, 9>  rxd;
    typedef Gpio<PA, 10> txd;
    typedef Gpio<PA, 15> nReset;
}

namespace spi
{
    typedef Gpio<PB, 13> sclk;
    typedef Gpio<PB, 14> miso;
    typedef Gpio<PB, 15> mosi;

    typedef Gpio<PA, 8>  csBaro;
    typedef Gpio<PB, 2>  csEeprom;
    typedef Gpio<PC, 13> csRadio;
};

} //namespace miosix
