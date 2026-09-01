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

namespace miosix::stm32_eth {

struct alignas(uint32_t) RxReadDmaDescriptor {
    // Marks the descriptor as owned by DMA
    void setDmaOwned() {
        status |= (1U << 31)    // DMA owned
                  | (1U << 30); // Interrupt on completion
    }

    // Sets the buffer pointed to by this descriptor
    // Size parameter unused but needed for API compatibility
    void setBuffer(void *buf, uint16_t) {
        buffer1 = buf;
        buffer2 = nullptr;
        // Set buffer 1 valid, buffer 2 invalid (unused)
        status = 0b01 << 24;
        // Size set in ETH_DMACRXCR.RBSZ on STM32H7
    }

    // Noop but needed for API compatibility
    void setEndOfRing() {}

    /**
     * CPU -> DMA memory sync.
     * Synchronizes the descriptor and buffer memory so that DMA reads the
     * latest data written by CPU.
     * \note Assumes the descriptor is 32-byte aligned
     */
    void syncToDma();

    void *buffer1;
    uint32_t reserved;
    void *buffer2; // Unused
    uint32_t status;
};
static_assert(sizeof(RxReadDmaDescriptor) == 16);

struct alignas(uint32_t) RxWritebackDmaDescriptor {
    // Returns true if the descriptor is owned by the DMA
    bool ownedByDma() const { return status & (1U << 31); }

    bool first() const { return status & (1U << 29); }

    bool last() const { return status & (1U << 28); }

    // Error summary bit is only valid if last is set
    bool error() const { return status & (1U << 15); }

    // Frame length in bytes, including CRC
    // Valid only if last is set and error is reset
    uint16_t frameLength() const { return status & 0x7FFF; }

    /**
     * DMA -> CPU memory sync.
     * Synchronizes the descriptor memory so that the CPU reads
     * the latest data written by DMA.
     * \note Assumes the descriptor is 32-byte aligned
     * \note Buffer memory needs to be synchronized externally because the
     * writeback format doesn't contain a reference to the buffer anymore!
     */
    void syncToCpu();

  private:
    uint32_t unused[3]; // Unused status bits
    uint32_t status;
};
static_assert(sizeof(RxWritebackDmaDescriptor) == 16);

struct RxDmaDescriptor {
  private:
    union {
        RxReadDmaDescriptor read;
        RxWritebackDmaDescriptor writeback;
    };
};
static_assert(sizeof(RxDmaDescriptor) == 16);

struct alignas(uint32_t) TxReadDmaDescriptor {
    // Marks the descriptor as owned by DMA
    void setDmaOwned() { control2 |= (1U << 31); }

    // Marks the descriptor as owned by CPU
    void setCpuOwned() { control2 &= ~(1U << 31); }

    // End of ring bit is unused but needed for API compatibility
    void assignBuffer(void *buf, uint16_t bufSize, bool first, bool last,
                      bool /* unused */) {
        buffer1 = buf;
        buffer2 = nullptr; // Unused

        control1 = (1U << 31) |        // Enable IRQ on full frame tx complete
                   (0U << 30) |        // TX timestamp disabled
                   (bufSize & 0x3FFF); // buffer1 size bits [13:0]


    bool first() const { return control2 & (1U << 29); }

    bool last() const { return control2 & (1U << 28); }

    uint16_t bufferSize() const { return control1 & 0x3FFF; }

    // Whole packet length, excluding FCS; meaningful on the first descriptor.
    uint16_t frameLength() const { return control2 & 0x7FFF; }

    // Noop but needed for API compatibility
    void setEndOfRing() {}

    /**
     * CPU -> DMA memory sync.
     * Synchronizes the descriptor and buffer memory so that DMA reads the
     * latest data written by CPU.
     * \note Assumes the descriptor is 32-byte aligned
     */
    void syncToDma();

  private:
    void *buffer1;
    void *buffer2;     // Unused
    uint32_t control1; // buffer1 size bits [13:0]
    uint32_t control2;
};
static_assert(sizeof(TxReadDmaDescriptor) == 16);

struct alignas(uint32_t) TxWritebackDmaDescriptor {
    // Returns true if the descriptor is owned by the DMA
    bool ownedByDma() const { return status & (1U << 31); }

    bool first() const { return status & (1U << 29); }

    bool last() const { return status & (1U << 28); }

    // Error summary bit is only valid if last is set
    bool error() const { return status & (1U << 15); }

    /**
     * DMA -> CPU memory sync.
     * Synchronizes the descriptor memory so that the CPU reads the
     * latest data written by DMA.
     * \note Assumes the descriptor is 32-byte aligned
     */
    void syncToCpu();

  private:
    uint32_t timestamp[2];
    uint32_t reserved;
    uint32_t status;
};
static_assert(sizeof(TxWritebackDmaDescriptor) == 16);

struct TxDmaDescriptor {
  private:
    union {
        TxReadDmaDescriptor read;
        TxWritebackDmaDescriptor writeback;
    };
};
static_assert(sizeof(TxDmaDescriptor) == 16);

} // namespace miosix::stm32_eth
