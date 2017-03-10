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

using namespace std;
using namespace miosix;

static VirtualClock* vt=nullptr;
static long long computedNsProva;
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
    tc=new TimeConversion(48000000);
    
    initDebugPins();
}
        
bool FlooderSyncNode::synchronize()
{
    if(missPackets>maxMissPackets) return true;
    
    //This an uncorrected clock! Good for Rtc, that doesn't correct by itself
    computedFrameStart+=syncPeriod+clockCorrection;
    computedFrameStartTick=tc->ns2tick(computedFrameStart);
    long long wakeupTime=computedFrameStart-(syncNodeWakeupAdvance+receiverWindow); 
    //This is corrected: thank to the last factor of the sum
    long long timeoutTime=computedFrameStart+receiverWindow+packetPreambleTime-2*clockCorrection; 
        
    if(getTime()>=wakeupTime)
    {
        if(debug) puts("FlooderSyncNode::synchronize called too late");
        ++missPackets;
        return false;
    }

    pm.deepSleepUntil(wakeupTime);
    long long now=getTime();
    
    debug1::high();
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
            debug1::low();
            auto result=transceiver.recv(packet,syncPacketSize,timeoutTime,Transceiver::Unit::NS,HardwareTimer::Correct::UNCORR);
            debug1::high();
            if(isSyncPacket(result,packet) && packet[2]==hop)
            {
                measuredFrameStart=result.timestamp;
                rssi=result.rssi;
                break;
            }
        } catch(exception& e) {
            if(debug) puts(e.what());
            break;
        }
        if(getTime()>=timeoutTime)
        {
            timeout=true;
            break;
        }
    }
    debug1::low();
    
    transceiver.idle(); //Save power waiting for rebroadcast time
    if(timeout==false) rebroadcast(measuredFrameStart,packet);
    transceiver.turnOff();
    ledOff();
    pair<int,int> r;
    printf("%lld %lld\n",wakeupTime,now);
    printf("[%lld] ",getTime()/1000000000);
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
    computedNsProva+=syncPeriod;
//    printf("MeasuredFrame start %lld %lld %lld %lld\n",
//            tc->ns2tick(measuredFrameStart),
//            vt->uncorrected2corrected(tc->ns2tick(measuredFrameStart)),
//            tc->ns2tick(computedFrameStart-clockCorrection),
//            tc->ns2tick(computedFrameStart));
    clockCorrection=r.first;
    receiverWindow=r.second;
    //Correct frame start considering hops
    //measuredFrameStart-=hop*packetRebroadcastTime;
    theoreticalFrameStartNs+=syncPeriod;
    theoreticalFrameStartTick=tc->ns2tick(theoreticalFrameStartNs);
    printf("%lld %lld %lld\n",theoreticalFrameStartNs,(measuredFrameStart),computedFrameStart);
    vt->update(tc->ns2tick(measuredFrameStart),computedFrameStartTick,clockCorrection);
    
    
//    long long measuredFrameStartTick=tc->ns2tick(measuredFrameStart);
//    //Try to estimate some timestamp
//    for(long long i=computedFrameStartTick;i<computedFrameStartTick+676190476;i+=80000000){
//        printf("%lld\n",i-vt->uncorrected2corrected(i));
//    }
//    printf("%lld?\n",computedFrameStartTick+syncPeriod-clockCorrection-vt->uncorrected2corrected(computedFrameStartTick+syncPeriod));
//    printf("Fine test\n");
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
            if(isSyncPacket(result,packet) && (fixedHop==false || packet[2]==hop))
            {
                //Even the Theoretic is started at this time, so the absolute time is dependent of the board
                computedNsProva=theoreticalFrameStartNs=computedFrameStart=measuredFrameStart=result.timestamp;
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
    if(!fixedHop) hop=packet[2];
    //printf("First resync:%lld\n",computedFrameStart);
    //Correct frame start considering hops
    //measuredFrameStart-=hop*packetRebroadcastTime;
    if(debug) puts("Done.\n");
}

void FlooderSyncNode::rebroadcast(long long int receivedTimestamp,
                                  unsigned char* packet)
{
    if(packet[2]>=maxHops-1) return;
    packet[2]++;
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

