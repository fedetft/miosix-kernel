/***************************************************************************
 *   Copyright (C) 2025 - 2026 by Rogora Matteo                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#pragma once

#include "miosix_settings.h"

#ifdef PROCESS_DEBUGGER

#include "sys/types.h"
#include <miosix.h>
#include <string.h>

#include "interfaces/debugger.h"

namespace miosix {

class RegisterFile {
public:

    /**
     * @brief Copy content of register i into ref
     *
     * Destination must be big enough to store the register, register size can
     * be queried with registerSize
     *
     * @param thread Thread to reference for read
     * @param i number of register to read
     * @param ref to store value
     * @return true if valid read, false otherwise
     */
    static bool read(Thread* thread, int i, char* value);

    /**
     * @brief Writes the value pinted by ref into register i
     *
     * The number of bytes written depends on register size
     *
     * @param thread Thread to reference for write
     * @param i number of register to write
     * @param value value to write
     * @return true if valid write, false otherwise
     */
    static bool write(Thread* thread, int i, char* value);

    /**
     * @brief Return the size of register i in bytes
     *
     * @param i Index of register in register file
     */
    static int getSize(int index);

    // Number of entries in the register file
    const static int entries            = REGISTER_FILE_ENTRIES;

    // Total size of the register file in bytes
    const static int sizeBytes          = REGISTER_FILE_SIZE_BYTES;

    // Maximum size of a register in bytes
    const static int maxRegSizeBytes    = MAX_REGISTER_SIZE_BYTES;
};

typedef enum {
    OK = 0,
    FAIL,
    SPAWN_FAIL,
    ATTACH_FAIL,
    RUN_FAIL,
    MEMORY_WRITE_FAIL,
    MEMORY_READ_FAIL,
    REGISTER_READ_FAIL,
    REGISTER_WRITE_FAIL,
    BREAKPOINT_SET_FAIL,
    BREAKPOINT_FULL,
    BREAKPOINT_NOT_SUPPORTED,
} GDBReturnCode;

class GDBBuffer {
public:

    GDBBuffer() { clear(); }

    /**
     * @brief Computes chesksum of the buffer's content
     */
    inline const unsigned char checksum() const {
        unsigned char sum = 0;
        for (unsigned int i = 0; i < head; i++)
            sum += (unsigned char)data[i];
        return sum;
    };

    inline void clear() { head = 0; data[head] = '\0'; }

    /**
     * @brief Moves the head of buffer to "len" and null-terminates the string
     *
     * @param len 
     * @return the new position of head on success, -1 on fail
     */
    int setLen(unsigned int len) {
        if (size <= len) return -1;
        head = len;
        data[head] = '\0';
        return len;
    }

    /**
     * @brief Appends character to the buffer followed by '\0'
     *
     * @param character 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int appendChar(char character);

    /**
     * @brief Appends a null string to the buffer
     *
     * @param str 
     * @param len 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int appendString(const char* str, unsigned int len);

    /**
     * @brief Set the output buffer as one of the GDBReturnCode messages
     *
     * @param code 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int setReturnCode(GDBReturnCode code);

    /**
     * @brief Appends the value in the output buffer, null terminated
     *
     * @param value 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int appendRegister32(unsigned int value);

    /**
     * @brief Appends the value in the output buffer, null terminated
     *
     * @param value 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int appendRegister64(unsigned long long value);

    /**
     * @brief Append `bytes` bytes to the output buffer
     *
     * Prints the bytes from low to high addresses, each byte is represented as
     * two hex characters, padding with 0 if the byte is shorter than two characters.
     *
     * @param addr beginning address of bytes in memory
     * @param bytes number of bytes to append
     * @returns the length of the appended string, 0 on fail
     */
    int appendBytes(const unsigned int* addr, unsigned int size);

    inline int appendBytes(const char addr[]) {
        return appendBytes(reinterpret_cast<const unsigned int*>(addr), strlen(addr));
    }

    /**
     * @brief Returns the number of bytes still available
     *
     * @return 
     */
    const int available() {return size - head; }

    /**
     * @brief Returns the content of the buffer
     *
     * @return 
     */
    inline char* getData() { return data; }

    /**
     * @brief Returns the length of buffer's content
     *
     * @return 
     */
    inline const unsigned int len() const {return head; }
    
    // Must have enough space to fit Register filein a single message
    // (size of registerfile) * 2 (hex encoding) + 1 (null terminator) 
    static const unsigned int size = MINIMUM_GDB_BUFFER_SIZE;

private:
    char         data[size];
    unsigned int head;

    static_assert(size >= RegisterFile::sizeBytes * 2 + 2,
        "GDBBuffers must be large enough to store a 'G' packet:\n'G' + all registers with hex encoding (2 hex per byte) + \\0");
};

enum class StopReason {
    NONE,                   // No event
    DEBUGEVENT,             // Debug event triggered
    EXIT,                   // Exited normally, code is return value
    FAULT,                  // Terminated,      code is the fault reason
    EXECVE,                 // Execve called
};

class DebugEvent {
public:
    StopReason reason = StopReason::NONE;
    unsigned int code = 0;

    inline void IRQclear() {
        this->reason = StopReason::NONE;
        this->code = 0;
    };

    inline void IRQset(StopReason reason, unsigned int code) {
        this->reason = reason;
        this->code = code;
    }
};

/**
 * @class AttachedProcessInfo
 * @brief Contains information about the process currently being debugged
 *
 * Used in communication between Process & Thread class, DebugMon_Handler and
 * Debugger class
 *
 */
class AttachedProcessInfo {
public:

    const char*     name        = nullptr;
    Process*        process     = nullptr;
    Thread*         thread      = nullptr;
    bool            debugState  = true;
    DebugEvent      event;

    inline void IRQclear() {
        process     = nullptr;
        name        = nullptr;
        thread      = nullptr;
        debugState  = true;
        event.IRQclear();
    }

    inline bool noProgram() {
        return (process == nullptr
            ||  thread  == nullptr);
    }

};

/**
 * @class VMessage
 * @brief Tag for 'v' messages
 *
 */
class VMessage {
public:
    enum {
        VRUN,
        VATTACH,
        NONE,
    } type = NONE;
    char* args;
};

/**
 * @class QMessage
 * @brief Tag for 'q' messages
 *
 */
struct QMessage {
public:
    enum {
        SUPPORTED,
        OFFSETS,
        MEMORY_MAP,
        FEATURES,
        NONE,
    } type = NONE;
    unsigned int offset, length;

    /**
     * @brief Restricts qMessage's offset and length to a valid view of str
     *
     * @param size 
     * @return true if more data is available, false otherwise
     */
    inline bool xmlView(unsigned int size) {
        if (offset > size) {
            length = 0; return false;
        }
        if (offset + length > size) {
            length = size - offset; return false;
        }
        return true;
    }
};

/**
 * @class GDBFeatures
 * @brief Flags for supported GDB features
 *
 */
class GDBFeatures {
public:
    typedef enum : unsigned int {
        EXEC_EVENTS,
    } Name;

    /**
     * @brief Set featurea as supported
     *
     * @param feat 
     */
    void set(Name feat) { features |= (1 << feat); }

    /**
     * @brief Check if feature is supported
     *
     * @param feat 
     * @return 
     */
    bool supported(Name feat) { return features & (1 << feat); }

private:
    unsigned int features = 0;
};

/**
 * @class Debugger
 * @brief Handles communication with GDB
 *
 */
class Debugger {
public:

    void listen(int serial);
    void listen(char* serialName);

    static inline void fail() { failed = true; }

private:

    // FileNo forcommunication with GDB
    int serial = -1;

    // To start and stop process
    GDBBuffer buffer;
    GDBFeatures features;

    // Share information between Debugger and other modules
    static AttachedProcessInfo attached;
    static Thread* thread;

    static bool failed;
    static int needJoin; // < Cout of processes spawned by Debugger yet to join

    //Nedds attached
    friend void DebugMon_Handler();
    //Needs attached
    friend class Process;
    //Needs attached
    friend class Thread;
    friend class BreakpointUnit;
    friend class ProgramCache;

    void recvPacket();
    void sendPacket();

    // Handle specific commands
    void handleCommand();
    void handleCommand_cs();
    void handleCommand_D();
    void handleCommand_gG();
    void handleCommand_mM();
    void handleCommand_pP();
    void handleCommand_q();
    void handleCommand_v();
    void handleCommand_zZ();

    void stopReply();

    void processCleanup();

    /**
     * @brief Waits for the attached process to enter debugstate
     */
    inline void waitAttached() {
        // TODO: multithreaded-processes debugger: need a method to wait on 
        // attached.process->IRQdebugState(), in order to wait for all threads to stop,
        // without relying on Process structure, which may be freed (e.g.: event == EXIT)
        FastGlobalIrqLock dLock;
        // While process is not in debugstate (at least one thread running)
        while(attached.debugState == false) {
            // TODO: to implement process kill, should peek one character from
            // gdbserial if available, matching 0x03
            Thread::IRQglobalIrqUnlockAndWait(dLock);
        }
    }

    void vrun();
    void vattach();
    void parsePacket_v(VMessage* vMessage);
    void parsePacket_q(QMessage* qMessage);

};


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

    // NOTE: all methods here should actually be IRQmethods, as the data is also
    // accessed inside an IRQfunction (IRQsyncLocal, IRQhandleResched etc.)
    // Correct behavior is to acquire GlobalLock inside BreakpointUnit to make
    // sure it's synched
    // NOTE: However, this is working in stop-mode, interrupt handlers never
    // access this structure concurrently with debugger (only accessed when
    // thread can be scheduled again, i.e.: when I call debugWakup() on it)
    // ?? Can I avoid acquiring locks?: I know no thread of this process can be
    // scheduled if I'm actively modifying breakpoints

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
        // If the address lays outside the space addressable by the comparator,
        // do not attempt breakpoint insertion
        if(validBPUAddress(address) == false) return -1;
        for(int i=0; i<breakpointsNum; i++) {
            if(!breakpoints[i].enabled()) {
                breakpoints[i] = Breakpoint(address, kind);
                {
                    // NOTE: (same consideration for Breakpoint)
                    // scoping lock just to this line is basically useless,
                    // should take the lock for the whole function, but I am
                    // working in stop mode (and single threaded): BPU is
                    // accessed by other components only when scheduling an
                    // attached process, if this is executing no attached thread
                    // can be scheduled
                    //
                    // Correct behavior is: make whole function locked or remove
                    // lock entirely
                    FastGlobalIrqLock dLock;
                    IRQmarkDirty();
                }
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
                {
                    FastGlobalIrqLock dLock;
                    IRQmarkDirty();
                }
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
    static int addWatchpoint(unsigned int address, unsigned int kind, WatchpointType type) {
        for(int i=0; i<watchpointsNum; i++) {
            if(!watchpoints[i].enabled()) {
                watchpoints[i] = Watchpoint(address, kind, type);
                {
                    FastGlobalIrqLock dLock;
                    IRQmarkDirty();
                }
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
    static int removeWatchpoint(unsigned int address, unsigned int kind, WatchpointType type) {
        Watchpoint watchpoint(address, kind, type);
        for(int i=0; i<watchpointsNum; i++) {
            if (watchpoints[i].eq(watchpoint)) {
                watchpoints[i].remove();
                {
                    FastGlobalIrqLock dLock;
                    IRQmarkDirty();
                }
                return i;
            }
        }
        return -1;
    }
    
    /**
     * @brief Clear Flashpatch Unit to a new state
     */
    static void clear() {
        for (int i = 0; i < breakpointsNum; i++)
            breakpoints[i].remove();
        for (int i = 0; i < watchpointsNum; i++)
            watchpoints[i].remove();
        {
            FastGlobalIrqLock dLock;
            IRQmarkDirty();
        }
    }

    typedef unsigned char BPUFlag;

    // Make sure that the type used to store BreakpointUnit cpu flags is wide
    // enough to accomodate all available cpus
    static_assert(CPU_NUM_CORES < sizeof(BPUFlag) * 8,
            "BreakpointUnit: too many CPUs for dirty flag implementation");
    static inline BPUFlag IRQcpuDirty(unsigned int coreId) {
        return dirty & (1 << coreId);
    }

    static inline void IRQclearDirtyBit(unsigned int coreId) {
        dirty &= ~(1 << coreId);
    }

    static inline void IRQmarkDirty() {
        dirty = ~(BPUFlag)0x0;
    }

    // static inline int getBreakpointsNum()       { return breakpointsNum; }
    // static inline int getWatchpointsNum()       { return watchpointsNum; }

    /**
     * @brief Update FPB of the local cpu
     *
     * Needs GlobalIrqLock acquired to be consistent
     */

    #ifdef PROCESS_DEBUGGER
    static inline void IRQsyncLocal(Thread* t) {
        // Configure breakpoint unit:
        //
        // - PEND: pending exception takes precedence over all the other as it
        //   triggers as soon as interrupts are enabled
        // - STEP: need to clar PEND if previous thread was pending eventually
        //   but there is no need to disable other units, as GDB already removes
        //   all breakpoints and watchpoints before stepping
        // - RUN: clear PEND and enable comparators, disable stepping and clear pending,
        //   if cpu is dirty, update all comparators
        //
        switch(t->debugStatus) {
        case DebugStatus::PEND: {
            debugMonitorPendSet();
        } break;
        case DebugStatus::STEP: {
            debugTraceDisable();
            flashPatchDisable();
            debugMonitorPendClear();
            debugMonitorSteppingEnable();
        } break;
        case DebugStatus::RUN:
            debugTraceEnable();
            flashPatchEnable();
            debugMonitorPendClear();
            debugMonitorSteppingDisable();

            const auto coreId = getCurrentCoreId();
            // If cpu is valid: return
            if (!IRQcpuDirty(coreId)) return;
            // Update CPU debug register
            for (int i = 0; i < breakpointsNum; i++) breakpoints[i].IRQsetLocal(i);
            for (int i = 0; i < watchpointsNum; i++) watchpoints[i].IRQsetLocal(i);
            IRQclearDirtyBit(coreId);
        }
    }

    static inline void IRQdisableLocal() {
        // Disable DWT
        debugTraceDisable();
        // Dissable Flashpatch unit
        flashPatchDisable();
        // clear pending debugmonitor events
        debugMonitorPendClear();
        debugMonitorSteppingDisable();
    }

    static inline void IRQhandleResched(Thread* prev, Thread* next) {
        if (prev->flags.isInUserspace() == true
        && reinterpret_cast<Process*>(prev->getProcess()) == Debugger::attached.process
        && debugMonitorPendGet()) {
            // Thread switching out of context has a pending exception: must be
            // restored later
            prev->debugStatus = DebugStatus::PEND;
        }

        if (next->flags.isInUserspace() == true
        && reinterpret_cast<Process*>(next->getProcess()) == Debugger::attached.process) {
            // Scheduling attached process in userspace: configure local
            // breakpointUnit
            BreakpointUnit::IRQsyncLocal(next);
        } else {
            // Some other thread or attached in kernelspace: disable breakpoints
            BreakpointUnit::IRQdisableLocal();
        }
    }

    // NOTE: This is a reminder to add handle BreakpointUnit inside the
    // scheduler module if a new one is implemented
    // - Call BreakpointUnit::IRQhandleResched(prev, next) inside
    //   IRQrunScheduler passing pointers to the previously scheduled thread and
    //   the next scheduled thread
    // - Add scheduler guard definition below to suppress error message

    #if (defined(SCHED_TYPE_PRIORITY))\
    ||  (defined(SCHED_TYPE_CONTROL_BASED) &&  defined(SCHED_CONTROL_MULTIBURST))\
    ||  (defined(SCHED_TYPE_CONTROL_BASED) && !defined(SCHED_CONTROL_MULTIBURST))\
    ||  (defined(SCHED_TYPE_EDF))
    #else //Valid sched
    #error "Process debugger is enabled but current scheduler is not configured to handle BreakpointUnit"
    #endif //Valid sched
    #endif //defined(PROCESS_DEBUGGER)

private:

    static int breakpointsNum,
               watchpointsNum;

    static Breakpoint* breakpoints;
    static Watchpoint* watchpoints;

    static unsigned int dirty;
    
    static const unsigned int mask, writeMask, revision;

};

}

#endif

