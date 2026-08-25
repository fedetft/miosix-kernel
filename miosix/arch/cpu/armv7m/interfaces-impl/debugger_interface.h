#pragma once

#include "cpu_const_impl.h"

namespace miosix {

#define GDB_ARCH "armv7"
#define GDB_FEATURE_PROFILE "org.gnu.gdb.arm.m-profile"
#define GDB_PSR "xpsr"

typedef enum : int {
    r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12,
    sp,
    lr,
    pc,
    psr,
#if __FPU_PRESENT == 1
    d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15,
    fpscr,
#endif
    REGISTER_FILE_ENTRIES
} RegisterName;


#if __FPU_PRESENT == 1
    // includes float and fpscr
    const int REGISTER_FILE_SIZE_BYTES = (17*4) + (16*8) + (1*4);
#else
    const int REGISTER_FILE_SIZE_BYTES = (17*4);
#endif

// Used to ensure the communication buffer is big enough to fit a 'qSupported' package
const int MINIMUM_GDB_BUFFER_SIZE = 208;

#if __FPU_PRESENT == 1
const int MAX_REGISTER_SIZE_BYTES = 8;
#else
const int MAX_REGISTER_SIZE_BYTES = 4;
#endif
;

extern const char targetXMLString[];
extern const unsigned int targetXMLStringLen;

}
