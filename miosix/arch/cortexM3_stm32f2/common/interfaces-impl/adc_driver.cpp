/* 
 * File:   adc_driver.cpp
 * Author: Alessandro Rizzi
 * 
 * Created on 21 novembre 2012, 1.52
 */

#include "adc_driver.h"

namespace miosix {

    const uint32_t AdcDriver::gpioMapping[9] = {GPIOA_BASE, GPIOB_BASE, GPIOC_BASE, GPIOD_BASE, GPIOE_BASE, GPIOF_BASE, GPIOG_BASE, GPIOH_BASE, GPIOI_BASE};
    ADC_TypeDef* AdcDriver::Adc[3] = {ADC1, ADC2, ADC3};
    const uint32_t AdcDriver::ADC_Reg[3] = {RCC_APB2ENR_ADC1EN, RCC_APB2ENR_ADC2EN, RCC_APB2ENR_ADC3EN};

    AdcDriver::AdcDriver(uint32_t deviceId) {
        decodeDeviceId(deviceId);
    }

    void AdcDriver::init() {
        initGPIO();
        initADC();
    }

    unsigned short AdcDriver::read() {
        ADC_TypeDef *myADC = Adc[numADC];
        myADC->SQR3 = ADCchannel;
        myADC->CR2 |= ADC_CR2_SWSTART;
        while ((myADC->SR & ADC_SR_EOC) == 0);
        unsigned short data = myADC->DR;
        return data;
    }

    unsigned short AdcDriver::read(uint32_t deviceId) {
        AdcDriver adc(deviceId);
        adc.init();
        return adc.read();
    }

    void AdcDriver::decodeDeviceId(uint32_t deviceId) {
        ADCchannel = (unsigned char) (deviceId & ADC_CHANNEL_MASK);
        numADC = (unsigned char) ((deviceId & ADC_MASK) >> 5);
        gpioPin = (unsigned char) ((deviceId & GPIO_PIN_MASK) >> 8);
        gpioPort = (unsigned char) ((deviceId & GPIO_PORT_MASK) >> 16);
    }

    void AdcDriver::initGPIO() {
        GpioPin gpio(gpioMapping[gpioPort], gpioPin);
        gpio.mode(Mode::INPUT_ANALOG);
    }

    void AdcDriver::initADC() {
        InterruptDisableLock dLock; //Using the slow one so I don't care if kernel is started or not
        RCC->APB2ENR |= ADC_Reg[numADC];
        ADC_TypeDef *myADC = Adc[numADC];
        ADC->CCR |= ADC_CCR_ADCPRE_1; //ADC prescaler 84MHz/6=14MHz
        myADC->CR1 = 0;
        myADC->CR2 = ADC_CR2_ADON; //The first assignment sets the bit
        myADC->SQR1 = 0; //Do only one conversion
        myADC->SQR2 = 0;
        myADC->SQR3 = 0;
        myADC->SMPR1 = 7 << 18; //480 clock cycles of sample time for temp sensor		
    }

};