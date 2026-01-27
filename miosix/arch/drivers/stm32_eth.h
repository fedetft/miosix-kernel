/***************************************************************************
 *   Copyright (C) 2025-2026 by Niccolò Betto                              *
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

#include "miosix_settings.h"

namespace miosix::stm32_eth {

struct alignas(uint32_t) RxDmaDescriptor {
    // Returns true if the descriptor is owned by the DMA
    bool ownedByDma() const { return status & (1U << 31); }

    // Marks the descriptor as owned by DMA and clears status
    void setDmaOwned() { status = (1U << 31); }

    bool first() const { return status & (1U << 9); }

    bool last() const { return status & (1U << 8); }

    // Error summary bit is only valid if last is set
    bool error() const { return status & (1U << 15); }

    // Frame length in bytes, including CRC, valid only if last is set
    uint16_t frameLength() const { return (status >> 16) & 0x3FFF; }

    // Sets the buffer pointed to by this descriptor
    void setBuffer(void *buf, uint16_t bufSize) {
        buffer = buf;
        // Only set buffer1 size bits [12:0], keeps end-of-ring bit if set
        size = (size & ~0x1FFF) | (bufSize & 0x1FFF);
    }

    // Marks this descriptor as the last one in the descriptor ring
    void setEndOfRing() { size |= (1U << 15); }

    /**
     * DMA -> CPU memory sync.
     * Synchronizes the descriptor and buffer memory so that the CPU reads
     * the latest data written by DMA.
     * \note Assumes the descriptor is 32-byte aligned
     * \note Assumes the buffer is 32-byte aligned
     */
    void syncToCpu();

    /**
     * CPU -> DMA memory sync.
     * Synchronizes the descriptor and buffer memory so that DMA reads the
     * latest data written by CPU.
     * \note Assumes the descriptor is 32-byte aligned
     */
    void syncToDma();

  private:
    volatile uint32_t status = 0;
    uint32_t size = 0;
    void *buffer = nullptr;
    void *buffer2 = nullptr; // unused
    volatile uint32_t statusExt = 0;
    uint32_t reserved = 0;
    volatile uint32_t timestamp[2] = {};
};
static_assert(sizeof(RxDmaDescriptor) == 32);

struct alignas(uint32_t) TxDmaDescriptor {
    // Returns true if the descriptor is owned by the DMA
    bool ownedByDma() const { return control & (1U << 31); }

    // Marks the descriptor as owned by DMA
    void setDmaOwned() { control |= (1U << 31); }

    // Marks the descriptor as owned by CPU
    void setCpuOwned() { control &= ~(1U << 31); }

    void assignBuffer(void *buf, uint16_t bufSize, bool first, bool last,
                      bool endOfRing) {
        control = 0U |                // Clear status bits
                  (0U << 31) |        // Owned by CPU
                  (1U << 30) |        // Enable IRQ on full frame tx complete
                  (last << 29) |      // Last segment
                  (first << 28) |     // First segment
                  (0U << 27) |        // Enable CRC insertion
                  (0U << 26) |        // Enable pad insertion
                  (0U << 25) |        // TX timestamp disabled
                  (0b11 << 22) |      // Full checksum insertion
                  (endOfRing << 21) | // Ring mode
                  (0U << 20);         // Disable chain mode

        setBuffer(buf, bufSize);
    }

    bool first() const { return control & (1U << 28); }

    bool last() const { return control & (1U << 29); }

    uint16_t bufferSize() const { return size & 0x1FFF; }

    // Sets the buffer pointed to by this descriptor
    void setBuffer(void *buf, uint16_t bufSize) {
        buffer = buf;
        size = bufSize & 0x1FFF; // buffer1 size bits [12:0]
    }

    // Marks this descriptor as the last one in the descriptor ring
    void setEndOfRing() { control |= (1U << 21); }

    /**
     * DMA -> CPU memory sync.
     * Synchronizes the descriptor memory so that the CPU reads the
     * latest data written by DMA.
     * \note Assumes the descriptor is 32-byte aligned
     */
    void syncToCpu();

    /**
     * CPU -> DMA memory sync.
     * Synchronizes the descriptor memory so that DMA reads the
     * latest data written by CPU.
     * \note Assumes the descriptor is 32-byte aligned
     */
    void syncToDma();

  private:
    volatile uint32_t control = 0;
    uint32_t size = 0;
    void *buffer = nullptr;
    void *buffer2 = nullptr; // unused
    uint32_t reserved[2] = {};
    volatile uint32_t timestamp[2] = {};
};
static_assert(sizeof(TxDmaDescriptor) == 32);

/**
 * STM32 Ethernet hardware interface.
 * Provides low level access to the STM32 Ethernet MAC and DMA.
 */
namespace STM32Ethernet {

using EthernetIrqHandler = void (*)(void *);

/**
 * Ethernet IRQ status generic interface.
 * Presents a generic interface to read Ethernet IRQ status flags, decoupled
 * from the underlying hardware registers.
 */
class IrqStatus {
  public:
    IrqStatus(volatile uint32_t *r) : reg(r) {}

    /**
     * Returns true if an RX interrupt is pending.
     */
    bool rx();
    /**
     * Clears RX IRQ flags.
     */
    void clearRx();

    /**
     * Returns true if a TX interrupt is pending.
     */
    bool tx();
    /**
     * Clears TX IRQ flags.
     */
    void clearTx();

  private:
    volatile uint32_t *const reg; // Const pointer to maximize optimization
};

/**
 * Initialize the Ethernet hardware (MAC and DMA).
 * \param rxDesc pointer to the RX DMA descriptor list
 * \param txDesc pointer to the TX DMA descriptor list
 * \param hwaddr hardware MAC address
 * \param irqHandler optional IRQ handler to register for Ethernet
 * interrupts
 * \param irqParam optional parameter to pass to the IRQ handler
 */
void init(RxDmaDescriptor *rxDesc, TxDmaDescriptor *txDesc, uint8_t *hwaddr,
          EthernetIrqHandler irqHandler = nullptr, void *irqArg = nullptr);

IrqStatus getIrqStatus();

/**
 * Polls the DMA to resume RX processing.
 *
 * If the DMA RX engine was suspended (e.g. no RX descriptors available),
 * this function notifies the DMA to fetch the next descriptor and resume
 * reception.
 */
void pollRx();

void restartRx();

/**
 * Polls the DMA to resume TX processing.
 *
 * If the DMA TX engine was suspended (e.g. all TX descriptors were
 * processed, no descriptors to send), this function notifies the DMA to
 * fetch the next descriptor and resume transmission.
 */
void pollTx();

void restartTx();

/**
 * Prints the status register ETH->DMASR to the default console for debugging
 * purposes. Can only be called from an IRQ context.
 */
[[maybe_unused]] void IRQprintStatus();

}; // namespace STM32Ethernet

} // namespace miosix::stm32_eth
