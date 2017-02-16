/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   vht.h
 * Author: fabiuz
 *
 * Created on February 14, 2017, 11:43 AM
 */

#ifndef VHT_H
#define VHT_H

#include "high_resolution_timer_base.h"
#include "kernel/kernel.h"
#include "kernel/scheduler/timer_interrupt.h"
#include "high_resolution_timer_base.h"
#include "kernel/timeconversion.h"
#include "gpio_timer.h"
#include "transceiver_timer.h"
#include "rtc.h"
#include "flopsync_vht.h"
#include "../../../../debugpin.h"
namespace miosix{
    
class VHT {
public:
    //To keep a precise track of missing sync. 
    //It is incremented in the TMR2->CC2 routine and reset in the Thread 
    static int pendingVhtSync;
    
    static bool softEnable;
    static bool hardEnable;
    
    static VHT& instance();
    
    /**
     * Get the corrected and current tick
     * @return 
     */
    long long getTick();
    
    /**
     * For debug, return the tick value read from hrtb with a basic correction
     * @return 
     */
    long long getOriginalTick(long long);
    
    void stopResyncSoft();
    void startResyncSoft();
    
private:
    VHT();
    
    static void doRun(void *arg);
    
    void loop();
};

}

#endif /* VHT_H */

