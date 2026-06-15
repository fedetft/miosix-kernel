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

void RxDmaDescriptor::syncToCpu() {
#if defined(STM32F7) || defined(STM32H7)
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t *>(buffer),
                                 size & 0x1FFF);
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t *>(this),
                                 sizeof(RxDmaDescriptor));
#endif
}

void RxDmaDescriptor::syncToDma() {
#if defined(STM32F7) || defined(STM32H7)
    SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t *>(this),
                            sizeof(RxDmaDescriptor));
#endif
}

void TxDmaDescriptor::syncToCpu() {
#if defined(STM32F7) || defined(STM32H7)
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t *>(this),
                                 sizeof(RxDmaDescriptor));
#endif
}

void TxDmaDescriptor::syncToDma() {
#if defined(STM32F7) || defined(STM32H7)
    SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t *>(buffer), size);
    SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t *>(this),
                            sizeof(RxDmaDescriptor));
#endif
}

bool STM32Ethernet::IrqStatus::rx() { return *reg & ETH_DMASR_RS; }

void STM32Ethernet::IrqStatus::clearRx() {
    *reg &= (ETH_DMASR_RS | ETH_DMASR_NIS);
}

bool STM32Ethernet::IrqStatus::tx() { return *reg & ETH_DMASR_TS; }

void STM32Ethernet::IrqStatus::clearTx() {
    *reg &= (ETH_DMASR_TS | ETH_DMASR_NIS | ETH_DMASR_TBUS | ETH_DMASR_ETS);
}

void STM32Ethernet::init(RxDmaDescriptor *rxDesc, TxDmaDescriptor *txDesc,
                         uint8_t *hwaddr, EthernetIrqHandler irqHandler,
                         void *irqArg) {
    {
        miosix::FastGlobalIrqLock dLock;
        // Enable ETH clock
        RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACRXEN |
                        RCC_AHB1ENR_ETHMACTXEN |
                        RCC_AHB1ENR_ETHMACPTPEN; // TODO: do we need it?
        RCC_SYNC();
    }

    ETH->MACCR = 0   // Do not strip FCS
                 | 0 // Enable watchdog to cut RX packets to 2048 bytes
                 | 0 // Enable watchdog to cut TX packets to 2048 bytes
                 | 0 // Standard IFG 96 bit
                 | 0 // Enable carrier sense checking
                 | ETH_MACCR_FES  // 100MBit/s mode
                 | ETH_MACCR_DM   // Full duplex
                 | ETH_MACCR_IPCO // Check checksum of IPv4 frames
                 | 0;

    // No source filtering, no promiscuous mode, don't forward flow control
    // frames
    ETH->MACFFR = 0;

    // TODO: how long is a slot time? PT and PLT taken from example in
    // datasheet
    ETH->MACFCR = (256 << 16)       // PT=256 slot times
                  | (1 << 4)        // PLT=1 (28 slot times)
                  | ETH_MACFCR_UPFD // Detect also uncast pause frames
                  | ETH_MACFCR_RFCE // Enable honoring received pause frames
                  | 0;              // Disable sending pause frames

    ETH->MACIMR = ETH_MACIMR_TSTIM    // Disable timestamp interrupts
                  | ETH_MACIMR_PMTIM; // Disable power management interrupts

    // Set MAC address
    ETH->MACA0HR = 1U << 31         // Address enable bit
                   | hwaddr[5] << 8 //
                   | hwaddr[4];
    ETH->MACA0LR = hwaddr[3] << 24   //
                   | hwaddr[2] << 16 //
                   | hwaddr[1] << 8  //
                   | hwaddr[0];

    // Disable counter interrupts
    ETH->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFAEM | ETH_MMCRIMR_RFCEM;
    ETH->MMCTIMR = ETH_MMCTIMR_TGFM | ETH_MMCTIMR_TGFMSCM | ETH_MMCTIMR_TGFSCM;

    ETH->DMABMR = ETH_DMABMR_PBL_8Beat // PBL=8 (default)
                  | ETH_DMABMR_EDE;    // Enhanced descriptors enabled

    ETH->DMAOMR = ETH_DMAOMR_RSF | ETH_DMAOMR_TSF;

    // Set up DMA descriptor lists
    ETH->DMARDLAR = reinterpret_cast<uint32_t>(rxDesc);
    ETH->DMATDLAR = reinterpret_cast<uint32_t>(txDesc);

    // Setup DMA interrupt
    ETH->DMAIER = ETH_DMAIER_NISE   // Normal interrupt summary
                  | ETH_DMAIER_RIE  // RX interrupt
                  | ETH_DMAIER_TIE; // TX interrupt

    if (irqHandler) {
        miosix::GlobalIrqLock gLock;
        IRQregisterIrq(gLock, ETH_IRQn, irqHandler, irqArg);
    }

    // Finally enable MAC and DMA
    ETH->MACCR |= ETH_MACCR_TE | ETH_MACCR_RE;    // Enable MAC TX and RX
    ETH->DMAOMR |= ETH_DMAOMR_ST | ETH_DMAOMR_SR; // Start DMA TX and RX
}

STM32Ethernet::IrqStatus STM32Ethernet::getIrqStatus() { return &ETH->DMASR; }

void STM32Ethernet::pollRx() {
    ETH->DMARPDR = 0; // Poll RX
}

void STM32Ethernet::restartRx() {
    ETH->DMAOMR |= ETH_DMAOMR_SR; // Start RX
}

void STM32Ethernet::pollTx() {
    ETH->DMATPDR = 0; // Poll TX
}

void STM32Ethernet::restartTx() {
    ETH->DMAOMR |= ETH_DMAOMR_ST; // Start TX
}

void STM32Ethernet::IRQprintStatus() {
    IRQerrorLog("ETH->DMASR: ");

#define LOG_IRQ_BIT(bit)                                                       \
    if (ETH->DMASR & ETH_DMASR_##bit) {                                        \
        IRQerrorLog(#bit " ");                                                 \
    }

    LOG_IRQ_BIT(NIS);
    LOG_IRQ_BIT(AIS);
    LOG_IRQ_BIT(ERS);
    LOG_IRQ_BIT(FBES);
    LOG_IRQ_BIT(ETS);
    LOG_IRQ_BIT(RWTS);
    LOG_IRQ_BIT(RPSS);
    LOG_IRQ_BIT(RBUS);
    LOG_IRQ_BIT(RS);
    LOG_IRQ_BIT(TUS);
    LOG_IRQ_BIT(ROS);
    LOG_IRQ_BIT(TJTS);
    LOG_IRQ_BIT(TBUS);
    LOG_IRQ_BIT(TPSS);
    LOG_IRQ_BIT(TS);

    IRQerrorLog("\r\n");

#undef LOG_IRQ_BIT
}

} // namespace miosix::stm32_eth
