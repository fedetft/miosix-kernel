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
#include "../../../../debugpin.h"
#include "kernel/timeconversion.h"
#include "interfaces-impl/virtual_clock.h"
#include "roundtrip.h"

using namespace std;
using namespace miosix;

static Roundtrip *roundtrip=nullptr;
static VirtualClock *vt=nullptr;

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
      measuredFrameStart(0), 
      computedFrameStart(0),
      computedFrameStartTick(0),
      theoreticalFrameStartNs(0),theoreticalFrameStartTick(0),
      radioFrequency(radioFrequency), clockCorrection(0),
      receiverWindow(synchronizer->getReceiverWindow()),
      missPackets(maxMissPackets+1), hop(0),
      panId(panId), txPower(txPower), fixedHop(false), debug(true)
{
    roundtrip=new Roundtrip(hop,radioFrequency,txPower,panId,2500000);
    
    vt=&VirtualClock::instance();
    vt->setSyncPeriod(syncPeriod);
    //350us and forced receiverWindow=1 fails, keeping this at minimum
    //This is dependent on the optimization level, i usually use level -O2
    syncNodeWakeupAdvance=450000;
    //5 byte (4 preamble, 1 SFD) * 32us/byte
    packetPreambleTime=160000;
    //Time when packet is rebroadcast (time for packet)
    //~400us is the minimum to add, added 200us slack
    //FIXME: with a so small slack if the packet is received at the end of the
    //receiver window there may be no time for rebroadcast
    packetRebroadcastTime=(syncPacketSize+6)*32000+600000;
    tc=new TimeConversion(EFM32_HFXO_FREQ);
    
    initDebugPins();
}
        
bool FlooderSyncNode::synchronize()
{
    if(missPackets>maxMissPackets) return true;
    
    theoreticalFrameStartNs+=syncPeriod;
    theoreticalFrameStartTick=tc->ns2tick(theoreticalFrameStartNs);
    
    //This an uncorrected clock! Good for Rtc, that doesn't correct by itself
    //This is the estimate of the next packet in our clock
    computedFrameStart+=syncPeriod+clockCorrection;
    computedFrameStartTick=tc->ns2tick(computedFrameStart);
    //This is NOT corrected
    long long wakeupTime=computedFrameStart-(syncNodeWakeupAdvance+receiverWindow); 
    //This is fully corrected
    long long timeoutTime=tc->tick2ns(vt->uncorrected2corrected(tc->ns2tick(computedFrameStart+receiverWindow+packetPreambleTime)));    
    
    if(getTime()>=wakeupTime)
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
    long long now=0;
    for(;;)
    {
        try {    
            auto result=transceiver.recv(packet,syncPacketSize,timeoutTime,Transceiver::Unit::NS,HardwareTimer::Correct::UNCORR);
            if(isSyncPacket(result,packet) && packet[2]==hop-1)
            {
                now=getTime();
                measuredFrameStart=result.timestamp;
                rssi=result.rssi;
                break;
            }
        } catch(exception& e) {
            if(debug) puts(e.what());
            break;
        }
        now=getTime();
        if(now>=timeoutTime)
        {
            timeout=true;
            break;
        }
    }
    
    transceiver.idle(); //Save power waiting for rebroadcast time
    
    //This conversion is really necessary to get the corrected time in NS, to pass to transceiver
    long long a=tc->tick2ns(vt->uncorrected2corrected(tc->ns2tick(measuredFrameStart)));
    if(timeout==false) rebroadcast(a,packet);
    transceiver.turnOff();
    ledOff();
    pair<int,int> r;
    
    if(timeout==false) roundtrip->ask(a, 4800000);
    
    printf("[%lld] ",now/1000000000);
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
    
    //vt->update(tc->ns2tick(measuredFrameStart),computedFrameStartTick,clockCorrection);
    vt->update(theoreticalFrameStartTick,computedFrameStartTick,clockCorrection);
    
    return false;
}

void FlooderSyncNode::resynchronize()
{   
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
            if(isSyncPacket(result,packet) && (!fixedHop || packet[2]==hop-1))
            {
                //Even the Theoretic is started at this time, so the absolute time is dependent of the board
                theoreticalFrameStartNs=computedFrameStart=measuredFrameStart=result.timestamp;
                computedFrameStartTick=theoreticalFrameStartTick=tc->ns2tick(theoreticalFrameStartNs);
                break;
            }
        } catch(exception& e) {
            if(debug) puts(e.what());
        }
    }
    ledOff();
    transceiver.turnOff();
    clockCorrection=0;
    receiverWindow=synchronizer->getReceiverWindow();
    missPackets=0;
    if(!fixedHop) hop=packet[2]+1;
    roundtrip->setHop(hop);
    
    //Correct frame start considering hops
    //measuredFrameStart-=hop*packetRebroadcastTime;
    if(debug) puts("Done.\n");
}

void FlooderSyncNode::rebroadcast(long long int receivedTimestamp,
                                  unsigned char* packet)
{
    if(packet[2]>=maxHops-1) return;
    packet[2]=hop;
    receivedTimestamp+=packetRebroadcastTime;
    try {
        transceiver.sendAt(packet,syncPacketSize,receivedTimestamp);
    } catch(exception& e) {
        if(debug) puts(e.what());
    }
}

bool FlooderSyncNode::isSyncPacket(RecvResult& result, unsigned char *packet){
    return    result.error==RecvResult::OK && result.timestampValid==true
               && result.size==syncPacketSize
               && packet[0]==0x46 && packet[1]==0x08
               && packet[3]==static_cast<unsigned char>(panId>>8)
               && packet[4]==static_cast<unsigned char>(panId & 0xff)
               && packet[5]==0xff && packet[6]==0xff;
}

