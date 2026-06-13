/***************************************************************************
 *   HR_C7000 (CK803S) busy-wait delays for modern Miosix.                  *
 *   GPL v2+ with the Miosix linking exception.                            *
 *                                                                          *
 *   Backed by the free-running DW_apb_timers ch0 (the os_timer TIME_CH),   *
 *   polled — exact, unlike a calibrated busy-loop. The same channel the    *
 *   os timer reads for timekeeping; polling its CurrentValue is read-only  *
 *   and does not perturb it. 42 MHz, measured (project_hd2_timebase).      *
 *                                                                          *
 *   PRECONDITION: ch0 must already be free-running. It is started by       *
 *   IRQosTimerInit() and (earlier) by the board boot before any delay.     *
 ***************************************************************************/

#include "interfaces/delays.h"
#include "interfaces/arch_registers.h"   // HD2_T1_CURVAL, HD2_TIMER_HZ

namespace miosix {

void delayUs(unsigned int useconds)
{
    const unsigned int countsPerUs=HD2_TIMER_HZ/1000000u;   //42
    const unsigned int target=useconds*countsPerUs;
    unsigned int last=HD2_T1_CURVAL;                         //ch0 down-counter
    unsigned int elapsed=0;
    while(elapsed<target)
    {
        unsigned int cur=HD2_T1_CURVAL;
        elapsed+=(last-cur);   //unsigned subtraction folds the down-count + 32-bit wrap
        last=cur;
    }
}

void delayMs(unsigned int mseconds)
{
    for(unsigned int i=0;i<mseconds;i++) delayUs(1000);
}

} //namespace miosix
