
#ifndef SOFTWARE_I2C_H
#define	SOFTWARE_I2C_H

#include "interfaces/gpio.h"
#include "interfaces/delays.h"

namespace miosix {

/**
 * Software I2C class.
 * \param SDA SDA gpio pin. Pass a Gpio<P,N> class
 * \param SCL SCL gpio pin. Pass a Gpio<P,N> class
 */
template <typename SDA, typename SCL>
class SoftwareI2C
{
public:
    /**
     * Initializes the SPI software peripheral
     */
    static void init();

    /**
     * Send a start condition
     */
    static void sendStart();

    /**
     * Send a stop condition
     */
    static void sendStop();

    /**
     * Send a byte to a device.
     * \param data byte to send
     * \return true if the device acknowledged the byte
     */
    static bool send(unsigned char data);

    /**
     * Receive a byte from a device. Always acknowledges back.
     * \return the received byte
     */
    static unsigned char recvWithAck();

    /**
     * Receive a byte from a device. Never acknowledges back.
     * \return the received byte
     */
    static unsigned char recvWithNack();

private:
    SoftwareI2C();//Disallow creating instances, class is used via typedefs
};

template <typename SDA, typename SCL>
void SoftwareI2C<SDA, SCL>::init()
{
    SDA::mode(Mode::OPEN_DRAIN);
    SCL::mode(Mode::OPEN_DRAIN);
    SDA::high();
    SCL::high();
    sendStart();//Send dummy start-stop to initialize the bus
    sendStop();
}

template <typename SDA, typename SCL>
void SoftwareI2C<SDA, SCL>::sendStart()
{
    SDA::low();
    delayUs(3);
    SCL::low();
    delayUs(3);
}

template <typename SDA, typename SCL>
void SoftwareI2C<SDA, SCL>::sendStop()
{
    SDA::low();
    delayUs(3);
    SCL::high();
    delayUs(3);
    SDA::high();
    delayUs(3);
}

template <typename SDA, typename SCL>
bool SoftwareI2C<SDA, SCL>::send(unsigned char data)
{
    for(int i=0;i<8;i++)
    {
        if(data & 0x80) SDA::high(); else SDA::low();
        delayUs(3);
        SCL::high();
        data<<=1;
        delayUs(5);//Double delay
        SCL::low();
        delayUs(3);
    }
    SDA::high();
    delayUs(3);
    SCL::high();
    delayUs(3);
    bool result=(SDA::value()==0);
    delayUs(3);
    SCL::low();
    delayUs(3);
    return result;
}

template <typename SDA, typename SCL>
unsigned char SoftwareI2C<SDA, SCL>::recvWithAck()
{
    SDA::high();
    unsigned char result=0;
    for(int i=0;i<8;i++)
    {
        result<<=1;
        if(SDA::value()) result |= 1;
        delayUs(3);
        SCL::high();
        delayUs(5);//Double delay
        SCL::low();
        delayUs(3);
    }
    SDA::low();
    delayUs(3);
    SCL::high();
    delayUs(5);//Double delay
    SCL::low();
    delayUs(3);
    return result;
}

template <typename SDA, typename SCL>
unsigned char SoftwareI2C<SDA, SCL>::recvWithNack()
{
    SDA::high();
    unsigned char result=0;
    for(int i=0;i<8;i++)
    {
        result<<=1;
        if(SDA::value()) result |= 1;
        delayUs(3);
        SCL::high();
        delayUs(5);//Double delay
        SCL::low();
        delayUs(3);
    }
    delayUs(3);
    SCL::high();
    delayUs(5);//Double delay
    SCL::low();
    delayUs(3);
    return result;
}

} //namespace miosix

#endif	//SOFTWARE_I2C_H
