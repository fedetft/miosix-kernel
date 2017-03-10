/***************************************************************************
 *   Copyright (C) 2016 by Fabiano Riccardi                                *
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

#include "virtual_clock.h"

VirtualClock& VirtualClock::instance(){
    static VirtualClock vt;
    return vt;
}

void VirtualClock::update(long long baseTheoretical, long long baseComputed, long long clockCorrection){
        assert(syncPeriod!=0);
        //efficient way to calculate the factor T/(T+u(k))
        unsigned long long temp=(syncPeriod<<28)/(syncPeriod+clockCorrection);
        //calculate inverse of previous factor (T+u(k))/T
        unsigned long long inverseTemp = ((syncPeriod+clockCorrection)<<28)/syncPeriod;
        {
            FastInterruptDisableLock dLock;
            factorI = static_cast<unsigned int>((temp & 0x0FFFFFFFF0000000LLU)>>28);
            factorD = static_cast<unsigned int>(temp<<4);

            inverseFactorI = static_cast<unsigned int>((inverseTemp & 0x0FFFFFFFF0000000LLU)>>28);
            inverseFactorD = static_cast<unsigned int>(inverseTemp<<4);

            this->baseTheoretical=baseTheoretical;
            this->baseComputed=baseComputed;
        }
        //printf("%u %u %u %u %lld %lld\n",factorI,factorD,inverseFactorI,inverseFactorD,this->baseTheoretical, this->baseComputed);
    }