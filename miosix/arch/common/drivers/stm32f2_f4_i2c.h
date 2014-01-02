
#include <interfaces/arch_registers.h>

#ifndef STM32F2_I2C_H
#define	STM32F2_I2C_H

namespace miosix {

/**
 * Driver for the I2C1 peripheral in STM32F2 and STM32F4 under Miosix
 */
class I2C1Driver
{
public:
    /**
     * \return an instance of this class (singleton)
     */
    static I2C1Driver& instance();
    
    /**
     * Initializes the peripheral. The only supported mode is 100KHz, master,
     * 7bit address. Note that there is no need to manually call this member
     * function as the constructor already inizializes the I2C peripheral.
     * The only use of this member function is to reinitialize the peripheral
     * if the microcontroller clock frequency or the APB prescaler is changed.
     */
    void init();
    
    /**
     * Send data to a device connected to the I2C bus
     * \param address device address (bit 0 is forced at 0)
     * \param data pointer with data to send
     * \param len length of data to send
     * \return true on success, false on failure
     */
    bool send(unsigned char address, const void *data, int len);
    
    /**
     * Receive data from a device connected to the I2C bus
     * \param address device address (bit 0 is forced at 1) 
     * \param data pointer to a buffer where data will be received
     * \param len length of data to receive
     * \return true on success, false on failure
     */
    bool recv(unsigned char address, void *data, int len);
    
private:
    I2C1Driver(const I2C1Driver&);
    I2C1Driver& operator=(const I2C1Driver&);
    
    /**
     * Constructor. Initializes the peripheral except the GPIOs, that must be
     * set by the caller to the appropriate alternate function mode prior to
     * creating an instance of this class.
     * \param i2c pinter to the desired I2C peripheral, such as I2C1, I2C2, ...
     */
    I2C1Driver() { init(); }
    
    /**
     * Send a start condition
     * \param address 
     * \param immediateNak
     * \return 
     */
    bool start(unsigned char address, bool immediateNak=false);
    
    /**
     * Wait until until an interrupt occurs during the send start bit and
     * send address phases of the i2c communication.
     * \return true if the operation was successful, false on error
     */
    bool waitStatus1();
};

} //namespace miosix

#endif //STM32F2_I2C_H
