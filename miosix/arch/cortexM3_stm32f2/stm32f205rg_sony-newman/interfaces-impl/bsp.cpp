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

namespace miosix {

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
    OLED_nSS_Pin::speed(Speed::_100MHz); //Without changing the default speed
    OLED_SCK_Pin::mode(Mode::ALTERNATE); //OLED does not work!
    OLED_SCK_Pin::alternateFunction(5);
    OLED_SCK_Pin::speed(Speed::_100MHz);
    OLED_MOSI_Pin::mode(Mode::ALTERNATE);
    OLED_MOSI_Pin::alternateFunction(5);
    OLED_MOSI_Pin::speed(Speed::_100MHz);
    OLED_A0_Pin::mode(Mode::OUTPUT);
    OLED_A0_Pin::low();
    OLED_A0_Pin::speed(Speed::_100MHz);
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

};//namespace miosix
