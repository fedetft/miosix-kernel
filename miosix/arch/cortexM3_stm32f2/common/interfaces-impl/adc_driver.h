/* 
 * File:   adc_driver.h
 * Author: Alessandro Rizzi
 *
 * Created on 21 novembre 2012, 1.52
 */

#ifndef ADC_DRIVER_H
#define	ADC_DRIVER_H

#define GPIO_PORT_MASK		0x000f0000
#define GPIO_PIN_MASK		0x0000ff00
#define ADC_MASK		0x000000e0
#define ADC_CHANNEL_MASK	0x0000001f

#define POTENTIOMETER_ID	0x00050947

#include "interfaces/gpio.h"
#include "kernel/kernel.h"

namespace miosix {

    class AdcDriver {
    public:

        /**
         * Class constructor
         * It initializes the value of GPIO port and pin, ADC and its channel
         * @param deviceId
         * integer which contains all the information of GPIO, ADC, etc...
         * 
         * The structure is the following:
         * 
         * bits 0-4         ADC channel (0 the first, 1 the second, etc...)
         * bits 5-7         ADC (0 the first, 1 the second, etc...)
         * bits 8-15	GPIO pin
         * bits 16-19	GPIO port (A=0, B=1, C=2, etc...)
         * bits 20-31	Not used
         */
        AdcDriver(uint32_t deviceId);

        /**
         * Initialize the GPIO and the ADC selected by the deviceId
         */
        void init();

        /**
         * Capture a sample from the ADC
         * @return analog value of the selected GPIO
         */
        unsigned short read();

        /**
         * This is a shortcut to initialize the class and read a single value with only one instruction
         * @param deviceId 
         * integer which contains all the information of GPIO, ADC, etc...
         * @return analog value of the selected GPIO
         */
        static unsigned short read(uint32_t deviceId);

    private:

        /**
         * Extract from the deviceId the GPIO information (port, pin)
         * and the ADC ones (ADC, channel)
         * @param deviceId encoded information on the
         */
        void decodeDeviceId(uint32_t deviceId);

        /**
         * Initialize the selected GPIO
         */
        void initGPIO();

        /**
         * Initialize the selected ADC and the selected channel
         */
        void initADC();

        unsigned char gpioPort;
        unsigned char gpioPin;
        unsigned char ADCchannel;
        unsigned char numADC;

        static const uint32_t gpioMapping[9];
        static ADC_TypeDef* Adc[3];
        static const uint32_t ADC_Reg[3];

    };

};

#endif	/* ADC_DRIVER_H */

