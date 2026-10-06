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

#include <arch/drivers/eth/stm32_eth.h>
#include <interfaces/cache.h>

#include <cstdio>
#include <span>
#include <utility>

/*
 * Enable the ethernet driver debug prints by uncommenting the following lines
 */

#define DEBUG_TX(x, ...) // iprintf("TX: " x "\n" __VA_OPT__(, ) __VA_ARGS__)
#define DEBUG_CX(x, ...) // iprintf("CX: " x "\n" __VA_OPT__(, ) __VA_ARGS__)
#define DEBUG_RX(x, ...) // iprintf("RX: " x "\n" __VA_OPT__(, ) __VA_ARGS__)

// #define DEBUG_RX_DESC_LIST
// #define DEBUG_TX_DESC_LIST
// #define DEBUG_CX_DESC_LIST

namespace detail {
template <typename T> struct EnterExitLambda {
    EnterExitLambda(T &&f) : func(std::forward<T>(f)) { func(); }
    ~EnterExitLambda() { func(); }
    T func;
};
} // namespace detail

#ifdef DEBUG_RX_DESC_LIST
#define DEBUG_PRINT_RX_DESC(descs, curIndex)                                   \
    do {                                                                       \
        fputs("RX: ", stdout);                                                 \
        printRxDescriptorList(descs, curIndex);                               \
        fputc('\n', stdout);                                                   \
    } while (0)

#define DEBUG_AUTOPRINT_RX_DESC(descs, curIndex)                               \
    auto _rx_desc_debug =                                                      \
        detail::EnterExitLambda([&] { DEBUG_PRINT_RX_DESC(descs, curIndex); })
#else // DEBUG_RX_DESC_LIST
#define DEBUG_PRINT_RX_DESC(descs, curIndex)
#define DEBUG_AUTOPRINT_RX_DESC(descs, curIndex)
#endif // DEBUG_RX_DESC_LIST

#ifdef DEBUG_TX_DESC_LIST
#define DEBUG_PRINT_TX_DESC(descs, insertIndex, cleanupIndex)                  \
    do {                                                                       \
        fputs("TX: ", stdout);                                                 \
        printTxDescriptorList(descs, insertIndex, cleanupIndex);               \
        fputc('\n', stdout);                                                   \
    } while (0)

#define DEBUG_AUTOPRINT_TX_DESC(descs, insertIndex, cleanupIndex)              \
    auto _tx_desc_debug = detail::EnterExitLambda(                             \
        [&] { DEBUG_PRINT_TX_DESC(descs, insertIndex, cleanupIndex); })
#else // DEBUG_TX_DESC_LIST
#define DEBUG_PRINT_TX_DESC(descs, insertIndex, cleanupIndex)
#define DEBUG_AUTOPRINT_TX_DESC(descs, insertIndex, cleanupIndex)
#endif // DEBUG_TX_DESC_LIST

#ifdef DEBUG_CX_DESC_LIST
#define DEBUG_PRINT_CX_DESC(descs, insertIndex, cleanupIndex)                  \
    do {                                                                       \
        fputs("CX: ", stdout);                                                 \
        printTxDescriptorList(descs, insertIndex, cleanupIndex);               \
        fputc('\n', stdout);                                                   \
    } while (0)

#define DEBUG_AUTOPRINT_CX_DESC(descs, insertIndex, cleanupIndex)              \
    auto _tx_desc_debug = detail::EnterExitLambda(                             \
        [&] { DEBUG_PRINT_CX_DESC(descs, insertIndex, cleanupIndex); })
#else // DEBUG_CX_DESC_LIST
#define DEBUG_PRINT_CX_DESC(descs, insertIndex, cleanupIndex)
#define DEBUG_AUTOPRINT_CX_DESC(descs, insertIndex, cleanupIndex)
#endif // DEBUG_CX_DESC_LIST

// Debug macros for the F2/F4/F7 ethernet driver.
#if defined(_CHIP_STM32F2) || defined(_CHIP_STM32F4) || defined(_CHIP_STM32F7)

[[maybe_unused]] static void
printRxDescriptorList(std::span<miosix::stm32_eth::RxDmaDescriptor> descs,
                      size_t index) {
    /*
    Optimize for compact display of 16 descriptors per line
    Format as table with borders like:
    | DMA FS LS LEN | CPU FS .. 256 [ DMA xx LS  36 ] ... |
    */
    int segmentCounter = 0;

    for (size_t i = 0; i < descs.size(); i++) {
        auto &d = descs[i].writebackLayout();
        miosix::markBufferAfterDmaRead(&d, sizeof(d));

        bool dma = d.ownedByDma();
        bool fs = false;
        bool ls = false;
        int len = -1;
        char sep = (i == index) //
                       ? '['
                       : (i == index + 1) //
                             ? ']'
                             : '|';

        if (dma)
            goto print;

        fs = d.first();
        ls = d.last();

        // Start of frame, reset segment counter
        if (fs)
            segmentCounter = 0;
        else
            segmentCounter++;

        // Compute length
        len = ls //
                  ? d.frameLength() - segmentCounter * 256
                  : 256;

    print:
        iprintf("%c %s %s %s %3d ",  //
                sep,                 //
                dma ? "DMA" : "CPU", //
                fs ? "FS" : "..",    //
                ls ? "LS" : "..",    //
                len);
    }
    iprintf("%c", index == (descs.size() - 1) ? ']' : '|');
}

[[maybe_unused]] static void
printTxDescriptorList(std::span<miosix::stm32_eth::TxDmaDescriptor> descs,
                      size_t insertIndex, size_t cleanupIndex) {
    /*
    Optimize for compact display of 16 descriptors per line
    Format as table with borders like:
    | DMA FS LS LEN { ... FS .. 256 [ DMA .. LS  36 ] ... |
    */
    for (size_t i = 0; i < descs.size(); i++) {
        auto &d = descs[i].writebackLayout();
        miosix::markBufferAfterDmaRead(&d, sizeof(d));

        bool dma = d.ownedByDma();
        bool fs = false;
        bool ls = false;
        int len = -1;
        char sep = '|';

        // Handle separators, precedence to insertIndex
        if (i == cleanupIndex)
            sep = '{';
        else if (i == cleanupIndex + 1)
            sep = '}';
        // Then check insertIndex and overwrite
        if (i == insertIndex)
            sep = '[';
        else if (i == insertIndex + 1)
            sep = ']';

        fs = d.first();
        ls = d.last();
        len = d.bufferSize();

        iprintf("%c %s %s %s %3d ",  //
                sep,                 //
                dma ? "DMA" : "...", //
                fs ? "FS" : "..",    //
                ls ? "LS" : "..",    //
                len);
    }
    iprintf("%c", (insertIndex == descs.size() - 1) //
                      ? ']'
                      : (cleanupIndex == descs.size() - 1) //
                            ? '}'
                            : '|');
}

#elif defined(_CHIP_STM32H7)

[[maybe_unused]] static void
printRxDescriptorList(std::span<miosix::stm32_eth::RxDmaDescriptor> descs,
                      size_t index) {
    // Called before receive() consumes any descriptors. DMA-owned entries
    // have the read layout, where FS, LS, error and packet length do not exist.
    // LEN is the whole received frame including FCS, only valid on a good LS.
    // Match the F2/F4/F7 columns: ownership, FS, LS, length.
    // The length column shows --- when unavailable, or ERR on a bad LS.
    for (size_t i = 0; i < descs.size(); i++) {
        auto &desc = descs[i].writebackLayout();
        miosix::markBufferAfterDmaRead(&desc, sizeof(desc));
        const auto d = desc;
        const char sep = i == index ? '[' : i == index + 1 ? ']' : '|';

        const bool dma = d.ownedByDma();
        iprintf("%c %s %s %s ", sep, dma ? "DMA" : "CPU",
                !dma && d.first() ? "FS" : "..",
                !dma && d.last() ? "LS" : "..");
        if (!dma && d.last() && !d.error())
            iprintf("%3u ", static_cast<unsigned>(d.frameLength()));
        else
            fputs(!dma && d.last() && d.error() ? "ERR " : "--- ", stdout);
    }
    iprintf("%c", index == descs.size() - 1 ? ']' : '|');
}

[[maybe_unused]] static void
printTxDescriptorList(std::span<miosix::stm32_eth::TxDmaDescriptor> descs,
                      size_t insertIndex, size_t cleanupIndex) {
    // Both transmit() (before pollTx) and cleanup() can see a mixture of
    // read and writeback layouts. All CPU edits are synced at these call sites.
    // OWN, FS and LS occupy the same bits in both layouts. As on F2/F4/F7,
    // the length column is the buffer size, available only in the read layout.
    // Writeback shows --- instead, or ERR on a bad LS.
    // CPU-owned entries may also be unused or retain an old completion.
    for (size_t i = 0; i < descs.size(); i++) {
        miosix::markBufferAfterDmaRead(&descs[i], sizeof(descs[i]));
        auto snapshot = descs[i];
        const auto &d = snapshot.writebackLayout();
        char sep = '|';

        if (i == cleanupIndex)
            sep = '{';
        else if (i == cleanupIndex + 1)
            sep = '}';
        if (i == insertIndex)
            sep = '[';
        else if (i == insertIndex + 1)
            sep = ']';

        const bool dma = d.ownedByDma();
        iprintf("%c %s %s %s ", sep, dma ? "DMA" : "...",
                d.first() ? "FS" : "..", d.last() ? "LS" : "..");
        if (dma) {
            const auto &read = snapshot.readLayout();
            iprintf("%3u ", static_cast<unsigned>(read.bufferSize()));
        } else {
            fputs(d.last() && d.error() ? "ERR " : "--- ", stdout);
        }
    }
    iprintf("%c", insertIndex == descs.size() - 1
                      ? ']'
                      : cleanupIndex == descs.size() - 1 ? '}' : '|');
}

#else

#warning "No debug macros defined for the current chip"

[[maybe_unused]] static void
printRxDescriptorList(std::span<miosix::stm32_eth::RxDmaDescriptor>, size_t) {}
[[maybe_unused]] static void
printTxDescriptorList(std::span<miosix::stm32_eth::TxDmaDescriptor>, size_t,
                      size_t) {}

#endif
