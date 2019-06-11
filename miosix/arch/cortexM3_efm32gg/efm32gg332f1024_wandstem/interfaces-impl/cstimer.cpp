/***************************************************************************
 *   Copyright (C) 2016 by Fabiano Riccardi, Sasan                         *
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

#include "interfaces/cstimer.h"
#include "kernel/timeconversion.h"
#include "cstimer_impl.h"
#include "vht.h"
#include "virtual_clock.h"

using namespace miosix;

static TimeConversion *tc=nullptr;
static VHT *vht=nullptr;
static VirtualClock *vt=nullptr;

namespace miosix {
    
    ContextSwitchTimer& ContextSwitchTimer::instance()
    {
        static ContextSwitchTimer instance;
        return instance;
    }
    
    void ContextSwitchTimer::IRQsetNextInterrupt(long long ns){
        pImpl->b.IRQsetNextInterruptCS(pImpl->b.removeBasicCorrection(vht->corrected2uncorrected(vt->corrected2uncorrected(tc->ns2tick(ns)))));
    }
    
    long long ContextSwitchTimer::getNextInterrupt() const{
        return tc->tick2ns(vt->uncorrected2corrected(vht->uncorrected2corrected(pImpl->b.addBasicCorrection(pImpl->b.IRQgetSetTimeCS()))));
    }
    
    long long ContextSwitchTimer::getCurrentTime() const{
        return tc->tick2ns(vt->uncorrected2corrected(vht->uncorrected2corrected(pImpl->b.addBasicCorrection(pImpl->b.getCurrentTick()))));
    }
    
    long long ContextSwitchTimer::IRQgetCurrentTime() const{
        return tc->tick2ns(vt->uncorrected2corrected(vht->uncorrected2corrected(pImpl->b.addBasicCorrection(pImpl->b.IRQgetCurrentTick()))));
    }
    
    ContextSwitchTimer::~ContextSwitchTimer(){}
    
    ContextSwitchTimer::ContextSwitchTimer(){
        pImpl=new ContextSwitchTimerImpl();
        timerFreq=pImpl->b.getTimerFrequency();
        tc = new TimeConversion(timerFreq);
        vht=&VHT::instance();
        vt=&VirtualClock::instance();
        
    }
}
