/***************************************************************************
 *   Copyright (C) 2026 by Niccolò Betto                                   *
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

#include "stm32_uid.h"
#include <interfaces/arch_registers.h>

namespace miosix {

#ifdef STM32_UID_NONCONTIGUOUS
STM32Uid::STM32Uid(const uid_source b) {
    const auto &[w1, w2, w3] = b;
    bytes = {
        w1[0], w1[1], w1[2], w1[3], // w1
        w2[0], w2[1], w2[2], w2[3], // w2
        w3[0], w3[1], w3[2], w3[3], // w3
    };
}
#else
STM32Uid::STM32Uid(const uid_source b) : bytes(b) {}
#endif

STM32Uid GetUniqueId() {
#ifdef STM32_UID_NONCONTIGUOUS
    auto w1 = reinterpret_cast<const uint8_t *>(UID_BASE);
    auto w2 = reinterpret_cast<const uint8_t *>(UID_BASE + 0x04);
    auto w3 = reinterpret_cast<const uint8_t *>(UID_BASE + 0x14);
    return STM32Uid{std::make_tuple(w1, w2, w3)};
#else
    auto uidBytes = reinterpret_cast<const uint8_t *>(UID_BASE);
    return STM32Uid{std::span(uidBytes, 12)};
#endif
}

} // namespace miosix
