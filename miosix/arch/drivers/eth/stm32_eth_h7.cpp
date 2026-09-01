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

#include "stm32_eth.h"

#include <interfaces/arch_registers.h>
#include <interfaces/endianness.h>
#include <interfaces/interrupts.h>
#include <kernel/lock.h>
#include <kernel/logging.h>

namespace miosix::stm32_eth {

bool STM32Ethernet::IrqStatus::rx() { return *reg & ETH_DMACSR_RI; }

void STM32Ethernet::IrqStatus::clearRx() {
    *reg &= (ETH_DMACSR_RI | ETH_DMACSR_ERI | ETH_DMACSR_NIS);
}

bool STM32Ethernet::IrqStatus::tx() { return *reg & ETH_DMACSR_TI; }

void STM32Ethernet::IrqStatus::clearTx() {
    *reg &= (ETH_DMACSR_TI | ETH_DMACSR_TBU | ETH_DMACSR_NIS);
}

void STM32Ethernet::init(std::span<RxDmaDescriptor> rxDesc,
                         std::span<TxDmaDescriptor> txDesc, uint8_t *hwaddr,
                         EthernetIrqHandler irqHandler, void *irqArg) {
    {
        miosix::FastGlobalIrqLock dLock;
        // Enable ETH clock
        RCC->AHB1ENR |=
            RCC_AHB1ENR_ETH1MACEN | RCC_AHB1ENR_ETH1RXEN | RCC_AHB1ENR_ETH1TXEN;

        RCC_SYNC();
    }

    // Reset the peripheral and wait for completion
    ETH->DMAMR = 1;
    while (ETH->DMAMR & 1)
        ;

    // Set MAC address
    ETH->MACA0HR = 1U << 31         // Address enable bit
                   | hwaddr[5] << 8 //
                   | hwaddr[4];
    ETH->MACA0LR = hwaddr[3] << 24   //
                   | hwaddr[2] << 16 //
                   | hwaddr[1] << 8  //
                   | hwaddr[0];

    // No source filtering, no promiscuous mode, don't forward flow control
    // frames
    ETH->MACPFR = 0;
    // No MAC interrupts enabled
    ETH->MACIER = 0;

    // TODO: ARP offload
    ETH->MACCR = 0   // Do not strip FCS
                 | 0 // Enable watchdog to cut RX packets to 2048 bytes
                 | 0 // Enable watchdog to cut TX packets to 2048 bytes
                 | ETH_MACCR_IPG_96BIT | 0 // Enable carrier sense checking
                 | ETH_MACCR_FES           // 100MBit/s mode
                 | ETH_MACCR_DM            // Full duplex
                 | ETH_MACCR_IPC           // Check checksum of IPv4 frames
                 | 0;

    // TODO: how long is a slot time? PT and PLT taken from example in datasheet
    // ETH_MACQTXFCR
    ETH->MACTFCR = (256 << ETH_MACTFCR_PT_Pos)  // PT=256 slot times
                   | (1 << ETH_MACTFCR_PLT_Pos) // PLT=1 (28 slot times)
                   | 0;
    // ETH_MACRXFCR
    ETH->MACRFCR = ETH_MACRFCR_UP    // Detect also uncast pause frames
                   | ETH_MACRFCR_RFE // Enable honoring received pause frames
                   | 0;

    // Disable counter interrupts
    ETH->MMCRIMR = 0;
    ETH->MMCTIMR = 0;

    // Enable DMA interrupts generation
    ETH->DMAMR = ETH_DMAMR_INTM_0;

    // Set descriptor lists

    // Per RM: ring length must be descriptor count minus one
    // ETH_DMACRXRLR: Tx desc ring length
    ETH->DMACTDRLR = txDesc.size() - 1;
    // ETH_DMACTXRLR: Rx desc ring length
    ETH->DMACRDRLR = rxDesc.size() - 1;
    // ETH_DMACTXDLAR: Tx desc list address
    ETH->DMACTDLAR = reinterpret_cast<uint32_t>(txDesc.data());
    // ETH_DMACRXDLAR: Rx desc list address
    ETH->DMACRDLAR = reinterpret_cast<uint32_t>(rxDesc.data());
    // ETH_DMACTXDTPR: Tx desc tail pointer
    ETH->DMACTDTPR = reinterpret_cast<uint32_t>(&txDesc.back());
    // ETH_DMACRXDTPR: Rx desc tail pointer
    ETH->DMACRDTPR = reinterpret_cast<uint32_t>(&rxDesc.back());

    // ETH_DMACTXCR: Tx control register
    ETH->DMACTCR = ETH_DMACTCR_TPBL_8PBL // PBL=8
                   | 0;
    // ETH_DMACRXCR: Rx control register
    ETH->DMACRCR = ETH_DMACRCR_RPBL_8PBL // PBL=8
                   | 0;

    // Setup DMA interrupt
    ETH->DMACIER = ETH_DMACIER_NIE    // Normal interrupt summary
                   | ETH_DMACIER_RIE  // RX interrupt
                   | ETH_DMACIER_TIE; // TX interrupt

    if (irqHandler) {
        miosix::GlobalIrqLock gLock;
        IRQregisterIrq(gLock, ETH_IRQn, irqHandler, irqArg);
    }

    // Finally enable DMA and MAC
    ETH->DMACTCR |= ETH_DMACTCR_ST;            // Start TX DMA
    ETH->DMACRCR |= ETH_DMACRCR_SR;            // Start RX DMA
    ETH->MACCR |= ETH_MACCR_TE | ETH_MACCR_RE; // Enable MAC TX and RX
}

STM32Ethernet::IrqStatus STM32Ethernet::getIrqStatus() { return &ETH->DMACSR; }

void STM32Ethernet::pollRx(void *rxDescTail) {
    // Writing to the rx descriptor tail pointer polls the rx DMA engine
    ETH->DMACRDTPR = reinterpret_cast<uint32_t>(rxDescTail);
}

void STM32Ethernet::restartRx() {
    ETH->DMACRCR |= ETH_DMACRCR_SR; // Start RX
}

void STM32Ethernet::pollTx(void *txDescTail) {
    // Writing to the tx descriptor tail pointer polls the tx DMA engine
    ETH->DMACTDTPR = reinterpret_cast<uint32_t>(txDescTail);
}

void STM32Ethernet::restartTx() {
    ETH->DMACTCR |= ETH_DMACTCR_ST; // Start TX
}

void STM32Ethernet::IRQprintStatus() {
    IRQerrorLog("ETH->DMASR: ");

#define LOG_IRQ_BIT(bit)                                                       \
    if (ETH->DMACSR & ETH_DMACSR_##bit) {                                      \
        IRQerrorLog(#bit " ");                                                 \
    }

    LOG_IRQ_BIT(REB);
    LOG_IRQ_BIT(TEB);
    LOG_IRQ_BIT(NIS);
    LOG_IRQ_BIT(AIS);
    LOG_IRQ_BIT(CDE);
    LOG_IRQ_BIT(FBE);
    LOG_IRQ_BIT(ERI);
    LOG_IRQ_BIT(ETI);
    LOG_IRQ_BIT(RWT);
    LOG_IRQ_BIT(RPS);
    LOG_IRQ_BIT(RBU);
    LOG_IRQ_BIT(RI);
    LOG_IRQ_BIT(TBU);
    LOG_IRQ_BIT(TPS);
    LOG_IRQ_BIT(TI);

    IRQerrorLog("\r\n");

#undef LOG_IRQ_BIT
}

} // namespace miosix::stm32_eth
