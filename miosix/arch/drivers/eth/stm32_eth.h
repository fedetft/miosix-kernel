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
#include <span>

#include "miosix_settings.h"
#include "stm32_eth_desc.h"

namespace miosix::stm32_eth {

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
 * \note MII/RMII *must* be selected prior to calling this function. If the
 * incorrect configuration is selected, the ETH peripheral will be enabled but
 * won't be able to communicate with the PHY: no RX buffers will be filled, and
 * no TX buffers will be sent out.
 * \param rxDesc pointer to the RX DMA descriptor list
 * \param txDesc pointer to the TX DMA descriptor list
 * \param hwaddr hardware MAC address
 * \param irqHandler optional IRQ handler to register for Ethernet
 * interrupts
 * \param irqParam optional parameter to pass to the IRQ handler
 */
void init(std::span<RxDmaDescriptor> rxDesc, std::span<TxDmaDescriptor> txDesc,
          uint8_t *hwaddr, EthernetIrqHandler irqHandler = nullptr,
          void *irqArg = nullptr);

IrqStatus getIrqStatus();

/**
 * Polls the DMA to resume RX processing.
 *
 * If the DMA RX engine was suspended (e.g. no RX descriptors available),
 * this function notifies the DMA to fetch the next descriptor and resume
 * reception.
 */
void pollRx(void* rxDescTail);

void restartRx();

/**
 * Polls the DMA to resume TX processing.
 *
 * If the DMA TX engine was suspended (e.g. all TX descriptors were
 * processed, no descriptors to send), this function notifies the DMA to
 * fetch the next descriptor and resume transmission.
 */
void pollTx(void* txDescTail);

void restartTx();

/**
 * Prints the status register ETH->DMASR to the default console for debugging
 * purposes. Can only be called from an IRQ context.
 */
[[maybe_unused]] void IRQprintStatus();

} // namespace STM32Ethernet

} // namespace miosix::stm32_eth
