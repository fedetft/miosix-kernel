#include "miosix.h"
#include "cstimer.h"
#include "kernel/logging.h"
#include <cstdlib>
using namespace miosix;

static long long cst_ms32time = 0; //most significant 32 bits of counter
static long long cst_ms32chkp = 0; //most significant 32 bits of check point
static volatile bool cst_chkProcessed;
#define CST_ROLLOVER_INT (TIM2->SR & TIM_SR_UIF) && (TIM2->SR & TIM_SR_CC2IF)
ContextSwitchTimer& ContextSwitchTimer::instance() {
    static ContextSwitchTimer _instance;
    return _instance;
}

ContextSwitchTimer::ContextSwitchTimer() {
    /*
     *  TIM2 Source Clock (from APB1) Enable
     */
    {
        FastInterruptDisableLock idl;
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
        RCC_SYNC();
    }
    /*
     * Setup TIM2 base configuration
     * Mode: Up-counter
     * Interrupts: counter overflow, Compare/Capture on channel 1
     */
    TIM2->CR1=0;
    TIM2->CCR1=0;
    TIM2->CCR2=0;
    TIM2->CCR3=0;
    TIM2->CCR4=0;
    TIM2->DIER=TIM_DIER_UIE | TIM_DIER_CC1IE;
    NVIC_SetPriority(TIM2_IRQn,1); //TO-DO: WHICH IRQ Priority to use? 0?
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
     * TIM2 Operation Frequency Configuration
     */
    uint32_t pcsfreq = SystemCoreClock;
    uint32_t apb1_div = (RCC->CFGR & RCC_CFGR_PPRE1)>>10;
    //True assumption: AHB frequency is not divided :D
    if (apb1_div & 0x4) //APB1 Divider is active
        pcsfreq /= ((apb1_div & 0x3)+1)<<1;
    
    TIM2->PSC = 0;
    TIM2->ARR = 0xFFFFFFFF;
    //printf("PSCFreq: %lu  PCS: %lu   ARR: %lu  CCR1: %lu  SR: %lu CNT: %lu\n",pcsfreq,TIM2->PSC,TIM2->ARR,TIM2->CCR1,TIM2->SR,TIM2->CNT);
    /*
     * Other initializations
     */
    // Enable TIM2 Counter
    TIM2->EGR = TIM_EGR_UG; //To enforce the timer to apply PSC (and other non-immediate settings)
    TIM2->CR1 |= TIM_CR1_CEN;
    cst_ms32time = 0;
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
    cst_chkProcessed = false;
}

long long ContextSwitchTimer::getNextInterrupt(){
    return cst_ms32chkp | TIM2->CCR1;
}

void __attribute__((used)) cstirqhnd(){
    if ((TIM2->SR & TIM_SR_CC1IF) && (cst_ms32time==cst_ms32chkp)){
        //Checkpoint met
        TIM2->SR = ~TIM_SR_CC1IF;
        IRQbootlog(" (CHK POINT) \r\n");
        cst_chkProcessed = true;
        
    }
    if (CST_ROLLOVER_INT){
        //Rollover (Update and CC2 that is always set on zero)
        //On the initial update SR = UIF (ONLY)
        TIM2->SR = ~TIM_SR_UIF; //w0 clear
        IRQbootlog(" (ROLLOVER) \r\n");
        cst_ms32time += 0x100000000;
        
    }
    /*
    if (!cst_chkProcessed && cst_ms32chkp < (cst_ms32time | TIM2->CNT) && cst_ms32time==cst_ms32chkp){
        IRQbootlog(" (CHK POINT) \r\n");
        cst_chkProcessed = true;
    }*/ 
}

void __attribute__((naked)) TIM2_IRQHandler(){
    saveContext();
    asm volatile ("bl _Z9cstirqhndv");
    restoreContext();
}

ContextSwitchTimer::~ContextSwitchTimer() {

}
