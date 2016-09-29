
#include "interfaces/cstimer.h"
#include "interfaces/arch_registers.h"
#include "kernel/kernel.h"
#include "kernel/scheduler/timer_interrupt.h"
#include "kernel/timeconversion.h"

using namespace miosix;

const unsigned int timerBits=32;
const unsigned long long overflowIncrement=(1LL<<timerBits);
const unsigned long long lowerMask=overflowIncrement-1;
const unsigned long long upperMask=0xFFFFFFFFFFFFFFFFLL-lowerMask;

static long long ms32time = 0; //most significant 32 bits of counter
static long long ms32chkp = 0; //most significant 32 bits of check point
static bool lateIrq=false;

static TimeConversion *tc;

static inline long long nextInterrupt()
{
    return ms32chkp | TIM2->CCR1;
}

static inline long long IRQgetTick()
{
    //THE PENDING BIT TRICK, version 2
    //This algorithm is the main part that allows to extend in software a
    //32bit timer to a 64bit one. The basic idea is this: the lower bits of the
    //64bit timer are kept by the counter register of the timer, while the upper
    //bits are kept in a software variable. When the hardware timer overflows, 
    //an interrupt is used to update the upper bits.
    //Reading the timer may appear to be doable by just an OR operation between
    //the software variable and the hardware counter, but is actually way
    //trickier than it seems, because user code may:
    //1 disable interrupts,
    //2 spend a little time with interrupts disabled,
    //3 call this function.
    //Now, if a timer overflow occurs while interrupts are disabled, the upper
    //bits have not yet been updated, so we would return the wrong time.
    //To fix this, we check the timer overflow pending bit, and if it is set
    //we return the time adjusted accordingly. This almost works, the last
    //issue to fix is that reading the timer counter and the pending bit
    //is not an atomic operation, and the counter may roll over exactly at that
    //point in time. To solve this, we read the timer a second time to see if
    //it had rolled over.
    //Note that this algorithm imposes a limit on the maximum time interrupts
    //can be disabeld, equals to one hardware timer period minus the time
    //between the two timer reads in this algorithm.
    unsigned int counter=TIM2->CNT;
    if((TIM2->SR & TIM_SR_UIF) && TIM2->CNT>=counter)
        return (ms32time | static_cast<long long>(counter)) + overflowIncrement;
    return ms32time | static_cast<long long>(counter);
}

void __attribute__((naked)) TIM2_IRQHandler()
{
    saveContext();
    asm volatile ("bl _Z9cstirqhndv");
    restoreContext();
}

void __attribute__((used)) cstirqhnd()
{
    if(TIM2->SR & TIM_SR_CC1IF || lateIrq)
    {
        TIM2->SR = ~TIM_SR_CC1IF;
        if(ms32time==ms32chkp || lateIrq)
        {
            lateIrq=false;
            IRQtimerInterrupt(tc->tick2ns(IRQgetTick()));
        }

    }
    //Rollover
    if(TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR = ~TIM_SR_UIF;
        ms32time += overflowIncrement;
    }
}

//
// class ContextSwitchTimer
//

namespace miosix {

ContextSwitchTimer& ContextSwitchTimer::instance()
{
    static ContextSwitchTimer instance;
    return instance;
}

void ContextSwitchTimer::IRQsetNextInterrupt(long long ns)
{
    long long tick = tc->ns2tick(ns);
    ms32chkp = tick & upperMask;
    TIM2->CCR1 = static_cast<unsigned int>(tick & lowerMask);
    if(IRQgetTick() >= nextInterrupt())
    {
        NVIC_SetPendingIRQ(TIM2_IRQn);
        lateIrq=true;
    }
}

long long ContextSwitchTimer::getNextInterrupt() const
{
    return tc->tick2ns(nextInterrupt());
}

long long ContextSwitchTimer::getCurrentTick() const
{
    bool interrupts=areInterruptsEnabled();
    //TODO: optimization opportunity, if we can guarantee that no call to this
    //function occurs before kernel is started, then we can use
    //fastInterruptDisable())
    if(interrupts) disableInterrupts();
    long long result=tc->tick2ns(IRQgetTick());
    if(interrupts) enableInterrupts();
    return result;
}

long long ContextSwitchTimer::IRQgetCurrentTick() const
{
    return tc->tick2ns(IRQgetTick());
}

ContextSwitchTimer::~ContextSwitchTimer() {}

ContextSwitchTimer::ContextSwitchTimer()
{
    // TIM2 Source Clock (from APB1) Enable
    {
        //NOTE: Not FastInterruptDisableLock as this is called before kernel
        //is started
        InterruptDisableLock idl;
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
        RCC_SYNC();
        DBGMCU->APB1FZ|=DBGMCU_APB1_FZ_DBG_TIM2_STOP; //Tim2 stops while debugging
    }
    // Setup TIM2 base configuration
    // Mode: Up-counter
    // Interrupts: counter overflow, Compare/Capture on channel 1
    TIM2->CR1=TIM_CR1_URS;
    TIM2->DIER=TIM_DIER_UIE | TIM_DIER_CC1IE;
    NVIC_SetPriority(TIM2_IRQn,3); //High priority for TIM2 (Max=0, min=15)
    NVIC_EnableIRQ(TIM2_IRQn);
    // Configure channel 1 as:
    // Output channel (CC1S=0)
    // No preload(OC1PE=0), hence TIM2_CCR1 can be written at anytime
    // No effect on the output signal on match (OC1M = 0)
    TIM2->CCMR1 = 0;
    TIM2->CCR1 = 0;
    // TIM2 Operation Frequency Configuration: Max Freq. and longest period
    TIM2->PSC = 0;
    TIM2->ARR = 0xFFFFFFFF;
    
    // Enable TIM2 Counter
    TIM2->EGR = TIM_EGR_UG; //To enforce the timer to apply PSC
    TIM2->CR1 |= TIM_CR1_CEN;
    
    // The global variable SystemCoreClock from ARM's CMSIS allows to know
    // the CPU frequency.
    timerFreq=SystemCoreClock;

    // The timer frequency may however be a submultiple of the CPU frequency,
    // due to the bus at whch the periheral is connected being slower. The
    // RCC->CFGR register tells us how slower the APB1 bus is running.
    // This formula takes into account that if the APB1 clock is divided by a
    // factor of two or greater, the timer is clocked at twice the bus
    // interface. After this, the freq variable contains the frequency in Hz
    // at which the timer prescaler is clocked.
    if(RCC->CFGR & RCC_CFGR_PPRE1_2) timerFreq/=1<<((RCC->CFGR>>10) & 0x3);
    static TimeConversion stc(timerFreq);
    tc = &stc;
}

} //namespace miosix
