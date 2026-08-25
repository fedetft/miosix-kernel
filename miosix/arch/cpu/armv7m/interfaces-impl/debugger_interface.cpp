#include "interfaces-impl/cpu_const_impl.h"
#include "interfaces/cpu_const.h"
#include "miosix_settings.h"

#ifdef PROCESS_DEBUGGER

#include "debugger/debugger.h"
#include "debugger_interface.h"
#include "kernel/process.h"

namespace miosix {

////////////////////////////////////////////////////////////////////////////////

Breakpoint::Breakpoint(unsigned int address, unsigned int kind) {
    // Revision 2: Simply set address and enable
    if (fpbGetRevisionVersion() == FP_Ctrl_REVISION_2) {
        value = address | FP_Comp_BE;
        return;
    }

    // Revision 1: Set replace mode, mask address and enable
    int mode;
    if (kind == 4)              mode = FP_Comp_Mode_BKPT_AT_X0;
    else if (address & 0x02)    mode = FP_Comp_Mode_BKPT_AT_10;
    else                        mode = FP_Comp_Mode_BKPT_AT_00;

    value = (address & FP_Comp_COMP_MASK)
          | (FP_Comp_ENABLE_MASK)
          | (mode << FP_Comp_REPLACE_SHIFT)
          ;
}

void Breakpoint::remove() { this->value = 0; }

bool Breakpoint::enabled() { return this->value & FP_Comp_ENABLE_MASK; }

bool Breakpoint::eq(const Breakpoint& other) const { return this->value == other.value; }

void Breakpoint::IRQsetLocal(int id) { FPB->FP_COMP[id] = value; }

////////////////////////////////////////////////////////////////////////////////

Watchpoint::Watchpoint(unsigned int address, unsigned int kind, WatchpointType type) {

    const auto tz = __builtin_ctz(kind);

    // Just checking, GDB should know about this
    // Make sure that required kind can be expressed by mask register
    if (kind != static_cast<unsigned int>(1 << tz)) return;

    this->address   = address;
    this->mask      = tz;
    this->type      = type;
}

void Watchpoint::remove() { this->type = WatchpointType::NONE; }

bool Watchpoint::enabled() { return this->type != WatchpointType::NONE; }

bool Watchpoint::eq (const Watchpoint& other) const {
    return this->address == other.address
        && this->mask    == other.mask
        && this->type    == other.type
        ;
}

void Watchpoint::IRQsetLocal(int id) {
    _DWT->WP[id].COMP      = address;
    _DWT->WP[id].MASK      = mask;
    _DWT->WP[id].FUNCTION  = type;
}

////////////////////////////////////////////////////////////////////////////////

int RegisterFile::getSize(int i) {
    // Invalid register number
    if (i <  0  || i >= REGISTER_FILE_ENTRIES)    return 0;
#if __FPU_PRESENT == 1
    // Floating point, double precision registers
    if (i >= d0 && i <= d15)        return 8;
#endif
    // Integer registers
                                    return 4;
}

// CTXSAVE_ON_STACK does not respect alignment, thus + 4
// If alignment is not respected, gdb will fail to backtrace call frames
static const unsigned int ctxSaveOnStackAligned = ((CTXSAVE_ON_STACK - 1) / CTXSAVE_STACK_ALIGNMENT + 1) * CTXSAVE_STACK_ALIGNMENT;
    static_assert(ctxSaveOnStackAligned % CTXSAVE_STACK_ALIGNMENT == 0,
            "ctxSaveOnStackAligned - does not respect stack alignment");

bool RegisterFile::read(Thread* t, int regNum, char *ref) {
    if (t == nullptr) return false;
    if (getSize(regNum) == 0) return false;

    const auto dest = reinterpret_cast<unsigned int*>(ref);

    unsigned int* const ctx = t->userCtxsave;
    // Already validated in debugmon excepiton
    unsigned int* const stackPtr = reinterpret_cast<unsigned int*>(ctx[STACK_OFFSET_IN_CTXSAVE]);

    auto offset = 32;
    #if __FPU_PRESENT==1
    bool fpuPresent = false;
    if (!(ctx[9] & (1 << 4))) {
        fpuPresent = true;
        offset = ctxSaveOnStackAligned;
    }
    #endif

    if (regNum <= r3) {
        // r0-r3
        *dest = stackPtr[regNum];
    } else if (regNum <= r11) {
        // r4-r11
        *dest = ctx[regNum + 1 - 4];
    } else if (regNum == r12) {
        *dest = stackPtr[4];
    } else if (regNum == sp) {
        // Report proper stack pointer (before it gets moved by context switch,
        // position depends on size of context, which is different when float
        // registers are saved/omitted
        *dest = reinterpret_cast<unsigned int>(stackPtr) + offset;
    } else if (regNum == lr) {
        *dest = stackPtr[5];
    } else if (regNum == pc) {
        *dest = stackPtr[6];
    } else if (regNum == psr) {
        *dest = stackPtr[7];
    }

#if __FPU_PRESENT==1

    if (regNum < d0) return true;

    if (!fpuPresent) return false;

    // fpscr
    if (regNum == fpscr) {
        *dest = stackPtr[24];
    } else {
        const auto off = (regNum - d0) * 2;
        const auto dest64 = reinterpret_cast<unsigned long long*>(ref);
        // d0-d7 (either d0-d7 or s0-s15)
        if (regNum <= d7) {
            const auto src = *reinterpret_cast<unsigned long long*>(stackPtr + 8 + off);
            *dest64 = src;
        } else {
            // d8-d15 (either d8-d15 or s16-s31)
            const auto src = *reinterpret_cast<unsigned long long*>(ctx + 9 + off);
            *dest64 = src;
        }
    }
#endif
    return true;

}

bool RegisterFile::write(Thread* t, int regNum, char* ref) {

    if (t == nullptr) return false;
    if (getSize(regNum) == 0) return false;

    // To make interface generic for bigger registers
    const auto value = *reinterpret_cast<unsigned int*>(ref);

    unsigned int* const ctx = t->userCtxsave;

    // Already validated in debugmon excepiton
    unsigned int* const stackPtr = reinterpret_cast<unsigned int*>(ctx[STACK_OFFSET_IN_CTXSAVE]);

    auto offset = 32;
    #if __FPU_PRESENT==1
    bool fpuPresent = false;
    if (!(ctx[9] & (1 << 4))) {
        fpuPresent = true;
        offset = ctxSaveOnStackAligned;
    }
    #endif

    // NOTE:
    // Stack pointer has already been validated in debugmon handler, however
    // a write of sp might invalidate it, giving write access to arbitrary
    // memory area, a sequence of these commands gives arbitrary memory region
    // write:
    // - 'P[sp]=badadd' -> writes to sp (userCtxSave[sp]=0xbadadd)
    // - 'P[r0]=dead'   -> writes to r0 (*sp = 0xdead)
    //
    // Before attempting to write the stack pointer, make sure its new value and
    // whole context is within process memory, then copy the context to new
    // location

    if (regNum <= r3) {
        stackPtr[regNum] = value;
    } else if (regNum <= r11) {
        ctx[regNum - 4 + 1] = value;
    } else if (regNum == r12) {
        stackPtr[4] = value;
    } else if (regNum == sp) {
        unsigned int newStackPtr = value - offset;

        // Safe cast: t is a user thread
        Process *p = reinterpret_cast<Process*>(t->getProcess());

        if (!(p->mpu.withinForWriting(reinterpret_cast<unsigned int*>(newStackPtr), offset)))
            return false;
        // Copy context to new location
        // Use of memmove since the two region may overlap
        // TODO: could speed up switching to memcpy if they don't
        memmove(reinterpret_cast<unsigned int*>(newStackPtr), stackPtr, offset);
        ctx[STACK_OFFSET_IN_CTXSAVE] = newStackPtr;
    } else if (regNum == lr) {
        stackPtr[5] = value;
    } else if (regNum == pc) {
        stackPtr[6] = value;
    } else if (regNum == psr) {
        stackPtr[7] = value;
    }

#if __FPU_PRESENT == 1

    if (regNum < d0) return true;

    // if floating point registers are not present, skip writing
    // This allows 'G' packets to write all registers
    if (fpuPresent)
    {
        if (regNum == fpscr) {
            stackPtr[24] = value;
        } else {
            auto off = (regNum - d0) * 2;
            auto value64 = *reinterpret_cast<unsigned long long*>(ref);
            // d0-d7 (either d0-d7 or s0-s15)
            if (regNum <= d7) {
                auto dest = reinterpret_cast<unsigned long long*>(stackPtr + 8 + off);
                *dest = value64;
            // d8-d15 (either d8-d15 or s16-s31)
            } else {
                auto dest = reinterpret_cast<unsigned long long*>(ctx + 9 + off);
                *dest = value64;
            }
        }
    }

#endif

    return true;

}

const char targetXMLString[] = 
// Not necessary for a proper response, save on space
//"<?xml version=\"1.0\"?>"
//"<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
"<target>"
    "<architecture>"
        GDB_ARCH
    "</architecture>"
    "<feature name=\"" GDB_FEATURE_PROFILE "\">"
        "<reg name=\"r0\" bitsize=\"32\"/>"
        "<reg name=\"r1\" bitsize=\"32\"/>"
        "<reg name=\"r2\" bitsize=\"32\"/>"
        "<reg name=\"r3\" bitsize=\"32\"/>"
        "<reg name=\"r4\" bitsize=\"32\"/>"
        "<reg name=\"r5\" bitsize=\"32\"/>"
        "<reg name=\"r6\" bitsize=\"32\"/>"
        "<reg name=\"r7\" bitsize=\"32\"/>"
        "<reg name=\"r8\" bitsize=\"32\"/>"
        "<reg name=\"r9\" bitsize=\"32\"/>"
        "<reg name=\"r10\" bitsize=\"32\"/>"
        "<reg name=\"r11\" bitsize=\"32\"/>"
        "<reg name=\"r12\" bitsize=\"32\"/>"
        "<reg name=\"sp\" bitsize=\"32\"/>"
        "<reg name=\"lr\" bitsize=\"32\"/>"
        "<reg name=\"pc\" bitsize=\"32\"/>"
        "<reg name=\"" GDB_PSR "\" bitsize=\"32\"/>"
    "</feature>"
#if __FPU_PRESENT == 1
    "<feature name=\"org.gnu.gdb.arm.vfp\">"
        "<reg name=\"d0\" bitsize=\"64\"/>"
        "<reg name=\"d1\" bitsize=\"64\"/>"
        "<reg name=\"d2\" bitsize=\"64\"/>"
        "<reg name=\"d3\" bitsize=\"64\"/>"
        "<reg name=\"d4\" bitsize=\"64\"/>"
        "<reg name=\"d5\" bitsize=\"64\"/>"
        "<reg name=\"d6\" bitsize=\"64\"/>"
        "<reg name=\"d7\" bitsize=\"64\"/>"
        "<reg name=\"d8\" bitsize=\"64\"/>"
        "<reg name=\"d9\" bitsize=\"64\"/>"
        "<reg name=\"d10\" bitsize=\"64\"/>"
        "<reg name=\"d11\" bitsize=\"64\"/>"
        "<reg name=\"d12\" bitsize=\"64\"/>"
        "<reg name=\"d13\" bitsize=\"64\"/>"
        "<reg name=\"d14\" bitsize=\"64\"/>"
        "<reg name=\"d15\" bitsize=\"64\"/>"
        "<reg name=\"fpscr\" bitsize=\"32\"/>"
    "</feature>"
#endif
"</target>"
;

const unsigned int targetXMLStringLen = sizeof(targetXMLString);

}

#endif
