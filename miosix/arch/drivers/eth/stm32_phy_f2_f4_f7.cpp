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

#include <cstdint>
#include <cstring>
#include <interfaces/arch_registers.h>
#include <network/phy.h>

namespace {
/**
 * Clock range bit enumeration.
 * Named as range (low, high].
 */
enum ClockRange {
    CR_150_216MHz = 0b100,
    CR_100_150MHz = 0b001,
    CR_60_100MHz = 0b000,
    CR_35_60MHz = 0b011,
    CR_20_35MHz = 0b010,
};

/**
 * STM32 MACMIIAR register bitfield mapping.
 */
struct __attribute__((packed)) MacMiiAr {
    bool busy : 1;
    bool write : 1;
    ClockRange clockRange : 3;
    uint8_t : 1; // reserved
    uint16_t registerId : 5;
    uint16_t phyAddress : 5;
    uint16_t : 16; // reserved

    static ClockRange getClockBits() {
        unsigned int clock = SystemCoreClock;

        if (clock >= 150'000'000)
            return CR_150_216MHz;
        else if (clock >= 100'000'000)
            return CR_100_150MHz;
        else if (clock >= 60'000'000)
            return CR_60_100MHz;
        else if (clock >= 35'000'000)
            return CR_35_60MHz;
        else
            return CR_20_35MHz;
    }

    /**
     * Construct a MACMIIAR bitfield from a raw 32 bit value.
     */
    static MacMiiAr from(uint32_t value) {
        MacMiiAr result;
        std::memcpy(&result, &value, sizeof(MacMiiAr));
        return result;
    }

    /**
     * Return a MACMIIAR bitfield as a 32 bit raw value.
     */
    operator uint32_t() const {
        uint32_t out = 0;
        std::memcpy(&out, this, sizeof(MacMiiAr));
        return out;
    }
};
static_assert(sizeof(MacMiiAr) == 4, "MacMiiAr should be a 32bit register");
} // namespace

namespace mdio {
void write(uint16_t phy, uint16_t reg, uint16_t value) {
    ETH->MACMIIDR = value;
    ETH->MACMIIAR = MacMiiAr{
        .busy = 1,
        .write = 1,
        .clockRange = MacMiiAr::getClockBits(),
        .registerId = static_cast<uint16_t>(reg),
        .phyAddress = phy,
    };

    // Wait for write operation complete
    while (MacMiiAr::from(ETH->MACMIIAR).busy)
        ;
}

uint16_t read(uint16_t phy, uint16_t reg) {
    ETH->MACMIIAR = MacMiiAr{
        .busy = 1,
        .write = 0,
        .clockRange = MacMiiAr::getClockBits(),
        .registerId = static_cast<uint16_t>(reg),
        .phyAddress = phy,
    };

    // Wait for read operation complete
    while (MacMiiAr::from(ETH->MACMIIAR).busy)
        ;

    return ETH->MACMIIDR;
}
} // namespace mdio
