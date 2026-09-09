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

# pragma once

#ifdef PROCESS_DEBUGGER

static inline void IRQsetLocalBreakpoints()
{
    static_assert(std::is_trivially_copyable_v<Breakpoint>
            // Matching size of expected breakpoints comparator on this architecture
            && sizeof(Breakpoint) == sizeof(unsigned int)
            ,"'Breakpoint' objects are not trivially copyable, consider switching implementation or copying them iteratively");
    // Unvolatilies
    memcpy((void*)FPB->FP_COMP, breakpoints, breakpointsNum * sizeof(Breakpoint));

    // NOTE: (SLOW) use this if implementation cannot be changed
    // for (int i = 0; i < breakpointsNum; i++) breakpoints[i].IRQsetLocal(i);
}

static inline void IRQsetLocalWatchpoints()
{
    static_assert(std::is_trivially_copyable_v<Watchpoint>
            // Matching size of expected watchpoint comparator on this architecture
            && sizeof(Watchpoint) == sizeof(unsigned int) * 4
            ,"'Watchpoint' objects are not trivially copyable, consider switching implementation or copying them iteratively");
    // Unvolatilies
    memcpy((void*)_DWT->WP, watchpoints, watchpointsNum * sizeof(Watchpoint));

    // NOTE: Not checking order of componets
    // NOTE: (SLOW) use this if implementation cannot be changed
    // for (int i = 0; i < watchpointsNum; i++) watchpoints[i].IRQsetLocal(i);
}

#endif//PROCESS_DEBUGGER
