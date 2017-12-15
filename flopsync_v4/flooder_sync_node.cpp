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
#include <iostream>
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
      theoreticalFrameStartNs(0),
      radioFrequency(radioFrequency), clockCorrection(0),
      receiverWindow(synchronizer->getReceiverWindow()),
      missPackets(maxMissPackets+1), hop(1),
      panId(panId), txPower(txPower), fixedHop(false), debug(true)
{
    roundtrip = new Roundtrip(hop, radioFrequency, txPower, panId, 52083);
    
    vt=&VirtualClock::instance();
    vt->setSyncPeriod(syncPeriod);
    //350us and forced receiverWindow=1 fails, keeping this at minimum
    //This is dependent on the optimization level, i usually use level -O2
    syncNodeWakeupAdvance = 450000;
    //5 byte (4 preamble, 1 SFD) * 32us/byte
    packetPreambleTime = 160000;
    //Time when packet is rebroadcast (time for packet)
    //~400us is the minimum to add, added 200us slack
    //FIXME: with a so small slack if the packet is received at the end of the
    //receiver window there may be no time for rebroadcast
    packetRebroadcastTime=(syncPacketSize + 6) * 32000 + 600000;//1,016 ms
    tc = new TimeConversion(EFM32_HFXO_FREQ);
    
    initDebugPins();
}
        
bool FlooderSyncNode::synchronize()
{
    if (missPackets > maxMissPackets) return true;
    if (debug) puts("Synchronizing...\n");
    
    //computing the times needed for synchronizing the nodes and performing checks
    
    theoreticalFrameStartNs += syncPeriod;
    
    //This an uncorrected clock! Good for Rtc, that doesn't correct by itself
    //This is the estimate of the next packet in our clock
    computedFrameStart += syncPeriod+clockCorrection;
    //This is NOT corrected
    long long wakeupTime = computedFrameStart - (syncNodeWakeupAdvance + receiverWindow); 
    //This is fully corrected
    long long timeoutTime = tc->tick2ns(vt->uncorrected2corrected(tc->ns2tick(computedFrameStart + receiverWindow + packetPreambleTime)));    
    
    //check if we skipped the synchronization time
    if (getTime() >= wakeupTime) {
        if (debug) puts("FlooderSyncNode::synchronize called too late\n");
		#define P(x) cout<<#x"="<<x<<endl
		P(getTime());
		P(wakeupTime);
		P(theoreticalFrameStartNs);
		P(computedFrameStart);
		P(timeoutTime);
		#undef P
        ++missPackets;
        return false;
    }
    
    //Transceiver configured with non strict timeout
    static miosix::TransceiverConfiguration cfg(radioFrequency, txPower, true, false);
    transceiver.configure(cfg);
    unsigned char packet[syncPacketSize];
    short rssi = -128;
    
    //Awaiting a time sync packet
    bool timeout = false;
    
    printf("Will wake up @ %lld\n", wakeupTime);

    ledOn();
    printf("Will await sync packet until %lld (uncorrected)\n", timeoutTime);
    pm.deepSleepUntil(wakeupTime);
    
    transceiver.turnOn();
    
    for (bool success = false; !(success || timeout);) {
        RecvResult result;
        try {    
            result = transceiver.recv(packet, syncPacketSize, timeoutTime, Transceiver::Unit::NS, HardwareTimer::Correct::UNCORR);
        } catch(exception& e) {
            if(debug) printf("%s\n", e.what());
        }
        /*if (debug){
            if(result.size){
                printf("Received packet, error %d, size %d, timestampValid %d: ", result.error, result.size, result.timestampValid);
                memDump(packet, result.size);
            } else printf("No packet received, timeout reached\n");
        }*/
        if (isSyncPacket(result, packet) && packet[2] == hop-1)
        {
            measuredFrameStart = result.timestamp;
            rssi = result.rssi;
            success = true;
        } else {
            if (getTime() >= timeoutTime)
                timeout = true;
        }
    }
    
    transceiver.idle(); //Save power waiting for rebroadcast time
    
    //This conversion is really necessary to get the corrected time in NS, to pass to transceiver
    long long correctedMeasuredFrameStart = tc->tick2ns(vt->uncorrected2corrected(tc->ns2tick(measuredFrameStart)));
    //Rebroadcast the sync packet
    if (!timeout) rebroadcast(correctedMeasuredFrameStart, packet);
    if (debug) {
        if (timeout) printf ("Sync packet not received in interval\n");
        else printf("Sync packet received @ %lld\n", measuredFrameStart);
    }
    transceiver.turnOff();
    ledOff();
    pair<int,int> clockCorrectionReceiverWindow;
    
    //Calculate the roundtrip time wrt the packet sender
    if(!timeout) roundtrip->ask(correctedMeasuredFrameStart, 100000LL);
    
    if (timeout) {
        if (++missPackets > maxMissPackets)
        {
            puts("Lost sync\n");
            return false;
        }
        clockCorrectionReceiverWindow = synchronizer->lostPacket();
        measuredFrameStart = computedFrameStart;
        if (debug) printf("miss u=%d w=%d\n", clockCorrection, receiverWindow);
    } else {
        int e = measuredFrameStart-computedFrameStart;
        clockCorrectionReceiverWindow = synchronizer->computeCorrection(e);
        missPackets = 0;
        if(debug) printf("e=%d u=%d w=%d rssi=%d\n", e, clockCorrection, receiverWindow, rssi);
    }

    clockCorrection = clockCorrectionReceiverWindow.first;
    receiverWindow = clockCorrectionReceiverWindow.second;
    
    vt->update(tc->ns2tick(theoreticalFrameStartNs),tc->ns2tick(computedFrameStart),clockCorrection);
    
    return false;
}

void FlooderSyncNode::resynchronize()
{   
    if(debug) puts("Resynchronize...\n");
    synchronizer->reset();
    ledOn();
    miosix::TransceiverConfiguration cfg(radioFrequency,txPower);
    transceiver.configure(cfg);
    transceiver.turnOn();
    unsigned char packet[syncPacketSize];
    //TODO: attach to strongest signal, not just to the first received packet
    RecvResult result;
    for (bool success = false; !success;) {
        try {
            result = transceiver.recv(packet,syncPacketSize,infiniteTimeout);
        } catch(exception& e) {
            if(debug) printf("%s\n", e.what());
        }
        if(debug){
        if(result.size){
            printf("Received packet, error %d, size %d, timestampValid %d: ", result.error, result.size, result.timestampValid);
            memDump(packet, result.size);
        } else printf("No packet received, timeout reached\n");
        }
        if (isSyncPacket(result, packet) && (!fixedHop || packet[2] == hop-1))
        {
            //Even the Theoretic is started at this time, so the absolute time is dependent of the board
            theoreticalFrameStartNs = computedFrameStart = measuredFrameStart = result.timestamp;
            success = true;
        }
    }
    ledOff();
    transceiver.turnOff();
    clockCorrection = 0;
    receiverWindow = synchronizer->getReceiverWindow();
    missPackets = 0;
    if(!fixedHop) hop = packet[2] + 1;
    roundtrip->setHop(hop);
    
    //Correct frame start considering hops
    //measuredFrameStart-=hop*packetRebroadcastTime;
    if(debug) printf("Resynchronized: hop=%d, frame start=%lld, w=%d, rssi=%d.\n", hop, theoreticalFrameStartNs, receiverWindow, result.rssi);
}

void FlooderSyncNode::rebroadcast(long long int receivedTimestamp,
                                  unsigned char* packet)
{
    if (packet[2] >= maxHops-1) return;
    packet[2] = hop;
    try {
        transceiver.sendAt(packet, syncPacketSize, receivedTimestamp + packetRebroadcastTime);
    } catch(exception& e) {
        if(debug) puts(e.what());
    }
}

