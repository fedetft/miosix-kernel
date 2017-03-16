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
     *          between recv and retransmit 
     */
    Roundtrip(unsigned char hop, unsigned int radioFrequency, short txPower, short panId,
            int delay);
    
    /**
     * Function to ask the previous hop nodes to send packet contains 
     * the distance from the master
     * @param timeout time until to wait for the reply, it is a relative time
     */
    void ask(long long at, long long timeout);

    /**
     * Function to listen and wait until an ask packet is received
     * @param timeout time to wait for ask packet, it is a relative time
     */
    void reply(long long timeout);
    
    void setHop(unsigned char hop){ this->hop=hop; }
    
private:
    bool isRoundtripPacket(RecvResult& result, unsigned char *packet);
    TimeConversion* tc;
    unsigned int radioFrequency;
    unsigned char hop;
    unsigned short panId;
    short txPower;
    int delay;
    
    int lastDelay;
    int totalDelay;
    
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

