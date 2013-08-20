/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/ 

/***********************************************************************
* bsp.cpp Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#include <cstdlib>
#include "interfaces/bsp.h"
#include "kernel/kernel.h"
#include "interfaces/delays.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"
#include <algorithm>

using namespace std;

namespace miosix {

FastMutex i2cMutex;

bool i2cWriteReg(miosix::I2C1Driver& i2c, unsigned char dev, unsigned char reg,
        unsigned char data)
{
    const unsigned char buffer[]={reg,data};
    return i2c.send(dev,buffer,sizeof(buffer));
}

bool i2cReadReg(miosix::I2C1Driver& i2c, unsigned char dev, unsigned char reg,
        unsigned char& data)
{
    if(i2c.send(dev,&reg,1)==false) return false;
    unsigned char temp;
    if(i2c.recv(dev,&temp,1)==false) return false;
    data=temp;
    return true;
}

//
// Initialization
//

void IRQbspInit()
{
    //Enable all gpios
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN
                  | RCC_AHB1ENR_GPIOBEN
                  | RCC_AHB1ENR_GPIOCEN;

    using namespace oled;
    OLED_nSS_Pin::mode(Mode::OUTPUT);
    OLED_nSS_Pin::high();
    OLED_nSS_Pin::speed(Speed::_50MHz); //Without changing the default speed
    OLED_SCK_Pin::mode(Mode::ALTERNATE); //OLED does not work!
    OLED_SCK_Pin::alternateFunction(5);
    OLED_SCK_Pin::speed(Speed::_50MHz);
    OLED_MOSI_Pin::mode(Mode::ALTERNATE);
    OLED_MOSI_Pin::alternateFunction(5);
    OLED_MOSI_Pin::speed(Speed::_50MHz);
    OLED_A0_Pin::mode(Mode::OUTPUT);
    OLED_A0_Pin::low();
    OLED_A0_Pin::speed(Speed::_50MHz);
    OLED_Reset_Pin::mode(Mode::OUTPUT);
    OLED_Reset_Pin::low();
    OLED_Reset_Pin::speed(Speed::_50MHz);
    OLED_V_ENABLE_Pin::mode(Mode::OUTPUT);
    OLED_V_ENABLE_Pin::low();
    OLED_V_ENABLE_Pin::speed(Speed::_50MHz);
    
    using namespace touch;
    Touch_Reset_Pin::mode(Mode::OUTPUT);
    Touch_Reset_Pin::low();
    Touch_Reset_Pin::speed(Speed::_50MHz);
    TOUCH_WKUP_INT_Pin::mode(Mode::INPUT);
    
    using namespace power;
    BATT_V_ON_Pin::mode(Mode::OUTPUT);
    BATT_V_ON_Pin::low();
    BATT_V_ON_Pin::speed(Speed::_50MHz);
    BAT_V_Pin::mode(Mode::INPUT_ANALOG);
    ENABLE_LIGHT_SENSOR_Pin::mode(Mode::OUTPUT);
    ENABLE_LIGHT_SENSOR_Pin::low();
    ENABLE_LIGHT_SENSOR_Pin::speed(Speed::_50MHz);
    LIGHT_SENSOR_ANALOG_OUT_Pin::mode(Mode::INPUT_ANALOG);
    ENABLE_2V8_Pin::mode(Mode::OUTPUT);
    ENABLE_2V8_Pin::low();
    ENABLE_2V8_Pin::speed(Speed::_50MHz);
    HoldPower_Pin::mode(Mode::OPEN_DRAIN);
    HoldPower_Pin::high();
    HoldPower_Pin::speed(Speed::_50MHz);
    
    ACCELEROMETER_INT_Pin::mode(Mode::INPUT_PULL_DOWN);
    
    using namespace i2c;
    I2C_SCL_Pin::mode(Mode::ALTERNATE);
    I2C_SCL_Pin::alternateFunction(4);
    I2C_SCL_Pin::speed(Speed::_50MHz);
    I2C_SDA_Pin::mode(Mode::ALTERNATE);
    I2C_SDA_Pin::alternateFunction(4);
    I2C_SDA_Pin::speed(Speed::_50MHz);
    
    BUZER_PWM_Pin::mode(Mode::OUTPUT);
    BUZER_PWM_Pin::low();
    BUZER_PWM_Pin::speed(Speed::_50MHz);
    
    POWER_BTN_PRESS_Pin::mode(Mode::INPUT);
    
    using namespace usb;
    USB5V_Detected_Pin::mode(Mode::INPUT_PULL_DOWN);
    USB_DM_Pin::mode(Mode::INPUT);
    USB_DP_Pin::mode(Mode::INPUT);
    
    using namespace bluetooth;
    Reset_BT_Pin::mode(Mode::OPEN_DRAIN);
    Reset_BT_Pin::low();
    Reset_BT_Pin::speed(Speed::_50MHz);
    BT_CLK_REQ_Pin::mode(Mode::INPUT);
    HOST_WAKE_UP_Pin::mode(Mode::INPUT);
    Enable_1V8_BT_Power_Pin::mode(Mode::OPEN_DRAIN);
    Enable_1V8_BT_Power_Pin::high();
    Enable_1V8_BT_Power_Pin::speed(Speed::_50MHz);
    BT_nSS_Pin::mode(Mode::OUTPUT);
    BT_nSS_Pin::low();
    BT_nSS_Pin::speed(Speed::_50MHz);
    BT_SCK_Pin::mode(Mode::OUTPUT);
    BT_SCK_Pin::low();
    BT_SCK_Pin::speed(Speed::_50MHz);
    BT_MISO_Pin::mode(Mode::INPUT_PULL_DOWN);
    BT_MOSI_Pin::mode(Mode::OUTPUT);
    BT_MOSI_Pin::low();
    BT_MOSI_Pin::speed(Speed::_50MHz);
    
    using namespace unknown;
    WKUP_Pin::mode(Mode::INPUT);
    MCO1_Pin::mode(Mode::ALTERNATE);
    MCO1_Pin::alternateFunction(0);
    MCO1_Pin::speed(Speed::_100MHz);
    Connect_USB_Pin::mode(Mode::OPEN_DRAIN);
    Connect_USB_Pin::low();
    Connect_USB_Pin::speed(Speed::_50MHz);
    POWER_3V3_ON_1V8_OFF_Pin::mode(Mode::OUTPUT);
    POWER_3V3_ON_1V8_OFF_Pin::low();
    POWER_3V3_ON_1V8_OFF_Pin::speed(Speed::_50MHz);
    SPI2_nSS_Pin::mode(Mode::OUTPUT);
    SPI2_nSS_Pin::high();
    SPI2_nSS_Pin::speed(Speed::_50MHz);
    SPI2_SCK_Pin::mode(Mode::OUTPUT);
    SPI2_SCK_Pin::low();
    SPI2_SCK_Pin::speed(Speed::_50MHz);
    SPI2_MISO_Pin::mode(Mode::INPUT_PULL_DOWN);
    SPI2_MOSI_Pin::mode(Mode::OUTPUT);
    SPI2_MOSI_Pin::low();
    SPI2_MOSI_Pin::speed(Speed::_50MHz);
    
    // Taken from underverk's SmartWatch_Toolchain/src/system.c:
    // Prevents hard-faults when booting from USB
    delayMs(50);

    USB_DP_Pin::mode(Mode::INPUT_PULL_UP); //Never leave GPIOs floating
    USB_DM_Pin::mode(Mode::INPUT_PULL_DOWN);
}

void bspInit2()
{
    PowerManagement::instance(); //This initializes the PMU
    BUZER_PWM_Pin::high();
    Thread::sleep(200);
    BUZER_PWM_Pin::low();
    //Wait for user to release the button
    while(POWER_BTN_PRESS_Pin::value()) Thread::sleep(20);
}

//
// Shutdown and reboot
//

void shutdown()
{
    // Taken from underverk's SmartWatch_Toolchain/src/Arduino/Arduino.cpp
    disableInterrupts();
    BUZER_PWM_Pin::high();
    delayMs(200);
    BUZER_PWM_Pin::low();
    while(POWER_BTN_PRESS_Pin::value()) ;
    //This is likely wired to the PMU. If the USB cable is not connected, this
    //cuts off the power to the microcontroller. But if USB is connected, this
    //does nothing. In this case we can only spin waiting for the user to turn
    //the device on again
    power::HoldPower_Pin::low();
    delayMs(500);
    while(POWER_BTN_PRESS_Pin::value()==0) ;
    reboot();
}

void reboot()
{
    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

//As usual, since the PMU datasheet is unavailable (we don't even know what
//chip it is), these are taken from underverk's code
#define CHGSTATUS               0x01

#define CH_ACTIVE_MSK           0x08

#define CHGCONFIG0              0x02

#define VSYS_4_4V               0x40
#define VSYS_5V                 0x80
#define ACIC_100mA_DPPM_ENABLE  0x00
#define ACIC_500mA_DPPM_ENABLE  0x10
#define ACIC_500mA_DPPM_DISABLE 0x20
#define ACIC_USB_SUSPEND        0x20
#define TH_LOOP                 0x08
#define DYN_TMR                 0x04
#define TERM_EN                 0x02
#define CH_EN                   0x01

#define CHGCONFIG1              0x03
#define I_PRE_05                0x00
#define I_PRE_10                0x40
#define I_PRE_15                0x80
#define I_PRE_20                0xC0

#define DEFDCDC                 0x07
#define DCDC1_DEFAULT           0x29
#define DCDC_DISCH              0x40
#define HOLD_DCDC1              0x80

#define ISET_25                 0x00
#define ISET_50                 0x10
#define ISET_75                 0x20
#define ISET_100                0x30

#define I_TERM_05               0x00
#define I_TERM_10               0x04
#define I_TERM_15               0x08
#define I_TERM_20               0x0C

#define CHGCONFIG2              0x04
#define SFTY_TMR_4h             0x0
#define SFTY_TMR_5h             0x40
#define SFTY_TMR_6h             0x80
#define SFTY_TMR_8h             0xC0

#define PRE_TMR_30m             0x0
#define PRE_TMR_60m             0x20

#define NTC_100k                0x0
#define NTC_10k                 0x8

#define V_DPPM_VBAT_100mV       0x0
#define V_DPPM_4_3_V            0x04

#define VBAT_COMP_ENABLE        0x02
#define VBAT_COMP_DISABLE       0x00

//
// class PowerManagement
//

PowerManagement& PowerManagement::instance()
{
    static PowerManagement singleton;
    return singleton;
}

bool PowerManagement::isUsbConnected() const
{
    return usb::USB5V_Detected_Pin::value();
}

bool PowerManagement::isCharging()
{
    Lock<FastMutex> l(i2cMutex);
    unsigned char chgstatus;
    //During testing the i2c command never failed. If it does, we lie and say
    //we're not charging
    if(i2cReadReg(i2c,PMU_I2C_ADDRESS,CHGSTATUS,chgstatus)==false) return false;
    return (chgstatus & CH_ACTIVE_MSK)!=0;
}

int PowerManagement::getBatteryStatus()
{
    const int battCharged=4000; //4.0V
    const int battDead=3000; //3V
    return max(0,min(100,(getBatteryVoltage()-battDead)*100/(battCharged-battDead)));
}

int PowerManagement::getBatteryVoltage()
{
    Lock<FastMutex> l(batteryMutex);
    power::BATT_V_ON_Pin::high(); //Enable battry measure circuitry
    ADC1->CR2=ADC_CR2_ADON; //Turn ADC ON
    Thread::sleep(5); //Wait for voltage to stabilize
    ADC1->CR2 |= ADC_CR2_SWSTART; //Start conversion
    while((ADC1->SR & ADC_SR_EOC)==0) ; //Wait for conversion
    int result=ADC1->DR; //Read result
    ADC1->CR2=0; //Turn ADC OFF
    power::BATT_V_ON_Pin::low(); //Disable battery measure circuitry
    return result*4440/4095;
}

PowerManagement::PowerManagement() : i2c(I2C1Driver::instance()),
        chargingAllowed(true)
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    }
    ADC1->CR1=0;
    ADC1->CR2=0; //Keep the ADC OFF to save power
    ADC1->SMPR2=ADC_SMPR2_SMP2_0; //Sample for 15 cycles channel 2 (battery)
    ADC1->SQR1=0; //Do only one conversion
    ADC1->SQR2=0;
    ADC1->SQR3=2; //Convert channel 2 (battery voltage)
    
    unsigned char config0=VSYS_4_4V
                        | ACIC_100mA_DPPM_ENABLE
                        | TH_LOOP
                        | DYN_TMR
                        | TERM_EN
                        | CH_EN;
    unsigned char config1=I_TERM_10
                        | ISET_100
                        | I_PRE_10;
    unsigned char config2=SFTY_TMR_5h
                        | PRE_TMR_30m
                        | NTC_10k
                        | V_DPPM_4_3_V
                        | VBAT_COMP_ENABLE;
    unsigned char defdcdc=DCDC_DISCH
                        | DCDC1_DEFAULT;
    Lock<FastMutex> l(i2cMutex);
    bool error=false;
    if(i2cWriteReg(i2c,PMU_I2C_ADDRESS,CHGCONFIG0,config0)==false) error=true;
    if(i2cWriteReg(i2c,PMU_I2C_ADDRESS,CHGCONFIG1,config1)==false) error=true;
    if(i2cWriteReg(i2c,PMU_I2C_ADDRESS,CHGCONFIG2,config2)==false) error=true;
    if(i2cWriteReg(i2c,PMU_I2C_ADDRESS,DEFDCDC,defdcdc)==false) error=true;
    if(!error) return;
    //Should never happen
    for(int i=0;i<10;i++)
    {
        BUZER_PWM_Pin::high();
        Thread::sleep(200);
        BUZER_PWM_Pin::low();
        Thread::sleep(200);
    }
}

//
// class LightSensor
//

LightSensor& LightSensor::instance()
{
    static LightSensor singleton;
    return singleton;
}

int LightSensor::read()
{
    Lock<FastMutex> l(lightMutex);
    power::ENABLE_LIGHT_SENSOR_Pin::high(); //Enable battry measure circuitry
    ADC2->CR2=ADC_CR2_ADON; //Turn ADC ON
    Thread::sleep(5); //Wait for voltage to stabilize
    ADC2->CR2 |= ADC_CR2_SWSTART; //Start conversion
    while((ADC2->SR & ADC_SR_EOC)==0) ; //Wait for conversion
    int result=ADC2->DR; //Read result
    ADC2->CR2=0; //Turn ADC OFF
    power::ENABLE_LIGHT_SENSOR_Pin::low(); //Disable battery measure circuitry
    return result;
}

LightSensor::LightSensor()
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
    }
    ADC2->CR1=0;
    ADC2->CR2=0; //Keep the ADC OFF to save power
    ADC2->SMPR1=ADC_SMPR1_SMP14_0; //Sample for 15 cycles channel 14
    ADC2->SQR1=0; //Do only one conversion
    ADC2->SQR2=0;
    ADC2->SQR3=14; //Convert channel 14 (light sensor)
}

} //namespace miosix
