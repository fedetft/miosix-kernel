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
     * @return 
     */
    static inline long long corrected2uncorrected(long long tick){
        return mul64x32d32((tick-HRTB::syncPointHrtTheoretical),inverseFactorI,inverseFactorD)+HRTB::syncPointHrtSlave;
    }
    
    void start();
    
    /**
     * Correct a given tick value with the windows parameter
     * @param tick
     * @return 
     */
    static inline long long uncorrected2corrected(long long tick){
        return HRTB::syncPointHrtTheoretical+fastNegMul(tick-HRTB::syncPointHrtSlave,factorI,factorD);
    }
    
    void stopResyncSoft();
    void startResyncSoft();
    
private:
    VHT();
    
    static inline long long fastNegMul(long long a,unsigned int bi, unsigned int bf){
        if(a<0){
            return -mul64x32d32(-a,bi,bf);
        }else{
            return mul64x32d32(a,bi,bf);
        }
    }
    
    static void doRun(void *arg);
    
    void loop();
    
    //Multiplicative factor VHT
    static unsigned int factorI;
    static unsigned int factorD;
    static unsigned int inverseFactorI;
    static unsigned int inverseFactorD;
};

}

#endif /* VHT_H */

