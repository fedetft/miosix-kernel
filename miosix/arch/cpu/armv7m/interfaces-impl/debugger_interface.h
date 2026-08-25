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
