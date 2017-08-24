/***************************************************************************
 *   Copyright (C) 2017 by Matteo Michele Piazzolla                        *
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

#define PRESERVE __attribute__((section(".preserve"))) 


namespace miosix {


 enum ResetReason{
  RST_LOW_PWR=0,
  RST_WINDOW_WDG=1,
  RST_INDEPENDENT_WDG=2,
  RST_SW=3,
  RST_POWER_ON=4,
  RST_PIN=5,
  RST_UNKNOWN=6,
};


class SGM 
{
public:
    /**
     * \return an instance of this class (singleton)
     */
    static SGM& instance();
    
    void enableWrite();
    void disableWrite();
    ResetReason lastResetReason();

private:
    ResetReason last_reset;

    SGM(const SGM&);
    SGM& operator=(const SGM&);
    SGM();
    void readResetRegister();
    void clearResetFlag();

};


}