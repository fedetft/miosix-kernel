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

#include "miosix_settings.h"

#include "network.h"

#include <interfaces/bsp.h>
#include <kernel/logging.h>
#include <kernel/sync.h>
#include <network/ethernetif.h>

#include <lwip/dhcp.h>
#include <lwip/init.h>
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/timeouts.h>

#include <atomic>
#include <bit>

namespace miosix::network {
namespace {
std::atomic<bool> networkOnline = false;
FastMutex waitOnlineMutex;
ConditionVariable waitOnlineCv;

/**
 * \internal
 * Convert a netmask to CIDR prefix length
 * \example 255.255.255.0 -> 24
 */
int netmaskToPrefix(const ip4_addr_t *netmask) {
    return std::countl_one(ntohl(netmask->addr));
}

/**
 * \internal
 * Print the configuration of a network interface to the bootlog
 */
void bootlogNetworkConfig(struct netif *netif) {
    bool loopback = netif->name[0] == 'l' && netif->name[1] == 'o';
    // loopback interface will never have a supplied DHCP address
    if (!dhcp_supplied_address(netif) && !loopback) {
        char name[3] = {netif->name[0], netif->name[1], '\0'};
        bootlog(
            "Network interface '%s' up, waiting for IP address via DHCP...\n",
            name);
        return;
    }

    char ip[16];
    ipaddr_ntoa_r(ip_2_ip4(&netif->ip_addr), ip, sizeof(ip));

    char gateway[16];
    ipaddr_ntoa_r(ip_2_ip4(&netif->gw), gateway, sizeof(gateway));

    int cidr = netmaskToPrefix(ip_2_ip4(&netif->netmask));

    char name[3] = {netif->name[0], netif->name[1], '\0'};

    bootlog("Network interface '%s' up: IP %s/%d, Gateway: %s\n", name, ip,
            cidr, gateway);
}

void netifStatusCallback(struct netif *netif) {
    if (dhcp_supplied_address(netif)) {
        networkOnline.store(true, std::memory_order_release);
        waitOnlineCv.broadcast();
    }
}

} // namespace

void bootlogNetworkConfig() {
    struct netif *netif = netif_list;
    while (netif != nullptr) {
        bootlogNetworkConfig(netif);
        netif = netif->next;
    }
}

void waitOnline() {
    if (networkOnline.load(std::memory_order_acquire))
        return;

    Lock l(waitOnlineMutex);
    while (!networkOnline.load(std::memory_order_acquire)) {
        waitOnlineCv.wait(l);
    }
}

void *netStackThread(void *) {
    lwip_init();

    struct netif netif;

#ifdef ETHERNET_ENABLE_DHCP
    ip4_addr_t ipaddr = IPADDR4_INIT(IPADDR_ANY);
    ip4_addr_t netmask = IPADDR4_INIT(IPADDR_ANY);
    ip4_addr_t gateway = IPADDR4_INIT(IPADDR_ANY);
#else
    ip4_addr_t ipaddr = ETHERNET_IP_ADDRESS;
    ip4_addr_t netmask = ETHERNET_NETMASK_ADDRESS;
    ip4_addr_t gateway = ETHERNET_GATEWAY_ADDRESS;
#endif

    auto res =
        netif_add(&netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask),
                  ip_2_ip4(&gateway), nullptr, ethernetif_init, netif_input);
    if (!res) {
        errorLog("netStackThread: netif init failed\n");
        return 0;
    }

    netif_set_status_callback(&netif, netifStatusCallback);
    netif_set_default(&netif);
    netif_set_up(&netif);

#ifdef ETHERNET_ENABLE_DHCP
    err_t err = dhcp_start(&netif);
    if (err != ERR_OK) {
        errorLog("netStackThread: DHCP start failed: %d\n", err);
        return 0;
    }
#else // ETHERNET_ENABLE_DHCP
    networkOnline.store(true, std::memory_order_release);
    waitOnlineCv.broadcast();
#endif // ETHERNET_ENABLE_DHCP

    while (true) {
        uint32_t sleepTime = sys_timeouts_sleeptime(); // ms

        if (sleepTime == 0) {
            // Some timeouts are due, process them
            sys_check_timeouts();
            continue;
        }

        long long wakeupTime = miosix::getTime() + sleepTime * 1'000'000ll;
        auto wakeupReason = Thread::timedWait(wakeupTime);

        if (wakeupReason == TimedWaitResult::NoTimeout) {
            // Woken up by an IRQ, service the interface for pending work
            ethernetif_service(&netif);
        }
    }

    return 0;
}

} // namespace miosix::network
