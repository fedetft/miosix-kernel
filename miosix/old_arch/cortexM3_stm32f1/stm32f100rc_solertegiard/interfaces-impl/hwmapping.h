/***************************************************************************
 *   Copyright (C) 2016 by Silvano Seva                                    *
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

#ifndef HWMAPPING_H
#define	HWMAPPING_H

#include "interfaces/gpio.h"

namespace miosix {

namespace gpio {
    
    typedef Gpio<PC,10> gpio0;
    typedef Gpio<PA,15> gpio1;
    typedef Gpio<PA,14> gpio2;    
    typedef Gpio<PB,12> gpio3;
    
    typedef Gpio<PC,0> ai0;
    typedef Gpio<PC,1> ai1;
    typedef Gpio<PC,2> ai2;
    typedef Gpio<PC,3> ai3;
    typedef Gpio<PC,4> ai4;
    typedef Gpio<PC,5> ai5;
    typedef Gpio<PB,0> ai6;
    typedef Gpio<PB,1> ai7;
}        

namespace display {
       
    typedef Gpio<PB,4> lcd_e;
    typedef Gpio<PC,13> lcd_rs;
    typedef Gpio<PA,7> lcd_d4;
    typedef Gpio<PB,3> lcd_d5;
    typedef Gpio<PC,12> lcd_d6;
    typedef Gpio<PC,11> lcd_d7;    
}

namespace buttons {
    
    typedef Gpio<PA,0> btn1;
    typedef Gpio<PA,1> btn2;
    typedef Gpio<PA,2> btn3;
    typedef Gpio<PA,3> btn4;
    typedef Gpio<PA,4> btn5;
    typedef Gpio<PA,5> btn6;
    typedef Gpio<PA,6> btn7;
}

namespace spi {
    
    typedef Gpio<PB,13> sck;
    typedef Gpio<PB,14> miso;
    typedef Gpio<PB,15> mosi;
}

namespace i2c {
    //Is I2C2
    typedef Gpio<PB,10> scl;
    typedef Gpio<PB,11> sda;
}

namespace valves {
    
    typedef Gpio<PC,6> valv1;
    typedef Gpio<PC,7> valv2;
    typedef Gpio<PC,8> valv3;
    typedef Gpio<PC,9> valv4;
    typedef Gpio<PB,6> valv5;
    typedef Gpio<PB,7> valv6;
    typedef Gpio<PB,8> valv7;
    typedef Gpio<PB,9> valv8;    
}

// typedef Gpio<PA,8> 
typedef Gpio<PA,9> tx;
typedef Gpio<PA,10> rx;
// typedef Gpio<PA,11>
// typedef Gpio<PA,12>
// typedef Gpio<PA,13>
// typedef Gpio<PB,2>
typedef Gpio<PB,5> powerSw;
    
} //namespace miosix

#endif //HWMAPPING_H
