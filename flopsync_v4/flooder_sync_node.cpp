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

#include "flooder_sync_node.h"
#include <cstdio>
#include <cassert>
#include <stdexcept>

using namespace std;
using namespace miosix;

//
// class FlooderSyncNode
//

FlooderSyncNode::FlooderSyncNode(Synchronizer *synchronizer,
                                 long long syncPeriod,
                                 unsigned int radioFrequency,
                                 unsigned short panId, short txPower)
    : FloodingScheme(syncPeriod),
      pm(PowerManager::instance()),
      timer(getTransceiverTimer()),
      transceiver(Transceiver::instance()),
      synchronizer(synchronizer),
      measuredFrameStart(0), computedFrameStart(0),
      radioFrequency(radioFrequency), clockCorrection(0),
      receiverWindow(synchronizer->getReceiverWindow()),
      missPackets(maxMissPackets+1), hop(0),
      panId(panId), txPower(txPower), fixedHop(false), debug(false)
{
    //300us and forced receiverWindow=1 fails, keeping this at minimum
    syncNodeWakeupAdvance=timer.ns2tick(450000);
    //5 byte (4 preamble, 1 SFD) * 32us/byte
    packetPreambleTime=timer.ns2tick(160000);
    //Time when packet is rebroadcast (time for packet)
    //~400us is the minimum to add, added 200us slack
    //FIXME: with a so small slack if the packet is received at the end of the
    //receiver window there may be no time for rebroadcast
    packetRebroadcastTime=timer.ns2tick((syncPacketSize+6)*32000+600000);
}
        
bool FlooderSyncNode::synchronize()
{
    if(missPackets>maxMissPackets) return true;
    
    computedFrameStart+=syncPeriod+clockCorrection;
    long long wakeupTime=computedFrameStart-(syncNodeWakeupAdvance+receiverWindow); 
    long long timeoutTime=computedFrameStart+receiverWindow+packetPreambleTime;
    
    if(timer.getValue()>=wakeupTime)
    {
        if(debug) puts("FlooderSyncNode::synchronize called too late");
        ++missPackets;
        return false;
    }
    pm.deepSleepUntil(wakeupTime);
    
    ledOn();
    //Transceiver configured with non strict timeout
    miosix::TransceiverConfiguration cfg(radioFrequency,txPower,true,false);
    transceiver.configure(cfg);
    transceiver.turnOn();
    unsigned char packet[syncPacketSize];
    short rssi;
    
    bool timeout=false;
    for(;;)
    {
        try {
            auto result=transceiver.recv(packet,syncPacketSize,timeoutTime);
            if(   result.error==RecvResult::OK && result.timestampValid==true
               && result.size==syncPacketSize
               && packet[0]==0x46 && packet[1]==0x08 && packet[2]==hop
               && packet[3]==static_cast<unsigned char>(panId>>8)
               && packet[4]==static_cast<unsigned char>(panId & 0xff)
               && packet[5]==0xff && packet[6]==0xff)
            {
                measuredFrameStart=result.timestamp;
                rssi=result.rssi;
                break;
            }
        } catch(exception& e) {
            if(debug) puts(e.what());
            break;
        }
        if(timer.getValue()>=timeoutTime)
        {
            timeout=true;
            break;
        }
    }
    transceiver.idle(); //Save power waiting for rebroadcast time
    if(timeout==false) rebroadcast(measuredFrameStart,packet);
    transceiver.turnOff();
    ledOff();
    
    pair<int,int> r;
    if(timeout)
    {
        if(++missPackets>maxMissPackets)
        {
            puts("Lost sync");
            return false;
        }
        r=synchronizer->lostPacket();
        measuredFrameStart=computedFrameStart;
        if(debug) printf("miss u=%d w=%d\n",clockCorrection,receiverWindow);
    } else {
        int e=measuredFrameStart-computedFrameStart;
        r=synchronizer->computeCorrection(e);
        missPackets=0;
        if(debug) printf("e=%d u=%d w=%d rssi=%d\n",e,clockCorrection,receiverWindow,rssi);
    }
    clockCorrection=r.first;
    receiverWindow=r.second;
    
    //Correct frame start considering hops
    measuredFrameStart-=hop*packetRebroadcastTime;
    return false;
}

void FlooderSyncNode::resynchronize()
{
    auto isSyncPacket=[this](RecvResult& result, unsigned char *packet)->bool {
        return    result.error==RecvResult::OK && result.timestampValid==true
               && result.size==syncPacketSize
               && packet[0]==0x46 && packet[1]==0x08
               && packet[3]==static_cast<unsigned char>(panId>>8)
               && packet[4]==static_cast<unsigned char>(panId & 0xff)
               && packet[5]==0xff && packet[6]==0xff;
    };
    
    if(debug) puts("Resynchronize...");
    synchronizer->reset();
    ledOn();
    miosix::TransceiverConfiguration cfg(radioFrequency,txPower);
    transceiver.configure(cfg);
    transceiver.turnOn();
    unsigned char packet[syncPacketSize];
    //TODO: attach to strongest signal, not just to the first received packet
    for(;;)
    {
        try {
            auto result=transceiver.recv(packet,syncPacketSize,infiniteTimeout);
            if(isSyncPacket(result,packet))
            {
                computedFrameStart=measuredFrameStart=result.timestamp;
                break;
            }
        } catch(exception& e) {
            if(debug) puts(e.what());
        }
    }
    ledOff();
    clockCorrection=0;
    receiverWindow=synchronizer->getReceiverWindow();
    missPackets=0;
    hop=packet[2];
    //Correct frame start considering hops
    measuredFrameStart-=hop*packetRebroadcastTime;
    if(debug) puts("Done.");
}

void FlooderSyncNode::rebroadcast(long long int receivedTimestamp,
                                  unsigned char* packet)
{
    if(packet[3]>=maxHops-1) return;
    packet[3]++;
    receivedTimestamp+=packetRebroadcastTime;
    try {
        transceiver.sendAt(packet,syncPacketSize,receivedTimestamp);
    } catch(exception& e) {
        if(debug) puts(e.what());
    }
}

