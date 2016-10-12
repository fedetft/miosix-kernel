/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   radio_timer.h
 * Author: fabiuz
 *
 * Created on October 11, 2016, 11:17 AM
 */

#include "high_resolution_timer_base.h"


#ifndef RADIO_TIMER_H
#define RADIO_TIMER_H

namespace miosix{
    class RadioTimer {
        public:
            static Thread *tWaitingGPIO;
            static RadioTimer& instance();
            virtual ~RadioTimer();
        private:
            RadioTimer();
            HighResolutionTimerBase& b;
    };
}



#endif /* RADIO_TIMER_H */

