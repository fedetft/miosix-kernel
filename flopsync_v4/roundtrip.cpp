#include "roundtrip.h"
#include <iostream>

using namespace std;
namespace miosix{
Roundtrip::Roundtrip(unsigned char hop, unsigned int radioFrequency, short txPower, short panId, int delay):
        radioFrequency(radioFrequency),
        hop(hop),
        panId(panId),
        txPower(txPower),
        lastDelay(0),
        transceiver(Transceiver::instance()),
        timer(getTransceiverTimer()),
        debug(true)
{
    tc=new TimeConversion(EFM32_HFXO_FREQ);
    this->delay=tc->ns2tick(delay);
}

void Roundtrip::ask(long long at, long long timeout){
    if(hop!=1) return;
    
    const unsigned char roundtripPacket[]=
    {
        0x46, //frame type 0b110 (reserved), intra pan
        0x08, //no source addressing, short destination addressing
        static_cast<unsigned char>(hop),          //seq no reused as glossy hop count, 0=root node, set the previous hop
        static_cast<unsigned char>(panId>>8),
        static_cast<unsigned char>(panId & 0xff),   //destination pan ID
        0xff, 0xfe                                  //destination addr (broadcast)
    };
    
    //Transceiver configured with non strict timeout
    miosix::TransceiverConfiguration cfg(radioFrequency,txPower,true,false);
    transceiver.configure(cfg);
    transceiver.turnOn();
    //120000 correspond to 0.25ms enough to do the sendAt
    long long sendTime=tc->ns2tick(at)+120000;
    try {
        transceiver.sendAt(roundtripPacket,askPacketSize,sendTime,Transceiver::Unit::TICK);
    } catch(exception& e) {
        if(debug) puts(e.what());
    }
    
    LedBar<125> p;
    RecvResult result;
    try {
        result=transceiver.recv(p.getPacket(),p.getPacketSize(),sendTime+timeout,Transceiver::Unit::TICK);
    } catch(exception& e) {
        if(debug) puts(e.what());
    }
    transceiver.turnOff();
    
    if(result.size==p.getPacketSize() && result.error==RecvResult::ErrorCode::OK && result.timestampValid){
        lastDelay=result.timestamp-sendTime-delay;
        totalDelay=p.decode().first*accuracy+lastDelay;
        if(debug) printf("delay=%d total=%d\n",lastDelay,totalDelay);
    }else{
        if(debug) printf("No roundtrip replay received\n");
    }
}

void Roundtrip::reply(long long timeout){
    //Transceiver configured with non strict timeout
    miosix::TransceiverConfiguration cfg(radioFrequency,txPower,true,false);
    transceiver.configure(cfg);
    transceiver.turnOn();
    
    unsigned char packet[replyPacketSize];
    bool isTimeout=false;
    long long now=0;
    long long timeoutTime=timer.getValue()+timeout;
    RecvResult result;
    for(;;)
    {
        try {
            result=transceiver.recv(packet,replyPacketSize,timeoutTime,Transceiver::Unit::TICK);
            if(isRoundtripPacket(result,packet) && (packet[2]==hop+1))
            {
                break;
            }
        } catch(exception& e) {
            if(debug) puts(e.what());
        }
        now=timer.getValue();
        if(now>=timeoutTime)
        {
            isTimeout=true;
            break;
        }
    }
    
    if(!isTimeout && result.error==RecvResult::OK && result.timestampValid==true){
        LedBar<125> p;
        p.encode(7);
        try {
            transceiver.sendAt(p.getPacket(),p.getPacketSize(),result.timestamp+delay,Transceiver::Unit::TICK);
        } catch(exception& e) {
            if(debug) puts(e.what());
        }
    }
    
    transceiver.turnOff();
    if(isTimeout){
        if(debug) printf("Roundtrip packet not received\n");
    }else{
        if(debug) printf("Roundtrip packet received\n");
    }
}

bool Roundtrip::isRoundtripPacket(RecvResult& result, unsigned char *packet){
    return    result.error==RecvResult::OK && result.timestampValid==true
               && result.size==askPacketSize
               && packet[0]==0x46 && packet[1]==0x08
               && packet[3]==static_cast<unsigned char>(panId>>8)
               && packet[4]==static_cast<unsigned char>(panId & 0xff)
               && packet[5]==0xff && packet[6]==0xfe;
}
}


