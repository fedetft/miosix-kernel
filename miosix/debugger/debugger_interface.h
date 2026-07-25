#pragma once

namespace miosix {

enum class DebugStatus {
                    //                          MON_EN      FP_EN       MON_STEP    MON_PEND
    RUN,            // Thread is running:       SET         SET         CLEAR       CLEAR
    STEP,           // Thread is stepping:      SET         CLEAR       SET         CLEAR
    PEND,           // Debugevent is pending:   ---         ---         ---         SET
};

enum class StopReason {
    NONE,                   // No thread running (start)
    DEBUGEVENT,             // Debug event triggered
    EXIT,                   // Exited normally, code is return value
    FAULT,                  // Terminated,      code is the fault reason
    EXECVE,                 // Execve called
};

#if defined(__aarch64__)
    #error "RegisterFile: missing 64 bit layout"
#elif defined (__arm__) || defined (__thumb__)
    #if __ARM_ARCH == 4
        #define GDB_ARCH "armv4"
    #elif __ARM_ARCH == 6
        #define GDB_ARCH "armv6"
    #elif __ARM_ARCH == 7
        #define GDB_ARCH "armv7"
    #endif

    #if defined (__ARM_ARCH_PROFILE) && (__ARM_ARCH_PROFILE == 'M')
        #define GDB_FEATURE_PROFILE "org.gnu.gdb.arm.m-profile"
        #define GDB_PSR "xpsr"
        #define IS_CORTEX_M 1
    #else
        #define GDB_FEATURE_PROFILE "org.gnu.gdb.arm.core"
        #define GDB_PSR "cpsr"
        #define IS_CORTEX_M 0
    #endif

    #if defined (__ARM_FP) && (__ARM_FP & 1)
        #error "RegisterFile: missing half preciosion layout"
    #elif defined(__ARM_FP) && (__ARM_FP > 1)
        #define FPU_REGISTERS 1
        #define GDB_MAX_REG_SIZE 8
    #else
        #define FPU_REGISTERS 0
        #define GDB_MAX_REG_SIZE 4
    #endif

    static const unsigned int      BREAKPOINT_INSTRUCTION_SIZE = 2;
    static const unsigned short    BREAKPOINT_INSTRUCTION_2    = 0xbebe;
    static const unsigned int      BREAKPOINT_INSTRUCTION_4    = 0xbebebebe;

#else
  #error "Debugger: layout not implemented for architectures other than ARM"
#endif

typedef enum : int {
    r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12,
    sp,
    lr,
    pc,
    psr,
#if FPU_REGISTERS == 1
    d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15,
    fpscr,
#endif
    excr,
    REGISTER_FILE_ENTRIES
} RegisterName;

const int REGISTER_FILE_SIZE_BYTES =
    // Base registers
    (17*4) + (1 * 4)
#if FPU_REGISTERS == 1
    // FPU registers (d0-d15 + fpscr)
    + (16*8) + (1*4)
#endif
;

const int MAX_REGISTER_SIZE_BYTES = 
#if FPU_REGISTERS == 1
    8
#else
    4
#endif
;

extern const char targetXMLString[];
extern const unsigned int targetXMLStringLen;

bool isCodeBreakpoint(unsigned int pc);

}
