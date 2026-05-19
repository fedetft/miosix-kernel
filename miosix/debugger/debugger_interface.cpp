#include "debug_registers.h"
#include "debugger_interface.h"
#include "debugger.h"
#include "kernel/process.h"
#include "kernel/thread.h"

#ifdef PROCESS_DEBUGGER

namespace miosix {

// NOTE: Kept here but commented, used when handling 'c' and 's' programs to
// step over hardcoded breakpoint instructions
//
// bool isCodeBreakpoint(unsigned int* pc) {
// #if defined (__arm__) || defined (__thumb__)
//     const auto instruction = *reinterpret_cast<unsigned short*>(pc);
//     return (instruction & 0xff00) == 0xbe00;
// #else
//     #error "Debugger: cannot determine structure of code breakpoints for the current architecture"
// #endif
// }

Breakpoint::Breakpoint(unsigned int address, unsigned int kind) {
    // Revision 2: Simply set address and enable
    if (fpbGetRevisionVersion() == FP_Ctrl_REVISION_2) {
        value = address | FP_Comp_BE;
        return;
    }
    // Revision 1: Set replace mode, mask address and enable
    const auto mode = (kind == 2)
                    ? (address & 0x02)
                        ? FP_Comp_Mode_BKPT_AT_10
                        : FP_Comp_Mode_BKPT_AT_00
                    : FP_Comp_Mode_BKPT_AT_X0
                    ;
    value = (address & FP_Comp_COMP_MASK)
          | (FP_Comp_ENABLE_MASK)
          | (mode << FP_Comp_REPLACE_SHIFT)
          ;
}

void Breakpoint::remove() { this->value = 0; }

bool Breakpoint::enabled() { return this->value & FP_Comp_ENABLE_MASK; }

bool Breakpoint::eq(const Breakpoint& other) const { return this->value == other.value; }

Watchpoint::Watchpoint(unsigned int address, unsigned int kind, WatchpointType type) {
    const auto tz = __builtin_ctz(kind);

    // Just checking, GDB should know about this
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

int RegisterFile::getSize(int i) {
    // Invalid register number
    if (i <  0  || i >= entries)    return 0;
#if FPU_REGISTERS == 1
    // Floating point, double precision registers
    if (i >= d0 && i <= d15)        return 8;
#endif
    // Integer registers
                                    return 4;
}

bool RegisterFile::read(Thread* t, int regNum, char *ref) {
    if (t == nullptr) return false;
    if (getSize(regNum) == 0) return false;

    const auto dest = reinterpret_cast<unsigned int*>(ref);

    // TODO: Maybe is there a better way than exposing class
    unsigned int* const ctx = t->userCtxsave;
    // Already validated in debugmon excepiton
    unsigned int* const stackPtr = reinterpret_cast<unsigned int*>(ctx[STACK_OFFSET_IN_CTXSAVE]);

    bool fpuPresent = true;
    static const unsigned int ctxSaveOnStackAligned = CTXSAVE_ON_STACK + 4;
    static_assert(ctxSaveOnStackAligned % CTXSAVE_STACK_ALIGNMENT == 0,
            "CTXSAVE_ON_STACK does not respect stack alignment");
    auto offset = ctxSaveOnStackAligned;
    if (ctx[9] & (1 << 4)) {
        fpuPresent = false;
        offset = 32;
    }
    
    if (regNum <= r3) {
        // r0-r3
        *dest = stackPtr[regNum];
        return true;
    }
    if (regNum <= r11) {
        // r4-r11
        *dest = ctx[regNum + 1 - 4];
        return true;
    }
    if (regNum == excr) {
        // excr
        *dest = ctx[9];
        return true;
    }
    // r12
    if (regNum == r12) {
        *dest = stackPtr[4];
        return true;
    }
    // sp
    if (regNum == sp) {
        // Report proper stack pointer (before it gets moved by context switch,
        // position depends on size of context, which is different when float
        // registers are saved/omitted
        // FIXME: Should I use oldStack instead? (saved by hardware) ?
        *dest = reinterpret_cast<unsigned int>(stackPtr) + offset;
        return true;
    }
    // lr,pc
    if (regNum <= pc) {
        *dest = stackPtr[regNum - 14 + 5];
        return true;
    }
    if (regNum == psr) {
        *dest = stackPtr[7];
        return true;
    }

#if FPU_REGISTERS == 1

    if (!fpuPresent) return false;

    // fpscr
    if (regNum == fpscr) {
        *dest = stackPtr[24];
        return true;
    }

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

    return true;

#endif

    return false;

}

// Check that stack pointer lies inside the process memory
bool RegisterFile::validStackPtr(Thread *t, unsigned int *ptr, unsigned int offset) {
    const Process *proc = static_cast<Process*>(t->getProcess());
    const auto base = reinterpret_cast<unsigned int>(ptr);
    const auto start = reinterpret_cast<unsigned int>(
            proc->image.getProcessBasePointer());
    const auto end = start + reinterpret_cast<unsigned int>(
            proc->image.getProcessImageSize());
    return (base >= start)
        && (base <= end)
        && (base + offset <= end);
}

bool RegisterFile::write(Thread* t, int regNum, char* ref) {

    if (t == nullptr) return false;
    if (getSize(regNum) == 0) return false;

    // To make interface generic for bigger registers
    const auto value = *reinterpret_cast<unsigned int*>(ref);

    unsigned int* const ctx = t->userCtxsave;
    
    // Already validated in debugmon excepiton
    unsigned int* const stackPtr = reinterpret_cast<unsigned int*>(ctx[STACK_OFFSET_IN_CTXSAVE]);

    bool fpuPresent = true;
    static const unsigned int ctxSaveOnStackAligned = CTXSAVE_ON_STACK;
    #if ctxSaveOnStackAligned % 8
        #error "CTXSAVE_ON_STACK does not respect stack alignment");
    #endif
    auto offset = ctxSaveOnStackAligned;
    if (ctx[9] & (1 << 4)) {
        fpuPresent = false;
        offset = 32;
    }

    // The stack pointer has already been validated in debugmon handler, however
    // subsequent write could avoid this check. User is allowed to write
    // anything on the stack pointer (they might crash the process), but since
    // the pointer is dereferenced to write on registers on stack, validate that
    // the current stack pointer is valid, otherwise malicious code could:
    // Write 0xbadadd in sp -> stackPtr
    // Write 0xdead   in r0 -> *badStackPtr
    // So before attempting to dereference the stack pointer, make sure it's a
    // valid process address
    if (!validStackPtr(t, stackPtr, offset)) return false;
    
    if (regNum <= r3) {
        // r0-r3
        stackPtr[regNum] = value;
        return true;
    }
    if (regNum <= r11) {
        // r4-r11
        ctx[regNum - 4 + 1] = value;
        return true;
    }
    if (regNum == excr) {
        // excr
        ctx[9] = value;
        return true;
    }
    // r12
    if (regNum == r12) {
        stackPtr[4] = value;
        return true;
    }
    // sp
    if (regNum == sp) {
        // Write proper stack pointer (before it gets moved by context switch,
        // position depends on size of context, which is different when float
        // registers are saved/omitted
        ctx[STACK_OFFSET_IN_CTXSAVE] = value - offset;
        return true;
    }
    // lr,pc
    if (regNum <= pc) {
        stackPtr[regNum - 14 + 5] = value;
        return true;
    }
    if (regNum == psr) {
        // TODO: write psr
        //stackPtr[7] = value;
        return true;
    }

#if FPU_REGISTERS == 1

    // fpscr
    if (regNum == fpscr) {
        stackPtr[24] = value;
        return true;
    }

    if (!fpuPresent) return false;

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
    return true;

#endif

    return false;

}

const char targetXMLString[] = 
"<?xml version=\"1.0\"?>"
"<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
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
#if FPU_REGISTERS == 1
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
    "<feature name=\"exc.return\">"
        "<reg name=\"excr\" bitsize=\"32\"/>"
    "</feature>"
#endif
"</target>"
;

const unsigned int targetXMLStringLen = sizeof(targetXMLString);

}

#endif
