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

#ifndef VIRTUAL_CLOCK_H
#define VIRTUAL_CLOCK_H

#include "kernel/timeconversion.h"
#include "cassert"
#include "stdio.h"
#include "miosix.h"

using namespace miosix;

class VirtualClock {
public:
    static VirtualClock& instance();
    
    /**
     * Uncorrect a given tick value with the windows parameter
     * @param tick in HIGH frequency
     * @return  
     */
    long long corrected2uncorrected(long long tick){
        return baseComputed+fastNegMul((tick-baseTheoretical),inverseFactorI,inverseFactorD);
    }
        
    /**
     * Correct a given tick value with the windows parameter
     * @param tick in HIGH frequency
     * @return 
     */
    long long uncorrected2corrected(long long tick){
        return baseTheoretical+fastNegMul(tick-baseComputed,factorI,factorD);
    }
    
    void update(long long baseTheoretical, long long baseComputed, long long clockCorrection);
    
    void setSyncPeriod(long long syncPeriod){
        if(syncPeriod>maxPeriod) throw 0;
        this->syncPeriod=syncPeriod;
    }
    
private:
    VirtualClock(){};
    VirtualClock(const VirtualClock&)=delete;
    VirtualClock& operator=(const VirtualClock&)=delete;
    
    long long fastNegMul(long long a,unsigned int bi, unsigned int bf){
        if(a<0){
            return -mul64x32d32(-a,bi,bf);
        }else{
            return mul64x32d32(a,bi,bf);
        }
    }
    
    //Max period, necessary to guarantee the proper behaviour of runUpdate
    //They are 2^40=1099s
    const long long maxPeriod=1099511627775;
    long long syncPeriod=-1;
    long long baseTheoretical=0;
    long long baseComputed=0;
    unsigned int factorI=1;
    unsigned int factorD=0;
    unsigned int inverseFactorI=1;
    unsigned int inverseFactorD=0;
};

#endif /* VIRTUAL_CLOCK_H */

