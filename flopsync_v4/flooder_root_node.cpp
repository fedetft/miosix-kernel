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

#include "flooder_root_node.h"
#include <cstdio>
#include <stdexcept>
#include <miosix.h>
#include "../debugpin.h"

using namespace std;
using namespace miosix;

//
// class FlooderRootNode
//

FlooderRootNode::FlooderRootNode(long long syncPeriod,
                                 unsigned int radioFrequency,
                                 unsigned short panId, short txPower)
    : FloodingScheme(syncPeriod),
      pm(PowerManager::instance()),
      timer(getTransceiverTimer()),
      transceiver(Transceiver::instance()),
      radioFrequency(radioFrequency),
      frameStart(-1),
      panId(panId), txPower(txPower), debug(false)
{
    //Minimum ~550us, 200us of slack added
    rootNodeWakeupAdvance=750000;
}

bool FlooderRootNode::synchronize()
{
    initDebugPins();
    if(frameStart>0)
    {
        frameStart+=syncPeriod;
        long long wakeupTime=frameStart-rootNodeWakeupAdvance;
        if(timer.getValue()>=wakeupTime)
        {
            if(debug) puts("FlooderRootNode::synchronize called too late");
            return false;
        }
        pm.deepSleepUntil(wakeupTime);
    } else frameStart=getTime()+rootNodeWakeupAdvance;
    
    ledOn();
    TransceiverConfiguration cfg(radioFrequency,txPower);
    transceiver.configure(cfg);
    transceiver.turnOn();
    const unsigned char syncPacket[]=
    {
        0x46, //frame type 0b110 (reserved), intra pan
        0x08, //no source addressing, short destination addressing
        0x00, //seq no reused as glossy hop count, 0=root node
        static_cast<unsigned char>(panId>>8),
        static_cast<unsigned char>(panId & 0xff), //destination pan ID
        0xff, 0xff                                //destination addr (broadcast)
    };
    try {
        transceiver.sendAt(syncPacket,sizeof(syncPacket),frameStart);
    } catch(exception& e) {
        if(debug) puts(e.what());
    }    
    transceiver.turnOff();
    ledOff();

    return false; //Root node does not desynchronize
}
