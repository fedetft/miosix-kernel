/***************************************************************************
 *   Copyright (C) 2001-2004 Swedish Institute of Computer Science         *
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

#include "ethernetif_debug.h"
#include <arch/drivers/stm32_eth.h>
#include <kernel/thread.h>

#include <lwip/etharp.h>
#include <lwip/ethip6.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <lwip/snmp.h>
#include <lwip/stats.h>

#include <memory>

#if ETH_PAD_SIZE
#warning ETH_PAD_SIZE unsupported by zero-copy RX with DMA, RX buffers will be unaligned
#endif

using namespace miosix;
using namespace miosix::stm32_eth;

namespace {
/*
 * Given MAC FIFOs are 2KB, compute the descriptor count to address enough
 * buffers for a 2x safety margin (4KB).
 */
constexpr size_t DescriptorCount =
    (4096 + PBUF_POOL_BUFSIZE - 1) / PBUF_POOL_BUFSIZE;

constexpr auto BitsPerSecond = 100'000'000U; // 100Mbps

[[nodiscard]] constexpr size_t nextIndex(size_t index) {
    return index >= (DescriptorCount - 1) ? 0 : index + 1;
}

class RxInterface {
  public:
    err_t init() {
        // Allocate pbufs and set up RX descriptors
        for (size_t i = 0; i < DescriptorCount; i++) {
            auto p = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
            if (!p) {
                // Allocation failure, free already allocated pbufs
                for (size_t j = 0; j < i; j++)
                    pbuf_free(pbufs[j]);

                return ERR_MEM;
            }

            auto &desc = descriptors[i];

            pbufs[i] = p;
            desc.setBuffer(p->payload, p->len);
            desc.setDmaOwned();
            if (i == (DescriptorCount - 1))
                desc.setEndOfRing();

            desc.syncToDma();
        }

        return ERR_OK;
    }

    /**
     * Receives a packet from the ethernet interface.
     *
     * \param netif the lwip network interface structure for this ethernetif
     * \return a pbuf filled with the received packet (including MAC header)
     *         nullptr on memory error
     */
    struct pbuf *receive(struct netif *netif) {
        DEBUG_PRINT_RX_DESC(descriptors, index);

        size_t i = index;

        // Scan the descriptor list for one with first bit set
        while (true) {
            auto &desc = descriptors[i];
            desc.syncToCpu();

            if (desc.ownedByDma()) [[unlikely]] {
                DEBUG_RX("No frame available yet");
                return nullptr;
            }

            if (desc.first()) [[likely]]
                break;

            DEBUG_RX("First descriptor not found at %d, continue", i);
            i = nextIndex(i);
        }

        index = i;
        DEBUG_RX("Descriptor %d contains FS", index);

        // index now points to a descriptor with the first bit set
        struct pbuf *head = nullptr;
        struct pbuf *tail = nullptr;
        bool last = false;
        u16_t totLen = 0;

        do {
            auto &desc = descriptors[i];
            auto pbuf = pbufs[i];

            desc.syncToCpu();

            if (desc.ownedByDma()) {
                // Found a descriptor still owned by DMA before finding last
                // Either no closed descriptor is available at all, or some
                // closed descriptors are available but no last descriptor
                // was found
                // In both cases no full frame was received yet, don't advance
                // the descriptor index
                DEBUG_RX("Descriptor %d owned by DMA, stop", i);
                if (head) {
                    pbuf_free(head);
                    head = nullptr;
                }
                return nullptr;
            }

            DEBUG_RX("Allocate pbuf");
            // Allocate the buffer to replenish the one filled by DMA
            auto newPbuf = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
            if (!newPbuf) {
                // Allocation failure, free already allocated pbufs
                if (head) {
                    pbuf_free(head);
                    head = nullptr;
                }
                break;
            }

            DEBUG_RX("Replenish DMA descriptor buffer");
            // Replenish the DMA descriptor with a new buffer
            pbufs[i] = newPbuf;
            desc.setBuffer(newPbuf->payload, newPbuf->len);

            last = desc.last();
            if (last)
                totLen = desc.frameLength() - 4; // Subtract FCS

            DEBUG_RX("Return descriptor to DMA");
            // Return the descriptor to DMA
            desc.setDmaOwned();
            desc.syncToDma();

            DEBUG_RX("Chain pbuf");
            // Chain the filled buffer to the pbuf chain
            if (!head) {
                head = pbuf;
                tail = head;
            } else {
                tail->next = pbuf;
                tail = tail->next;
            }

            i = nextIndex(i);
            DEBUG_RX("Advanced to descriptor: %d", i);
        } while (!last);

        index = i;
        DEBUG_RX("Got full frame, total length: %d", totLen);

        if (head) {
            DEBUG_RX("Fixup chain sizes");
            // Fixup chain sizes now that frame length is known
            struct pbuf *p = head;
            while (p->next != nullptr) {
                DEBUG_RX("\tpbuf %p len %d", p, p->len);
                p->tot_len = totLen;
                totLen -= p->len;
                p = p->next;
            }
            // Last pbuf in the chain
            p->tot_len = totLen;
            p->len = totLen;

            LINK_STATS_INC(link.recv);
            MIB2_STATS_NETIF_ADD(netif, ifinoctets, head->tot_len);
            if (((u8_t *)head->payload)[0] & 1)
                MIB2_STATS_NETIF_INC(netif,
                                     ifinnucastpkts); // Broad/multicast
            else
                MIB2_STATS_NETIF_INC(netif,
                                     ifinucastpkts); // Unicast packet
        } else {
            // Skip ahead this frame and release descriptors to DMA
            i = index;
            bool last = false;

            do {
                auto &desc = descriptors[index];
                desc.syncToCpu();

                if (desc.ownedByDma()) {
                    DEBUG_RX("Descriptor %d owned by DMA, stop", index);
                    // No more descriptors to process
                    break;
                }

                last = desc.last();

                desc.setDmaOwned();
                desc.syncToDma();

                i = nextIndex(i);
            } while (!last);

            index = i;
            DEBUG_RX("Dropped descriptors up to %d", index);

            LINK_STATS_INC(link.memerr);
            LINK_STATS_INC(link.drop);
            MIB2_STATS_NETIF_INC(netif, ifindiscards);
        }

        pending = false;
        return head;
    }

    bool isPending() const { return pending; }
    void setPending() { pending = true; }

    RxDmaDescriptor *descriptorList() { return descriptors.data(); }

  private:
    std::array<RxDmaDescriptor, DescriptorCount>
        descriptors alignas(alignof(RxDmaDescriptor));
    std::array<struct pbuf *, DescriptorCount> pbufs;
    size_t index = 0;

    // Whether RX descriptors with complete frames are pending processing
    volatile bool pending = false; // set by IRQ
};
} // namespace

class TxInterface {
  public:
    err_t init() {
        // Set up TX descriptors
        for (auto &desc : descriptors) {
            desc.setCpuOwned();
            desc.syncToDma();
        }
        descriptors.back().setEndOfRing();
        descriptors.back().syncToDma();

        return ERR_OK;
    }

    /**
     * Transmit a packet.
     *
     * \param netif the lwip network interface structure for this ethernetif
     * \param p the MAC packet to send (e.g. IP packet including MAC
     * addresses and type)
     * \return ERR_OK if the packet could be sent
     *         an err_t value if the packet couldn't be sent
     *
     * \note In case the DMA TX queue is full, the packet will either be
     * dropped or the function will block until space is available, depending
     * on ETHERNET_WAIT_ON_TX_FULL setting.
     */
    err_t transmit(struct netif *netif, struct pbuf *pbuf) {
#if ETH_PAD_SIZE
        pbuf_remove_header(pbuf, ETH_PAD_SIZE); // drop the padding word
#endif

        // Increase stats independent of transmission success
        if (((u8_t *)pbuf->payload)[0] & 1)
            MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts); // Broad/multicast
        else
            MIB2_STATS_NETIF_INC(netif, ifoutucastpkts); // Unicast

        size_t i = insertIndex;
        struct pbuf *p = pbuf;

        while (p != nullptr) {
            DEBUG_TX("DMA setup for pbuf %d/%d", p->len, pbuf->tot_len);

            auto &desc = descriptors[i];
            desc.syncToCpu();

            if (desc.ownedByDma()) {
#ifdef ETHERNET_WAIT_ON_TX_FULL
                DEBUG_TX("No free TX descriptor at %d, retry", i);
                continue; // Retry until a descriptor is free
#else
                DEBUG_TX("No free TX descriptor at %d, drop packet", i);
                // Cleanup already assigned descriptors
                size_t j = insertIndex;
                while (j != i) {
                    auto &d = descriptors[j];
                    d.setCpuOwned();
                    d.syncToDma();
                    j = nextIndex(j);
                }

                LINK_STATS_INC(link.drop);
                MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
                return ERR_IF;
#endif
            }

            bool first = p == pbuf;
            bool last = p->next == nullptr;
            bool endOfRing = i == (DescriptorCount - 1);

            desc.assignBuffer(p->payload, p->len, first, last, endOfRing);

            // Give the descriptor to DMA
            desc.setDmaOwned();
            desc.syncToDma();

            pbuf_ref(p); // Increase ref count so pbuf is not freed
            pbufs[i] = p;

            i = nextIndex(i);
            p = p->next;
        }

        insertIndex = i;

        DEBUG_PRINT_TX_DESC(descriptors, insertIndex, cleanupIndex);
        DEBUG_TX("Poll DMA to start transmission");
        STM32Ethernet::pollTx();

        LINK_STATS_INC(link.xmit);
        MIB2_STATS_NETIF_ADD(netif, ifoutoctets, pbuf->tot_len);

#if ETH_PAD_SIZE
        pbuf_add_header(pbuf, ETH_PAD_SIZE); // reclaim the padding word
#endif

        return ERR_OK;
    }

    /**
     * Clean up descriptors of transmitted packets.
     */
    void cleanup() {
        DEBUG_AUTOPRINT_CX_DESC(descriptors, insertIndex, cleanupIndex);
        DEBUG_CX("Cleaning up start at %d", cleanupIndex);

        size_t i = cleanupIndex;
        // It is assumed here that cleanupIndex points to a chain head
        // TODO: make this an invariant of this function
        struct pbuf *head = pbufs[i];
        bool last = false;

        do {
            auto &desc = descriptors[i];
            desc.syncToCpu();

            if (desc.ownedByDma()) {
                DEBUG_CX("Found descriptor in use by DMA at %d, stop", i);
                // No more descriptors to process
                break;
            }

            if (i == insertIndex) {
                DEBUG_CX("Reached insertIndex %d, stop", i);
                // Reached the insertion index, no more descriptors to
                // process
                break;
            }

            i = nextIndex(i);

            last = desc.last();
            if (last) {
                // Found last descriptor, the whole chain was sent
                // Free the pbuf chain
                pbuf_free(head);
                // Advance ahead of the freed chain
                DEBUG_CX("Advancing cleanupIndex %d->%d", cleanupIndex, i);
                cleanupIndex = i;
                // Prepare for next chain
                head = pbufs[i];
                last = false;
            }
        } while (!last);

        DEBUG_CX("Cleanup finished at %d (insertion at %d)", cleanupIndex,
                 insertIndex);
        pending = false;
    }

    bool isPending() const { return pending; }
    void setPending() { pending = true; }

    TxDmaDescriptor *descriptorList() { return descriptors.data(); }

  private:
    std::array<TxDmaDescriptor, DescriptorCount>
        descriptors alignas(alignof(TxDmaDescriptor));
    std::array<struct pbuf *, DescriptorCount> pbufs;

    size_t insertIndex = 0;  // Next descriptor to fill
    size_t cleanupIndex = 0; // Next descriptor to clean up

    // Whether completed TX descriptors are pending cleanup
    volatile bool pending = false; // set by IRQ
};

/**
 * Ethernet interface private data.
 * Includes everything needed to operate the ethernet interface.
 * Set as netif->state by ethernetif_init().
 */
struct EthInterface {
    RxInterface rx;
    TxInterface tx;
    Thread *netStackThread = nullptr;

    /**
     * Initializes the ethernet interface.
     * \param netStackThread the thread running the network stack, to be
     * woken up on IRQs
     * \return ERR_OK if the interface is initialized
     *         an err_t value on error
     */
    err_t init(Thread *netStackThread) {
        this->netStackThread = netStackThread;

        if (auto err = rx.init(); err != ERR_OK)
            return err;
        if (auto err = tx.init(); err != ERR_OK)
            return err;

        STM32Ethernet::init(rx.descriptorList(), tx.descriptorList(),
                            ethernetIrqHandler, this);

        return ERR_OK;
    }

    /**
     * Ethernet IRQ handler.
     * Set pending flags and wakeup the stack thread.
     * \param arg pointer to EthInterface instance
     */
    static void ethernetIrqHandler(void *arg) {
        auto self = reinterpret_cast<EthInterface *>(arg);

        auto status = STM32Ethernet::getIrqStatus();
        bool wakeup = false;

        // TODO: handle case of RX PBU error, need to restart DMA
        if (status.rx()) {
            status.clearRx();
            self->rx.setPending();
            wakeup = true;
        }

        if (status.tx()) {
            status.clearTx();
            self->tx.setPending();
            wakeup = true;
        }

        if (wakeup)
            self->netStackThread->IRQwakeup();
    }
};

/**
 * Trasmit a packet on the network interface. Called by the lwip stack.
 * \param netif the lwip network interface structure for this ethernetif
 * \param p the MAC packet to send
 */
err_t ethernetif_output(struct netif *netif, struct pbuf *p) {
    auto ethif = reinterpret_cast<EthInterface *>(netif->state);
    return ethif->tx.transmit(netif, p);
}

/**
 * Receive one or more packets from the network interface and pass it to the
 * stack (by calling netif->input).
 * \param netif the lwip network interface structure
 */
void ethernetif_input(struct netif *netif) {
    auto ethif = reinterpret_cast<EthInterface *>(netif->state);

    do {
        struct pbuf *p = ethif->rx.receive(netif);
        if (!p)
            break;

        err_t err = netif->input(p, netif);

        if (err != ERR_OK)
            pbuf_free(p);
    } while (true);
}

/**
 * Service the ethernet interface.
 * This is expected to be called in a loop, after IRQ wakeup, to process
 * pending RX packets and TX completions.
 * \param netif the lwip network interface structure
 */
void ethernetif_service(struct netif *netif) {
    auto ethif = reinterpret_cast<EthInterface *>(netif->state);

    if (ethif->rx.isPending())
        ethernetif_input(netif);

    if (ethif->tx.isPending())
        ethif->tx.cleanup();
}

/**
 * Initialize the ethernet interface.
 * This function should be passed as a parameter to netif_add().
 * \param netif the lwip network interface structure for this ethernetif
 * \return ERR_OK if the interface is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif) {
#if LWIP_NETIF_HOSTNAME
    // Initialize interface hostname
    netif->hostname = "miosix";
#endif // LWIP_NETIF_HOSTNAME

    // Initialize the snmp variables and counters inside the struct netif
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, BitsPerSecond);
    netif->name[0] = ETHERNET_IFNAME[0];
    netif->name[1] = ETHERNET_IFNAME[1];

#if LWIP_IPV4
    netif->output = etharp_output;
#endif
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif
    netif->linkoutput = ethernetif_output;

    // Initialize the interface private data
    // Use smart pointer to ensure proper cleanup on failure
    auto ethif = std::make_unique<EthInterface>();
    if (!ethif)
        return ERR_MEM;

    // Initialize the ethernet interface
    auto err = ethif->init(Thread::getCurrentThread());
    if (err != ERR_OK)
        return err;

    // Release ownership to netif
    netif->state = ethif.release();
    // TODO: cleaup on netif removal

    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0x80;
    netif->hwaddr[2] = 0xe1;
    netif->hwaddr[3] = 0x00;
    netif->hwaddr[4] = 0x00;
    netif->hwaddr[5] = 0x00;

    netif->mtu = 1500;

    // TODO: support link detection through PHY
    netif->flags =
        NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

#if LWIP_IPV6
    netif_create_ip6_linklocal_address(netif, 1);

#if LWIP_IPV6_MLD
    /*
     * For hardware/netifs that implement MAC filtering.
     * All-nodes link-local is handled by default, so we must let the
     * hardware know to allow multicast packets in. Should set
     * mld_mac_filter previously.
     */
    if (netif->mld_mac_filter != NULL) {
        ip6_addr_t ip6_allnodes_ll;
        ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
        netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
    }
#endif // LWIP_IPV6_MLD
#endif // LWIP_IPV6

    return ERR_OK;
}
