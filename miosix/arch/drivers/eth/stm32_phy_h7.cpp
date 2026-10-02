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
 * Named as range [low, high).
 */
enum ClockRange {
    CR_250_300MHz = 0b0101, // HCLK / 124
    CR_150_250MHz = 0b0100, // HCLK / 102
    CR_100_150MHz = 0b0001, // HCLK / 62
    CR_60_100MHz = 0b0000,  // HCLK / 42
    CR_35_60MHz = 0b0011,   // HCLK / 26
    CR_20_35MHz = 0b0010,   // HCLK / 16
};

/**
 * STM32H7 MACMDIOAR register bitfield mapping.
 */
struct __attribute__((packed)) MacMdioAr {
    bool busy : 1;
    bool clause45Enable : 1;
    uint8_t operation : 2;
    bool skipAddressPacket : 1;
    uint8_t : 3; // reserved
    ClockRange clockRange : 4;
    uint8_t trailingClocks : 3;
    uint8_t : 1; // reserved
    uint16_t registerId : 5;
    uint16_t phyAddress : 5;
    bool backToBack : 1;
    bool preambleSuppression : 1;
    uint8_t : 4; // reserved

    static ClockRange getClockBits() {
        // The MAC CSR clock is HCLK, after the AHB prescaler. SystemCoreClock
        // is the CPU clock on H7; SystemD2Clock is not updated by our startup.
        static constexpr uint8_t ahbPrescalerShift[16] = {
            0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
        const auto prescaler =
            (RCC->D1CFGR & RCC_D1CFGR_HPRE) >> RCC_D1CFGR_HPRE_Pos;
        const unsigned int clock =
            SystemCoreClock >> ahbPrescalerShift[prescaler];

        if (clock >= 250'000'000)
            return CR_250_300MHz;
        else if (clock >= 150'000'000)
            return CR_150_250MHz;
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
     * Construct a MACMDIOAR bitfield from a raw 32 bit value.
     */
    static MacMdioAr from(uint32_t value) {
        MacMdioAr result;
        std::memcpy(&result, &value, sizeof(MacMdioAr));
        return result;
    }

    /**
     * Return a MACMDIOAR bitfield as a 32 bit raw value.
     */
    operator uint32_t() const {
        uint32_t out = 0;
        std::memcpy(&out, this, sizeof(MacMdioAr));
        return out;
    }
};
static_assert(sizeof(MacMdioAr) == 4, "MacMdioAr should be a 32bit register");
} // namespace

namespace mdio {
void write(uint16_t phy, uint16_t reg, uint16_t value) {
    ETH->MACMDIODR = value;
    ETH->MACMDIOAR = MacMdioAr{
        .busy = 1,
        .operation = 0b01, // Clause 22 write
        .clockRange = MacMdioAr::getClockBits(),
        .registerId = static_cast<uint16_t>(reg),
        .phyAddress = phy,
    };

    // Wait for write operation complete
    while (MacMdioAr::from(ETH->MACMDIOAR).busy)
        ;
}

uint16_t read(uint16_t phy, uint16_t reg) {
    ETH->MACMDIOAR = MacMdioAr{
        .busy = 1,
        .operation = 0b11, // Clause 22 read
        .clockRange = MacMdioAr::getClockBits(),
        .registerId = reg,
        .phyAddress = phy,
    };

    // Wait for read operation complete
    while (MacMdioAr::from(ETH->MACMDIOAR).busy)
        ;

    return ETH->MACMDIODR;
}
} // namespace mdio
