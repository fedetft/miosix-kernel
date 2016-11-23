/***************************************************************************
 *   Copyright (C)  2013 by Terraneo Federico                              *
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

#include "monotonic_clock.h"
#include <algorithm>

using namespace std;

//
// class MonotonicClock
//

// ti=tie(k) + (t-t(k)) * (T+ui(k))/T = tie(k) + sgt +sgt*ui(k)/T 
// slotGlobalTime =(t-t(k)) 
long long MonotonicClock::localTime(long long slotGlobalTime)
{
    long long T=flood.getSyncPeriod();
    int ui=sync.getClockCorrection();
    unsigned long long tie=flood.getComputedFrameStart();
    //Round towards closest
    long long correction = slotGlobalTime*ui;
    int sign=correction>=0 ? 1 : -1;
    long long dividedCorrection=(correction+(sign*T/2))/T; //Round towards closest
    return (tie + max(0LL,slotGlobalTime+dividedCorrection)); 
    
    
//    long long signSlotGlobalTime = slotGlobalTime; //Conversion unsigned to signed is *required*
//    long long T=nominalPeriod; //Conversion unsigned to signed is *required*
//    int ui=sync.getClockCorrection();
//    int ei=flood.getMeasuredFrameStart()-flood.getComputedFrameStart();
//    unsigned long long tia=flood.getMeasuredFrameStart();
//    //Round towards closest
//    long long correction = slotGlobalTime*(ui-ei);
//    int sign=correction>=0 ? 1 : -1; 
//    long long dividedCorrection=(correction+(sign*T/2))/T; //Round towards closest
//    return (tia + max(0ll,signSlotGlobalTime+dividedCorrection)); 
    
}
//t=t(k)+(ti-tie(k))*T/(T+ui(k))
//slotLocalTime =(ti-tie(k))
long long MonotonicClock::globalTime(long long slotLocalTime)
{
//    #ifdef SEND_TIMESTAMPS
//    Can't have a monotonic clock with a sync scheme that overwrites the RTC
//    assert(sync.overwritesHardwareClock()==false);
//    #endif //SEND_TIMESTAMPS
//    unsigned long long T=nominalPeriod;
//    int ui=sync.getClockCorrection();
//    unsigned long long t=flood.getMeasuredFrameStart(); //FIXME
//    return (t + (slotLocalTime * T)/(T+ui));
    return 0;
}