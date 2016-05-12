#ifndef CSTIMER_H
#define	CSTIMER_H

namespace miosix {

#define CST_QUANTUM 84000 ///FIXME: remove

/**
 * This class is a low level interface to a hardware timer, that is used as
 * the basis for the Miosix timing infrastructure. In detail, it allows to
 * set interrupts used both for thread wakeup from sleep, and for preemption. 
 */
class ContextSwitchTimer
{
public:
    /**
     * \return an instance of this class (singleton)
     */
    static ContextSwitchTimer& instance();
    
    /**
     * Set the next interrupt.
     * Can be called with interrupts disabled or within an interrupt.
     * \param tick the time when the interrupt will be fired, in timer ticks
     */
    void IRQsetNextInterrupt(long long tick);
    
    /**
     * \return the time when the next interrupt will be fired.
     * That is, the last value passed to setNextInterrupt().
     */
    long long getNextInterrupt() const;
    
    /**
     * Could be call both when the interrupts are enabled/disabled!
     * TODO: investigate if it's possible to remove the possibility to call
     * this with IRQ disabled and use IRQgetCurrentTick() instead
     * \return the current tick count of the timer
     */
    long long getCurrentTick() const;
    
    /**
     * \return the current tick count of the timer.
     * Can only be called with interrupts disabled or within an IRQ
     */
    long long IRQgetCurrentTick() const;
    
    /**
     * \return the timer frequency in Hz
     */
    unsigned int getTimerFrequency() const
    {
        return timerFreq;
    }
    
    /**
     * Destructor
     */
    virtual ~ContextSwitchTimer();
    
private:
    /**
     * Constructor, private because it's a singleton
     */
    ContextSwitchTimer();
    
    unsigned int timerFreq;
};

} //namespace miosix

#endif //TIMER_H
