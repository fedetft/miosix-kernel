#include "interfaces/cstimer.h"
#include "interfaces/arch_registers.h"
#include "kernel/kernel.h"
#include "kernel/scheduler/timer_interrupt.h"
#include "kernel/timeconversion.h"
#include "high_resolution_timer_base.h"

using namespace miosix;

static TimeConversion *tc;

//
// class ContextSwitchTimer
//
namespace miosix {
    
    ContextSwitchTimer& ContextSwitchTimer::instance()
    {
        static ContextSwitchTimer instance;
        return instance;
    }
    
    void ContextSwitchTimer::IRQsetNextInterrupt(long long ns){
        b.IRQsetNextInterrupt1(tc->ns2tick(ns));
    }
    
    long long ContextSwitchTimer::getNextInterrupt() const
    {
        return tc->tick2ns(b.IRQgetSetTimeCCV1());
    }
    
    long long ContextSwitchTimer::getCurrentTick() const
    {
        return tc->tick2ns(b.getCurrentTick());
    }
    
    long long ContextSwitchTimer::IRQgetCurrentTick() const
    {
        return tc->tick2ns(b.IRQgetCurrentTick());
    }
    
    ContextSwitchTimer::~ContextSwitchTimer(){}
    
    ContextSwitchTimer::ContextSwitchTimer(): b(HighResolutionTimerBase::instance())
    {
        static TimeConversion stc(b.getTimerFrequency());
        tc = &stc;
    }
}