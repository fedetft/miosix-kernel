/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#ifndef ARCH_SETTINGS_H
#define	ARCH_SETTINGS_H

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// \internal Size of vector to store registers during ctx switch
/// ((9+16+1)*4=104Bytes). Only sp, r4-r11 and s16-s31 are saved here, since
/// r0-r3,r12,lr,pc,xPSR, old sp and s0-s15,fpscr are saved by hardware on the
/// process stack on Cortex M4 CPUs. The +1 is to save the exception lr, that
/// is, EXC_RETURN, as it is necessary to know if the thread has used fp regs
const unsigned char CTXSAVE_SIZE=9+16+1;

/// \internal some architectures save part of the context on their stack.
/// This constant is used to increase the stack size by the size of context
/// save frame. If zero, this architecture does not save anything on stack
/// during context save. Size is in bytes, not words.
///  8 registers=r0-r3,r12,lr,pc,xPSR
/// 17 registers=s0-s15,fpscr
/// MUST be divisible by 4.
// FIXME: +1 because of alignment of the cortex m3!!
const unsigned int CTXSAVE_ON_STACK=(8+17+1)*4;

/**
 * \}
 */

} //namespace miosix

#endif	/* ARCH_SETTINGS_H */
