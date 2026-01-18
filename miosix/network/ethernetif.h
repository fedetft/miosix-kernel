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

#include <lwip/err.h>
#include <lwip/netif.h>

/**
 * Initialize the ethernet interface.
 * This function should be passed as a parameter to netif_add().
 * \param netif the lwip network interface structure for this ethernetif
 * \return ERR_OK if the interface is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif);

/**
 * Service the ethernet interface.
 * This is expected to be called in a loop, after IRQ wakeup, to process
 * pending RX packets and TX completions.
 * \param netif the lwip network interface structure
 */
void ethernetif_service(struct netif *netif);

/**
 * Receive one or more packets from the network interface and pass it to the
 * stack (by calling netif->input).
 * \param netif the lwip network interface structure
 */
void ethernetif_input(struct netif *netif);

/**
 * Trasmit a packet on the network interface. Called by the lwip stack.
 * \param netif the lwip network interface structure for this ethernetif
 * \param p the MAC packet to send
 */
err_t ethernetif_output(struct netif *netif, struct pbuf *p);
