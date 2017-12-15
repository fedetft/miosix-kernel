#ifndef ROUNDTRIP_H
#define ROUNDTRIP_H

#include "led_bar.h"
#include "interfaces-impl/transceiver.h"
#include "interfaces-impl/timer_interface.h"
#include <miosix.h>

namespace miosix{
class Roundtrip {
public:
    /**
     * 
     * @param hop hop of current node
     * @param radioFrequency 
     * @param txPower 
     * @param panId
     * @param delay this is required by asker and replier, it is the delay 
     *          between recv and retransmit  expressed in nanoseconds
     */
    Roundtrip(unsigned char hop, unsigned int radioFrequency, short txPower, short panId,
            long long delay);
    
    /**
     * Function to ask the previous hop nodes to send packet containing 
     * the distance from the master for measuring the accumulated RTT
     * @param timeout time until to wait for the reply, it is a relative time in nanoseconds
     * @param at when to send the roundtrip request
     */
    void ask(long long at, long long timeout); //TODO this should maybe return some kind of time

    /**
     * Function to listen and wait until a RTT calculation packet is received and reply after the fixed delay
     * @param timeout time to wait for ask packet, it is a relative time
     */
    void reply(long long timeout);
    
    void setHop(unsigned char hop){ this->hop=hop; }
    
private:
    inline bool isRoundtripPacket(RecvResult& result, unsigned char *packet) {
        return result.error == RecvResult::OK && result.timestampValid == true
                && result.size == askPacketSize
                && packet[0] == 0x46 && packet[1] == 0x08
                && packet[3] == static_cast<unsigned char> (panId >> 8)
                && packet[4] == static_cast<unsigned char> (panId & 0xff)
                && packet[5] == 0xff && packet[6] == 0xfe;
    }
    TimeConversion* tc;
    unsigned int radioFrequency;
    unsigned char hop;
    unsigned short panId;
    short txPower;
    long long delay;
    
    long long lastDelay;
    long long totalDelay;
    
    Transceiver& transceiver;
    HardwareTimer& timer;
    bool debug;
    const int askPacketSize = 7;
    const int replyPacketSize = 125;
    
    /**
     * We are very limited with possible value to send through led bar, 
     * so we have to choose accurately the quantization level because it limits
     * the maximum value. We can send values from [0; replyPacketSize*2-1], so
     * with accuracy 15ns --> max value is 15*253=3795ns, or in meter:
     * 3,795e9s * 3e8m/s=1138.5m
     * Remember that each tick@48Mhz are 6.3m at speed of light.
     */
    const int accuracy = 15;
};
}
#endif /* ROUNDTRIP_H */

