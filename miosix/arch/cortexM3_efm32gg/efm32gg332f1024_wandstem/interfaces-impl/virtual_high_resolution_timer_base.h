/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VirtualHighResolutionTimerBase.h
 * Author: fabiuz
 *
 * Created on November 10, 2016, 9:28 PM
 */

#ifndef VIRTUALHIGHRESOLUTIONTIMERBASE_H
#define VIRTUALHIGHRESOLUTIONTIMERBASE_H

#include "high_resolution_timer_base.h"
#include "rtc.h"

namespace miosix{
    class VirtualHighResolutionTimerBase : public HighResolutionTimerBase{
        public:
            static VirtualHighResolutionTimerBase& instance();
            /**
             These 4 variables are used to manage the correction of the timers.
             * vhtBase (high frequency): keeps the last sync point
             * vhtSyncPointRtc (low frequency): keeps the last sync point, just a simple conversion from vhtBase
             * vhtSyncPointVht (high frequency: keeps the precise value of last sync point
             * vhtOffset (high frequency): keeps the difference between the actual time and the counter value
             */
            static long long vhtBase;
            static long long vhtSyncPointRtc;
            static long long vhtSyncPointVht;
            static long long vhtOffset;
            long long getCurrentTick();
            void setCurrentTick(long long value);
            void resync();
        private:
            VirtualHighResolutionTimerBase();
            int syncVhtRtcPeriod;
            Rtc& rtc;
    };
}


#endif /* VIRTUALHIGHRESOLUTIONTIMERBASE_H */

