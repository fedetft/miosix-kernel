
#ifndef SOFTWARE_SPI_H
#define	SOFTWARE_SPI_H

#include "interfaces/gpio.h"

namespace miosix {

/**
 * Software implementation of the SPI protocol (CPOL=0, CPHA=0 mode)
 * \param SI an instance of the Gpio class indicating the SPI input pin
 * \param SO an instance of the Gpio class indicating the SPI output pin
 * \param SCK an instance of the Gpio class indicating the SPI clock pin
 * \param CE an instance of the Gpio class indicating the SPI chip enable pin
 * \param numNops number of nops to add to the send loop to slow down SPI clock
 */
template<typename SI, typename SO, typename SCK, typename CE, unsigned numNops>
class SoftwareSPI
{
public:
    /**
     * Initialize the SPI interface
     */
    static void init()
    {
        SI::mode(Mode::INPUT);
        SO::mode(Mode::OUTPUT);
        SCK::mode(Mode::OUTPUT);
        CE::mode(Mode::OUTPUT);
        CE::high();
    }

    /**
     * Send a byte and, since SPI is full duplex, simultaneously receive a byte
     * \param data to send
     * \return data received
     */
    static unsigned char sendRecvChar(unsigned char data);

    /**
     * Send an unsigned short and, since SPI is full duplex, simultaneously
     * receive an unsigned short
     * \param data to send
     * \return data received
     */
    static unsigned short sendRecvShort(unsigned short data);

    /**
     * Send an int and, since SPI is full duplex, simultaneously receive an int
     * \param data to send
     * \return data received
     */
    //static unsigned int sendRecvLong(unsigned int data);

    /**
     * Pull CE low, indicating transmission start.
     */
    static void ceLow() { CE::low(); }

    /**
     * Pull CE high, indicating transmission end.
     */
    static void ceHigh() { CE::high(); }
};

template<typename SI, typename SO, typename SCK, typename CE, unsigned numNops>
unsigned char SoftwareSPI<SI,SO,SCK,CE,numNops>::
        sendRecvChar(unsigned char data)
{
    unsigned char result;
    for(int i=0;i<8;i++)
    {
        if(data & 0x80) SO::high();
        SCK::high();
        data<<=1;
        if(SI::value()) result |= 0x1;
        result<<=1;
        for(int j=0;j<numNops;j++) asm volatile("nop");
        SCK::low();
        for(int j=0;j<numNops;j++) asm volatile("nop");
        SO::low();
    }
    return result;
}

template<typename SI, typename SO, typename SCK, typename CE, unsigned numNops>
unsigned short SoftwareSPI<SI,SO,SCK,CE,numNops>::
        sendRecvShort(unsigned short data)
{
    unsigned short result;
    for(int i=0;i<16;i++)
    {
        if(data & 0x8000) SO::high();
        SCK::high();
        data<<=1;
		result<<=1;
        if(SI::value()) result |= 0x1;
        for(int j=0;j<numNops;j++) asm volatile("nop");
        SCK::low();
        for(int j=0;j<numNops;j++) asm volatile("nop");
        SO::low();
    }
    return result;
}

} //namespace miosix

#endif  //SOFTWARE_SPI_H
