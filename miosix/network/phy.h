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

/**
 * Enumeration of standard PHY registers, from IEEE 802.3 Subsection 22.2.4.
 */
enum class PHYRegister
{
    Control = 0,
    Status = 1,
    Identifier1 = 2,
    Identifier2 = 3,
    AutoNegotiationAdvertisement = 4,
    AutoNegotiationBasePageAbility = 5,
    AutoNegotiationExpansion = 6,
    AutoNegotiationNextPageTransmit = 7,
    AutoNegotiationNextPageReceived = 8,
    MasterSlaveControl = 9,
    MasterSlaveStatus = 10,
    PSEControl = 11,
    PSEStatus = 12,
    MMDAccessControl = 13,
    MMDAccessAddressData = 14,
    ExtendedStatus = 15,
};

namespace phy
{
/**
 * Returns the PHY Link Status from the Status register.
 * See IEEE 802.3 Subsection 22.2.4.2.13 for more information.
 */
bool getLinkStatus(uint16_t phy);
}
