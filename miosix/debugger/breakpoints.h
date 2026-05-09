#pragma once

#include "debug_registers.h"
#include "debugger_interface.h"

namespace miosix {

enum class DebugStatus {
                    //                          MON_EN      FP_EN       MON_STEP    MON_PEND    Debugger
    RUN,            // Process is running:      SET         SET         CLEAR       CLEAR       WAIT
    STEP,           // Process is stepping:     SET         CLEAR       SET         CLEAR       WAIT
    PEND,           // Debugevent is pending:   ---         ---         ---         SET         WAIT
    STOP,           // Process is stopped:      CLEAR       CLEAR       CLEAR       CLEAR       RUN
};

/**
 * @brief Check if the current instruction (given pc) is a breakpoint
 *
 * @return
 */
bool isCodeBreakpoint(unsigned int pc);

class Breakpoint {
public:
    Breakpoint() = default;

    Breakpoint(unsigned int address, unsigned int kind);

private:

    friend class BreakpointUnit;
    bool enabled();
    bool eq (const Breakpoint& other) const;
    void IRQsetLocal(int id);
    void remove();

    unsigned int value = 0;

};

class Watchpoint {
public:

    Watchpoint() = default;
    Watchpoint(unsigned int address, unsigned int kind, WatchpointType type);

private:

    friend class BreakpointUnit;
    bool enabled();
    bool eq (const Watchpoint& other) const;
    void IRQsetLocal(int id);
    void remove();

    unsigned int    address;
    unsigned int    mask;
    WatchpointType  type    = WatchpointType::NONE;

};

class SoftBreakpoint {
public:

    SoftBreakpoint() = default;
    SoftBreakpoint(unsigned int address, unsigned int kind);

private:

    friend class BreakpointUnit;
    bool enabled() { return address; }
    bool eq (const SoftBreakpoint& other) const;

    void applyPatch();
    void removePatch();
    inline void clear() { address = 0; }

    unsigned int address = 0;
    unsigned int istr;
    char kind;

};

class BreakpointUnit {
public:
    BreakpointUnit ();
    ~BreakpointUnit ();

    /**
     * @brief Sets a breakpoint at the specified address
     *
     * Does NOT check if a breakpoint already exists at the same address
     *
     * @param address 
     * @param kind Breakpoint's width
     * @return the address of the comparator holding the breakpoint, -1 if no comparator are available
     */
    static int addBreakpoint(unsigned int address, unsigned int kind) {
        for(int i=0; i<breakpointsNum; i++) {
            if(!breakpoints[i].enabled()) {
                breakpoints[i] = Breakpoint(address, kind);
                markDirty();
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief Remove a breakpoint from specified address
     *
     * @param address 
     * @param kind Breakpoint's width
     * @return the address of the comparator that was holding the breakpoint, -1 if no comparator was found
     */
    static int removeBreakpoint(unsigned int address, unsigned int kind) {
        Breakpoint breakpoint(address, kind);
        for(int i=0; i<breakpointsNum; i++) {
            if (breakpoints[i].eq(breakpoint)) {
                breakpoints[i].remove();
                markDirty();
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief Sets a watchpoint at the specified address
     *
     * Does NOT check if a watchpoint already exists at the same address
     *
     * @param address 
     * @param kind Watchpoint's width
     * @param type READ/WRITE/ACCESS
     * @return the address of the comparator holding the watchpoint, -1 if no comparator are available
     */
    int addWatchpoint(unsigned int address, unsigned int kind, WatchpointType type) {
        for(int i=0; i<watchpointsNum; i++) {
            if(!watchpoints[i].enabled()) {
                watchpoints[i] = Watchpoint(address, kind, type);
                markDirty();
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief Remove a watchpoint from specified address
     *
     * @param address 
     * @param kind Watchpoint's width
     * @param type READ/WRITE/ACCESS
     * @return the address of the comparator that was holding the watchpoint, -1 if no comparator was found
     */
    int removeWatchpoint(unsigned int address, unsigned int kind, WatchpointType type) {
        Watchpoint watchpoint(address, kind, type);
        for(int i=0; i<watchpointsNum; i++) {
            if (watchpoints[i].eq(watchpoint)) {
                watchpoints[i].remove();
                markDirty();
                return i;
            }
        }
        return -1;
    }
    
    /**
     * @brief Adds a software breakpoint at the specified address
     *
     * @param addressk 
     * @param kind 
     * @return 
     */
    int addSoftBreakpoint(unsigned int address, unsigned int kind) {
        if (kind != 2 && kind != 4) return -1;
        for(int i=0; i<softBreakpointsNum; i++) {
            if(!softBreakpoints[i].enabled()) {
                SoftBreakpoint softBreakpoint(address, kind);
                softBreakpoints[i] = softBreakpoint;
                softBreakpoint.applyPatch();
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief Remove a software breakpoint from the specified address
     *
     * @param addressk 
     * @param kind 
     * @return 
     */
    int removeSoftBreakpoint(unsigned int address, unsigned int kind) {
        if (kind != 2 && kind != 4) return -1;
        for(int i=0; i<softBreakpointsNum; i++) {
            if(!softBreakpoints[i].enabled()) {
                SoftBreakpoint softBreakpoint(address, kind);
                softBreakpoints[i] = softBreakpoint;
                softBreakpoint.applyPatch();
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief Clear Flashpatch Unit to a new state
     */
    void clear() {
        for (int i = 0; i < breakpointsNum; i++)
            breakpoints[i].remove();
        for (int i = 0; i < watchpointsNum; i++)
            watchpoints[i].remove();
        for (int i = 0; i < softBreakpointsNum; i++)
            softBreakpoints[i].clear();
        markDirty();
    }
    
    // FIXME: #if corenumber > sizeof(unsignedint) * 8 #error
    static inline unsigned int cpuDirty(unsigned int coreId) {
        return dirty & (1 << coreId);
    }

    static inline void clearDirtyBit(unsigned int coreId) {
        dirty &= ~(1 << coreId);
    }

    static inline void markDirty() {
        dirty = 0xffffffff;
    }

    static inline int getBreakpointsNum()       { return breakpointsNum; }
    static inline int getWatchpointsNum()       { return watchpointsNum; }
    static inline int getSoftBreakpointsNum()   { return softBreakpointsNum; }

    /**
     * @brief Update FPB of the local cpu
     *
     * Needs GlobalIrqLock acquired to be consistent
     */

    static inline void IRQsyncLocal(Thread* t) {
        switch(t->attached.status) {
        case DebugStatus::PEND: {
            debugMonitorPendSet();
        } break;
        case DebugStatus::STEP: {
            debugMonitorEnable();
            flashPatchDisable();
            debugMonitorSteppingEnable();
        } break;
        default:
            debugMonitorEnable();
            flashPatchEnable();
            debugMonitorSteppingDisable();
            if (!cpuDirty(getCurrentCoreId())) return;
            for (int i = 0; i < breakpointsNum; i++)
                breakpoints[i].IRQsetLocal(i);
            for (int i = 0; i < watchpointsNum; i++)
                watchpoints[i].IRQsetLocal(i);
            clearDirtyBit(getCurrentCoreId());
        }
    }

    inline void IRQdisableLocal() {
        // Disable DebugMon_handler, ignores step and dwt
        debugMonitorDisable();
        // Dissable Flashpatch unit
        flashPatchDisable();
        // clear pending debugmonitor events
        debugMonitorPendClear();
    }

private:

    static int breakpointsNum,
               watchpointsNum;
    static const int softBreakpointsNum = 8;

    static Breakpoint* breakpoints;
    static Watchpoint* watchpoints;
    static SoftBreakpoint softBreakpoints[softBreakpointsNum];

    static unsigned int dirty;
    
    static const unsigned int mask, writeMask, revision;

};

}
