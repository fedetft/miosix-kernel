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

#pragma once

#include <cstdint>
#include <span>

#if defined(STM32L0) || defined(STM32L1)
#define STM32_UID_NONCONTIGUOUS
#endif

#ifdef STM32_UID_NONCONTIGUOUS
#include <array>
#include <tuple>
#endif

namespace miosix {

/**
 * Represents the 96-bit unique ID of STM32 microcontrollers
 *
 * Not all STM32 microcontrollers have a UID that is contiguous in memory, this
 * class abstracts that away by providing a std::span of bytes.
 *
 * For MCUs where the UID is contiguous, the span points directly to the
 * memory-mapped registers holding the UID, avoiding unnecessary copies.
 *
 * For MCUs where the UID is not contiguous, this class copies the UID into an
 * internal array and the span points to that array.
 */
struct STM32Uid {
    std::span<const uint8_t> get() const { return bytes; }

#ifdef STM32_UID_NONCONTIGUOUS
    using storage_type = std::array<uint8_t, 12>;
    using uid_source =
        std::tuple<const uint8_t *, const uint8_t *, const uint8_t *> &&;
#else
    using storage_type = std::span<const uint8_t>;
    using uid_source = std::span<const uint8_t>;
#endif

    friend STM32Uid GetUniqueId();

  private:
    STM32Uid(const uid_source b);

    storage_type bytes;
};

/**
 * \return the unique ID of the STM32 microcontroller
 */
STM32Uid GetUniqueId();

} // namespace miosix
