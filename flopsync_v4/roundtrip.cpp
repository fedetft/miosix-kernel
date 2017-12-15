#include "roundtrip.h"
#include <iostream>

using namespace std;
namespace miosix{
Roundtrip::Roundtrip(unsigned char hop, unsigned int radioFrequency, short txPower, short panId, long long delay):
        radioFrequency(radioFrequency),
        hop(hop),
        panId(panId),
        txPower(txPower),
        delay(delay),
        lastDelay(0),
        transceiver(Transceiver::instance()),
        timer(getTransceiverTimer()),
        debug(true)
{
    tc=new TimeConversion(EFM32_HFXO_FREQ);
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
    
    //Sending led bar request to the previous hop
    //Transceiver configured with non strict timeout
    greenLed::high();
    static miosix::TransceiverConfiguration cfg(radioFrequency,txPower,true,false);
    transceiver.configure(cfg);
    //120000 correspond to 2.5us enough to do the sendAt
    //modified to 1ms
    long long sendTime = at + 1000000;
    transceiver.turnOn();
    try {
        transceiver.sendAt(roundtripPacket, askPacketSize, sendTime);
    } catch(exception& e) {
        if(debug) printf("%s\n", e.what());
    }
    if (debug) printf("Asked Roundtrip\n");
    
    //Expecting a ledbar reply from any node of the previous hop, crc disabled
    static miosix::TransceiverConfiguration cfgNoCrc = miosix::TransceiverConfiguration(radioFrequency, txPower, false, false);
    transceiver.configure(cfgNoCrc);
    LedBar<125> p;
    RecvResult result;
    try {
        result = transceiver.recv(p.getPacket(), p.getPacketSize(), sendTime + timeout);
    } catch(exception& e) {
        if(debug) puts(e.what());
    }
    if(debug) {
        if(result.size){
            printf("Received packet, error %d, size %d, timestampValid %d: ", result.error, result.size, result.timestampValid);
            memDump(p.getPacket(), result.size);
        } else printf("No packet received, timeout reached\n");
    }
    transceiver.turnOff();
    greenLed::low();
    
    if(result.size == p.getPacketSize() && result.error == RecvResult::ErrorCode::OK && result.timestampValid) {
        lastDelay = result.timestamp - (sendTime + delay);
        totalDelay = p.decode().first * accuracy + lastDelay;
        if(debug) printf("delay=%lld total=%lld\n", lastDelay, totalDelay);
    } else {
        if(debug) printf("No roundtrip reply received\n");
    }
}

void Roundtrip::reply(long long timeout){
    //Transceiver configured with non strict timeout
    sleep(1000000);
    long long timeoutTime = getTime() + timeout;
    greenLed::high();
    static miosix::TransceiverConfiguration cfg(radioFrequency,txPower,true,false);
    transceiver.configure(cfg);
    transceiver.turnOn();
    
    unsigned char packet[replyPacketSize];
    bool isTimeout = false;
    long long now=0;
    printf("[RTT] Receiving until %lld\n", timeoutTime);
    RecvResult result;
    for(bool success = false; !(success || isTimeout);)
    {
        try {
            result = transceiver.recv(packet, replyPacketSize, timeoutTime);
        } catch(exception& e) {
            if(debug) printf("%s\n", e.what());
        }
        now = getTime();
        if(debug) {
            if(result.size){
                printf("[RTT] Received packet, error %d, size %d, timestampValid %d: ", result.error, result.size, result.timestampValid);
                memDump(packet, result.size);
            } else printf("[RTT] No packet received, timeout reached\n");
        }
        if(isRoundtripPacket(result, packet) && (packet[2] == hop + 1))
        {
            success = true;
        }
        if(now >= timeoutTime)
        {
            isTimeout = true;
        }
    }
    
    if(!isTimeout && result.error == RecvResult::OK && result.timestampValid){
        printf("[RTT] Replying ledbar packet\n");
        LedBar<125> p;
        p.encode(7); //TODO: 7?! should check what's received, increment the led bar and filter it with a LPF
        try {
            transceiver.sendAt(p.getPacket(), p.getPacketSize(), result.timestamp + delay);
        } catch(exception& e) {
            if(debug) puts(e.what());
        }
    }
    
    transceiver.turnOff();
    greenLed::low();
    if(isTimeout){
        if(debug) printf("[RTT] Roundtrip packet not received\n");
    }else{
        if(debug) printf("[RTT] Roundtrip packet received\n");
    }
}
}


