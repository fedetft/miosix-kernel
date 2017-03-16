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

#ifndef FLOODER_SYNC_NODE_H
#define	FLOODER_SYNC_NODE_H

#include "stdio.h"
#include "interfaces-impl/transceiver.h"
#include "interfaces-impl/timer_interface.h"
#include "interfaces-impl/power_manager.h"
#include "synchronizer.h"
#include "flooding_scheme.h"
#include <miosix.h>
#include <cstring>
#include "kernel/timeconversion.h"

using namespace miosix;

/**
 * Glossy flooding scheme, generic synchronized node
 */
class FlooderSyncNode : public FloodingScheme
{
public:
    /**
     * Constructor
     * \param synchronizer pointer to synchronizer
     * \param syncPeriod synchronization period
     * \param radioFrequency the radio frequency used for synchronizing
     * \param panId pan ID
     * \param txPower powr at which the sync packet has to be transmitted
     */
    FlooderSyncNode(Synchronizer *synchronizer, long long syncPeriod,
                    unsigned int radioFrequency, unsigned short panId,
                    short txPower);

    /**
     * Needs to be periodically called to send the synchronization packet.
     * This member function sleeps till it's time to send the packet, then
     * sends it and returns. If this function isn't called again within
     * nominalPeriod, the synchronization won't work.
     * \return true if the node desynchronized
     */
    bool synchronize();
    
    /**
     * Called if synchronization is lost
     */
    void resynchronize();
    
    /**
     * \return The (local) time when the synchronization packet was
     * actually received
     */
    long long getMeasuredFrameStart() const { return measuredFrameStart; }
    
    /**
     * \return The (local) time when the synchronization packet was
     * expected to be received
     */
    long long getComputedFrameStart() const
    {
        return computedFrameStart-hop*packetRebroadcastTime;
    }

    /**
     * \return true if in this frame the sync packet was not received
     */
    bool isPacketMissed() const { return missPackets>0; }
    
    /**
     * Force the node to belong to a given hop
     * \param hop hop to which the node should belong, or 0 to restore the
     * default behaviour of automatically joining an hop
     */
    void forceHop(unsigned char hop) { this->hop=hop; fixedHop=hop!=0; }
    
    /**
     * \param enabled if true, this class prints debug data
     */
    void debugMode(bool enabled) { debug=enabled; }
    
private:
    /**
     * Rebroadcst the synchronization packet using glossy
     * \param receivedTimestamp the timestamp when the packet was received
     * \param packet the received packet
     */
    void rebroadcast(long long receivedTimestamp, unsigned char *packet);
        
    bool isSyncPacket(miosix::RecvResult& result, unsigned char *packet);
    
    long long fastNegMul(long long a,unsigned int bi, unsigned int bf){
        if(a<0){
            return -mul64x32d32(-a,bi,bf);
        }else{
            return mul64x32d32(a,bi,bf);
        }
    }
    
    TimeConversion* tc;
    miosix::PowerManager& pm;
    miosix::HardwareTimer& timer;
    miosix::Transceiver& transceiver;
    Synchronizer *synchronizer;
    
    long long measuredFrameStart;
    long long computedFrameStart;
    long long computedFrameStartTick;
    long long theoreticalFrameStartNs;
    long long theoreticalFrameStartTick;
    
    unsigned int radioFrequency;
    int clockCorrection;
    int receiverWindow; 
    int syncNodeWakeupAdvance;
    int packetPreambleTime;
    int packetRebroadcastTime;
    unsigned char missPackets;
    unsigned char hop;  // My hop
                        // Root is 0, the first dynamic hop starts from 1 
    unsigned short panId;
    short txPower;
    bool fixedHop;
    bool debug;
    
    static const unsigned char maxHops=20;
    static const unsigned char maxMissPackets=3;
    static const int syncPacketSize=7;
};

#endif //FLOODER_SYNC_NODE_H
