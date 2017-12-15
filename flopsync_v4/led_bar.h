#ifndef BARRALED_H
#define BARRALED_H

#include <utility>
#include <algorithm>
#include <cstring>

#ifdef _MIOSIX
#include "util/util.h"
#else // _MIOSIX

namespace miosix {
void memDump(const void *start, int len);
}
#endif // _MIOSIX

template<unsigned N>
class LedBar
{
public:
    /**
     * Encode a number in range [0; N*2]. Number is rounded in that range.
     * @param num
     */
    void encode(int num);
    
    /**
     * Return the content packet
     * @return  the int represent a number in range [0,N*2], the boolean says 
     *          of the packet is "good" and not too much corrupted
     */
    std::pair<int,bool> decode();
    
    /**
     * return the c-style packet, a simple sequence of char
     * @return 
     */
    unsigned char* getPacket() { return packet; }
    
    /**
     * Print the content of packet
     */
    void print();

    /**
     * return the packet size
     * @return 
     */
    inline int getPacketSize() { return N; }
    
    /**
     * Max number of errors
     */
    static const int threshold = 6;

private:
    unsigned char packet[N];
};

template<unsigned N>
void LedBar<N>::encode(int num)
{
    using namespace std;
    num = max<int>(0,min<int>(N*2,num));
    int bytesCount = num / 2;
    memset(packet, 0xFF, bytesCount);
    memset(packet + bytesCount, 0, N);

    if(num & 1) packet[bytesCount] = 0xf0;
}

template<unsigned N>
std::pair<int,bool> LedBar<N>::decode()
{

    using namespace std;

    const int pktLenNibble = 2*N;

    int fromLeft=pktLenNibble-1;
    bool twoInaRow=false;
    for(int i=0;i<pktLenNibble;i++)
    {
        unsigned char mask = (i & 1) ? 0x0f : 0xf0;

        if((packet[i/2] & mask) != mask)
        {
            if(twoInaRow)
            {
                fromLeft=i-2;
                twoInaRow=false;
                break;
            } else twoInaRow=true;
        } else twoInaRow=false;
    }
    //Last nibble was different, compensate for that
    if(twoInaRow) fromLeft--;

    int fromRight=0;
    twoInaRow=false;

    for(int i=pktLenNibble-1;i>=0;i--)
    {
        unsigned char mask = (i & 1) ? 0x0f : 0xf0;
        if((packet[i/2] & mask) != 0x00)
        {
            if(twoInaRow)
            {
                fromRight=i+2;
                twoInaRow=false;
                break;
            } else twoInaRow=true;
        } else twoInaRow=false;
    }
    //Last nibble was different, compensate for that
    if(twoInaRow) fromRight++;

//     cerr<<fromLeft<<" "<<fromRight<<endl;
//     if(fromLeft>=fromRight) cout<<fromLeft<<" "<<fromRight<<" "<<packet<<endl;
//     assert(fromLeft<fromRight);
    if(fromRight-fromLeft>threshold) return make_pair(0,false);
    else return make_pair((fromLeft+fromRight+1)/2,true);
}

template<unsigned N>
void LedBar<N>::print() { miosix::memDump(packet, N); }

typedef LedBar<16> packet16;
typedef LedBar<64> packet64;
typedef LedBar<127> packet127;

#endif // BARRALED_H
