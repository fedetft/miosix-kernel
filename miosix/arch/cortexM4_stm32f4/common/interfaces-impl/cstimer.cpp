#include "miosix.h"
#include "interfaces/cstimer.h"
#include "interfaces/portability.h"
#include "kernel/kernel.h"
#include "kernel/logging.h"
#include <cstdlib>
using namespace miosix;
namespace miosix{
void IRQaddToSleepingList(SleepData *x);
extern SleepData *sleeping_list;
}

static long long cst_ms32time = 0; //most significant 32 bits of counter
static long long cst_ms32chkp = 0; //most significant 32 bits of check point
static bool lateIrq=false;
static SleepData csRecord;

#define CST_QUANTOM 84000*4
#define CST_ROLLOVER_INT (TIM2->SR & TIM_SR_UIF) && (TIM2->SR & TIM_SR_CC2IF)

namespace miosix_private{
    void ISR_preempt();
}
ContextSwitchTimer& ContextSwitchTimer::instance() {
    static ContextSwitchTimer _instance;
    return _instance;
}

ContextSwitchTimer::ContextSwitchTimer() {
    /*
     *  TIM2 Source Clock (from APB1) Enable
     */
    {
        InterruptDisableLock idl;
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
        RCC_SYNC();
        DBGMCU->APB1FZ|=DBGMCU_APB1_FZ_DBG_TIM2_STOP; //Tim2 stops while debugging
    }
    /*
     * Setup TIM2 base configuration
     * Mode: Up-counter
     * Interrupts: counter overflow, Compare/Capture on channel 1
     */
    TIM2->CR1=TIM_CR1_URS;
    TIM2->DIER=TIM_DIER_UIE | TIM_DIER_CC1IE;
    NVIC_SetPriority(TIM2_IRQn,3);
    NVIC_EnableIRQ(TIM2_IRQn);
    /*
     * Configure channel 1 as:
     * Output channel (CC1S=0)
     * No preload(OC1PE=0), hence TIM2_CCR1 can be written at anytime
     * No effect on the output signal on match (OC1M = 0)
     */
    TIM2->CCMR1 = 0;
    TIM2->CCR1 = 0;
    /*
     * TIM2 Operation Frequency Configuration: Max Freq. and longest period
     */
    TIM2->PSC = 0;
    TIM2->ARR = 0xFFFFFFFF;
    //printf("PSCFreq: %lu  PCS: %lu   ARR: %lu  CCR1: %lu  SR: %lu CNT: %lu\n",pcsfreq,TIM2->PSC,TIM2->ARR,TIM2->CCR1,TIM2->SR,TIM2->CNT);
    /*
     * Other initializations
     */
    // Set the first checkpoint interrupt
    csRecord.p = 0;
    csRecord.wakeup_time = CST_QUANTOM;
    csRecord.next = sleeping_list;
    sleeping_list = &csRecord;
    setNextInterrupt(CST_QUANTOM);
    //IRQaddToSleepingList(&csRecord); //Recursive Initialization error
    // Enable TIM2 Counter
    cst_ms32time = 0;
    TIM2->EGR = TIM_EGR_UG; //To enforce the timer to apply PSC (and other non-immediate settings)
    TIM2->CR1 |= TIM_CR1_CEN;
}

long long ContextSwitchTimer::getCurrentTick() {
    bool interrupts=areInterruptsEnabled();
    if(interrupts) disableInterrupts();
    //If overflow occurs while interrupts disabled
    /*
     * counter should be checked before rollover interrupt flag
     *
     */
    uint32_t counter = TIM2->CNT;
    if (CST_ROLLOVER_INT && counter < 0x80000000){
        long long result=(cst_ms32time | counter) + 0x100000000ll;
        if(interrupts) enableInterrupts();
        return result;
    }
    long long result=cst_ms32time | counter;
    if(interrupts) enableInterrupts();
    return result;
}

void ContextSwitchTimer::setNextInterrupt(long long tick) {
    cst_ms32chkp = tick & 0xFFFFFFFF00000000;
    TIM2->CCR1 = (uint32_t)(tick & 0xFFFFFFFF);
    if (getCurrentTick() > getNextInterrupt())
    {
        NVIC_SetPendingIRQ(TIM2_IRQn);
        lateIrq=true;
    }
}

long long ContextSwitchTimer::getNextInterrupt(){
    return cst_ms32chkp | TIM2->CCR1;
}

void __attribute__((used)) cstirqhnd(){
    //IRQbootlog("TIM2-IRQ\r\n");
    if (TIM2->SR & TIM_SR_CC1IF || lateIrq){
        //Checkpoint met
        //The interrupt flag must be cleared unconditionally whether we are in the
        //correct epoch or not otherwise the interrupt will happen even in unrelated
        //epochs and slowing down the whole system.
        TIM2->SR = ~TIM_SR_CC1IF;
        if(cst_ms32time==cst_ms32chkp || lateIrq){
            lateIrq=false;
            long long tick = ContextSwitchTimer::instance().getCurrentTick();
            //Set next checkpoint interrupt
            ////cst_prevChk += CST_QUANTOM;
            ////ContextSwitchTimer::instance().setNextInterrupt(cst_prevChk);
            //ContextSwitchTimer::instance().setNextInterrupt(
            //    ContextSwitchTimer::instance().getCurrentTick() + CST_QUANTOM);
            
            //Add next context switch time to the sleeping list iff this is a
            //context switch
            
            if (tick > csRecord.wakeup_time){
                //Remove the cs item from the sleeping list manually
                if (sleeping_list==&csRecord)
                    sleeping_list=sleeping_list->next;
                SleepData* slp = sleeping_list;
                while (slp!=NULL){
                    if (slp->next==&csRecord){
                        slp->next=slp->next->next;
                        break;
                    }
                    slp = slp->next;
                }
                //Add next cs item to the list via IRQaddToSleepingList
                //Note that the next timer interrupt is set by IRQaddToSleepingList
                //according to the head of the list!
                csRecord.wakeup_time += CST_QUANTOM;
                IRQaddToSleepingList(&csRecord); //It would also set the next timer interrupt
            }
            miosix_private::ISR_preempt();
        }

    }
    //Rollover (Update and CC2 that is always set on zero)
    //On the initial update SR = UIF (ONLY)
    if (TIM2->SR & TIM_SR_UIF){
        TIM2->SR = ~TIM_SR_UIF; //w0 clear
        cst_ms32time += 0x100000000;
    }
}

void __attribute__((naked)) TIM2_IRQHandler(){
    saveContext();
    asm volatile ("bl _Z9cstirqhndv");
    restoreContext();
}

ContextSwitchTimer::~ContextSwitchTimer() {

}
