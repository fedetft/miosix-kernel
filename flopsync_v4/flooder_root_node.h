/***************************************************************************
 *   Copyright (C)  2013 by Terraneo Federico                              *
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

#ifndef FLOODER_ROOT_NODE_H
#define	FLOODER_ROOT_NODE_H

#include "interfaces-impl/transceiver.h"
#include "interfaces-impl/hardware_timer.h"
#include "interfaces-impl/power_manager.h"
#include "synchronizer.h"
#include "flooding_scheme.h"

/**
 * Glossy flooding scheme, root node
 */
class FlooderRootNode : public FloodingScheme
{
public:
    /**
     * Constructor
     * \param syncPeriod synchronization period
     * \param radioFrequency the radio frequency used for synchronizing
     * \param panId pan ID
     * \param txPower powr at which the sync packet has to be transmitted
     */
    FlooderRootNode(long long syncPeriod, unsigned int radioFrequency,
                    unsigned short panId, short txPower);

    /**
     * Needs to be periodically called to send the synchronization packet.
     * This member function sleeps till it's time to send the packet, then
     * sends it and returns. If this function isn't called again within
     * the synchronization period, the synchronization won't work.
     * \return true if the node desynchronized
     */
    bool synchronize();
    
    /**
     * \return The (local) time when the synchronization packet was
     * actually received
     */
    long long getMeasuredFrameStart() const { return frameStart; }
    
    /**
     * \return The (local) time when the synchronization packet was
     * expected to be received
     */
    long long getComputedFrameStart() const { return frameStart; }
    
    /**
     * \param enabled if true, this class prints debug data
     */
    void debugMode(bool enabled) { debug=enabled; }
    
private:
    miosix::PowerManager& pm;
    miosix::HardwareTimer& timer;
    miosix::Transceiver& transceiver;
    unsigned int radioFrequency;
    long long frameStart;
    int rootNodeWakeupAdvance;
    unsigned short panId;
    short txPower;
    bool debug;
};

#endif //FLOODER_ROOT_NODE_H
