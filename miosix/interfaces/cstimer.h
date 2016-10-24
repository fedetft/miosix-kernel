#ifndef CSTIMER_H
#define	CSTIMER_H

namespace miosix {

class ContextSwitchTimerImpl;

/**
 * This class is a low level interface to a hardware timer, that is used as
 * the basis for the Miosix timing infrastructure. In detail, it allows to
 * set interrupts used both for thread wakeup from sleep, and for preemption.
 * Please note that although the HW timer may operate in ticks, the class should
 * provide the user (which is kernel/scheduler codes) with a timing scheme
 * in nanoseconds. It is highly recommended to use TimeConversion class for
 * this purpose.
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
     * \param tick the time when the interrupt will be fired, in nanoseconds
     */
    void IRQsetNextInterrupt(long long ns);
    
    /**
     * \return the time when the next interrupt will be fired.
     * That is, the last value passed to setNextInterrupt().
     */
    long long getNextInterrupt() const;
    
    /**
     * Could be call both when the interrupts are enabled/disabled!
     * TODO: investigate if it's possible to remove the possibility to call
     * this with IRQ disabled and use IRQgetCurrentTime() instead
     * \return the current tick count of the timer (in terms of nanoseconds)
     */
    long long getCurrentTime() const;
    
    /**
     * \return the current tick count of the timer (in terms of nanoseconds)
     * Can only be called with interrupts disabled or within an IRQ
     */
    long long IRQgetCurrentTime() const;
    
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
    ContextSwitchTimerImpl* pImpl;
};

} //namespace miosix

#endif //CSTIMER_H
