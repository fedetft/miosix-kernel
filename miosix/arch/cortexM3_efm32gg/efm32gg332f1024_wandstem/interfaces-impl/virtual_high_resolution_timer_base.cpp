/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VirtualHighResolutionTimerBase.cpp
 * Author: fabiuz
 * 
 * Created on November 10, 2016, 9:28 PM
 */

#include "virtual_high_resolution_timer_base.h"
#include "../../../../debugpin.h"

namespace miosix{

long long VirtualHighResolutionTimerBase::getCurrentTick(){
    FastInterruptDisableLock dLock;
    long long vhtCount=HighResolutionTimerBase::getCurrentTick();
    return vhtCount-vhtSyncPointVht+vhtBase+vhtOffset;
}

void VirtualHighResolutionTimerBase::setCurrentTick(long long value)
{
    //We don't actually overwrite the RTC in this case, as the VHT
    //is a bit complicated, so we translate between the two times at
    //the boundary of this class.
    FastInterruptDisableLock dLock;
    long long vhtCount= HighResolutionTimerBase::getCurrentTick();
    //vhtOffset+=value - (vhtCount -vhtSyncPointVht+vhtBase+vhtOffset);   
    vhtOffset=value-vhtCount+vhtSyncPointVht-vhtBase;
}
    
/**
 * This takes 17us to 48us
 */
void VirtualHighResolutionTimerBase::resync(){
    {
	FastInterruptDisableLock dLock;
	HighPin<debug1> x;
	int prev=loopback32KHzIn::value();
	for(;;){
		int curr=loopback32KHzIn::value();
		if(curr-prev==-1) break;
		prev=curr;
	}
	int high=TIMER3->CNT;
	TIMER2->CC[2].CTRL |= TIMER_CC_CTRL_MODE_INPUTCAPTURE;
	
	//Wait until we capture the first raising edge in the 32khz wave, 
	//it takes max half period, less than 16us -->732.4ticks @48000000Hz
	while((TIMER2->IF & TIMER_IF_CC2)==0) ;
	
	//Creating the proper value of vhtSyncPointVht
	int high2=TIMER3->CNT;
	TIMER2->CC[2].CTRL &= ~_TIMER_CC_CTRL_MODE_MASK;
	//This is the exact point when the rtc is incremented to the actual value 
	TIMER2->IFC =TIMER_IFC_CC2;
	if(high==high2){
	    vhtSyncPointVht = high<<16 | TIMER2->CC[2].CCV; //FIXME:: add the highest part
	}else{
	    // 10000 are really enough to be sure that we are in the new window
	    if(TIMER2->CC[2].CCV < 735){
		vhtSyncPointVht = high2<<16 | TIMER2->CC[2].CCV;
	    }else{
		vhtSyncPointVht = high<<16 | TIMER2->CC[2].CCV;
	    }
	}
	vhtSyncPointVht=TIMER3->CNT | TIMER2->CC[2].CCV;
	
	vhtSyncPointRtc=rtc.IRQgetValue();
	{
            unsigned long long conversion=vhtSyncPointRtc;
            conversion*=HighResolutionTimerBase::freq;
            conversion+=HighResolutionTimerBase::freq/2; //Round to nearest
            conversion/=HighResolutionTimerBase::freq;
            vhtBase=conversion;
        }
    }
}

VirtualHighResolutionTimerBase& VirtualHighResolutionTimerBase::instance(){
    static VirtualHighResolutionTimerBase vhrtb;
    return vhrtb;
}

VirtualHighResolutionTimerBase::VirtualHighResolutionTimerBase(): rtc(Rtc::instance()) {
    //the base class is already started
    
    TIMER2->CC[2].CTRL=TIMER_CC_CTRL_ICEDGE_RISING
			| TIMER_CC_CTRL_FILT_DISABLE
			| TIMER_CC_CTRL_INSEL_PIN
			| TIMER_CC_CTRL_MODE_OFF;
}

}//namespace miosix


