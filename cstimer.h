#ifndef CSTIMER_H
#define	CSTIMER_H

namespace miosix{
    class ContextSwitchTimer{
    public:
        static ContextSwitchTimer& instance();
        void setNextInterrupt(long long tick); //timer tick
        long long getNextInterrupt();
        /**
         * Could be call both when the interrupts are enabled/disabled!
         * @return 
         */
        long long getCurrentTick();
        virtual ~ContextSwitchTimer();
    private:
        ContextSwitchTimer();
    };
}
#endif	/* TIMER_H */

