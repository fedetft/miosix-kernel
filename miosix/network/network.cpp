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

// Contains definitions from ip_addr.h and ip4_addr.h
#include <arpa/inet.h>

#include <interfaces/bsp.h>
#include <kernel/logging.h>
#include <kernel/sync.h>
#include <network/ethernetif.h>
#include <network/phy.h>

#include <lwip/dhcp.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/timeouts.h>

#include <atomic>
#include <bit>
#include <memory>

namespace miosix::network {
namespace {
std::atomic<bool> networkOnline = false;
FastMutex waitOnlineMutex;
ConditionVariable waitOnlineCv;

constexpr uint32_t LINK_STATUS_POLL_PERIOD_MS = 1000;

struct NetifStatusData {
    struct netif *netif;
    uint16_t phy;
    bool linkStatus;
};

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
    char hwaddr[18];
    sniprintf(hwaddr, sizeof(hwaddr), "%02x:%02x:%02x:%02x:%02x:%02x",
              (int)netif->hwaddr[0], (int)netif->hwaddr[1],
              (int)netif->hwaddr[2], (int)netif->hwaddr[3],
              (int)netif->hwaddr[4], (int)netif->hwaddr[5]);

    // Handle cases where DHCP is enabled and IP not yet assigned
    auto dhcp = netif_dhcp_data(netif);
    if (dhcp && !dhcp_supplied_address(netif)) {
        char name[3] = {netif->name[0], netif->name[1], '\0'};
        bootlog("Network interface '%s' (%s) up, waiting for DHCP...\n", name,
                hwaddr);
        return;
    }

    char ip[16];
    ipaddr_ntoa_r(ip_2_ip4(&netif->ip_addr), ip, sizeof(ip));

    char gateway[16];
    ipaddr_ntoa_r(ip_2_ip4(&netif->gw), gateway, sizeof(gateway));

    int cidr = netmaskToPrefix(ip_2_ip4(&netif->netmask));

    char name[3] = {netif->name[0], netif->name[1], '\0'};

    bootlog("Network interface '%s' (%s) up: IP %s/%d, Gateway: %s\n", name,
            hwaddr, ip, cidr, gateway);
}

void netifStatusCallback(struct netif *netif) {
    if (dhcp_supplied_address(netif)) {
        networkOnline.store(true, std::memory_order_release);
        waitOnlineCv.broadcast();
    } else if (netif_is_link_up(netif)) {
        dhcp_network_changed_link_up(netif);
    }
}

void netifStatusTimer(void *arg) {
    auto data = reinterpret_cast<NetifStatusData *>(arg);

    bool status = phy::getLinkStatus(data->phy);
    if (status != data->linkStatus) {
        if (status) {
            netif_set_link_up(data->netif);
        } else {
            netif_set_link_down(data->netif);
        }

        data->linkStatus = status;
    }

    sys_timeout(LINK_STATUS_POLL_PERIOD_MS, netifStatusTimer, arg);
}

} // namespace

void bootlogNetworkConfig() {
    struct netif *netif = netif_list;
    while (netif != nullptr) {
        bootlogNetworkConfig(netif);
        netif = netif->next;
    }
}

void waitConfigAvailable() {
    if (networkOnline.load(std::memory_order_acquire))
        return;

    Lock l(waitOnlineMutex);
    while (!networkOnline.load(std::memory_order_acquire)) {
        waitOnlineCv.wait(l);
    }
}

void *netStackThread(void *) {
    // Initialize the lwIP stack, spawns the tcpip_thread
    tcpip_init(nullptr, nullptr);

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
                  ip_2_ip4(&gateway), nullptr, ethernetif_init, tcpip_input);
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
#else  // ETHERNET_ENABLE_DHCP
    // Inform the DHCP server of our static IP address
    dhcp_inform(&netif);
    networkOnline.store(true, std::memory_order_release);
    waitOnlineCv.broadcast();
#endif // ETHERNET_ENABLE_DHCP

    // Check the initial link status
    bool linkStatus = phy::getLinkStatus(0);
    if (linkStatus)
        netif_set_link_up(&netif);

    // Create the netif status watcher timer
    auto netifStatusData = NetifStatusData{
        .netif = &netif,
        .phy = 0,
        .linkStatus = linkStatus,
    };
    sys_timeout(LINK_STATUS_POLL_PERIOD_MS, netifStatusTimer, &netifStatusData);

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
