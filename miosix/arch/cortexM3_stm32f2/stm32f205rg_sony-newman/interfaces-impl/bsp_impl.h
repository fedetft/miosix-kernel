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
* bsp_impl.h Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#ifndef BSP_IMPL_H
#define BSP_IMPL_H

#include "config/miosix_settings.h"
#include "hwmapping.h"
#include "drivers/stm32_hardware_rng.h"
#include "drivers/stm32f2_f4_i2c.h"
#include "kernel/sync.h"

namespace miosix {

//No LEDs in this board
inline void ledOn()  {}
inline void ledOff() {}

/**
 * You <b>must</b> lock this mutex before accessing the I2C1Driver directly
 * on this board, as there are multiple threads that access the I2C for
 * different purposes (touchscreen, accelerometer, PMU). If you don't do it,
 * your application will crash sooner or later.
 */
extern FastMutex i2cMutex;

enum {
     PMU_I2C_ADDRESS=0x90,  ///< I2C Address of the PMU
     TOUCH_I2C_ADDRESS=0x0a,///< I2C Address of the touchscreen controller
     ACCEL_I2C_ADDRESS=0x30 ///< I2C Address of the accelerometer
};

/**
 * Helper function to write a register of an I2C device. Don't forget to lock
 * i2cMutex before calling this.
 * \param i2c the I2C driver
 * \param dev device address (PMU_I2C_ADDRESS)
 * \param reg register address
 * \param data byte to write
 * \return true on success, false on failure
 */
bool i2cWriteReg(miosix::I2C1Driver& i2c, unsigned char dev, unsigned char reg,
        unsigned char data);

/**
 * Helper function to write a register of an I2C device. Don't forget to lock
 * i2cMutex before calling this.
 * \param i2c the I2C driver
 * \param dev device address (PMU_I2C_ADDRESS)
 * \param reg register address
 * \param data byte to write
 * \return true on success, false on failure
 */
bool i2cReadReg(miosix::I2C1Driver& i2c, unsigned char dev, unsigned char reg,
        unsigned char& data);

/**
 * This class contains all what regards power management on the watch.
 * The class can be safely used by multiple threads concurrently.
 */
class PowerManagement
{
public:
    /**
     * \return an instance of the power management class (singleton) 
     */
    static PowerManagement& instance();
    
    /**
     * \return true if the USB cable is connected 
     */
    bool isUsbConnected() const;
    
    /**
     * \return true if the battery is currently charging. For this to happen,
     * charging must be allowed, the USB cable must be connected, and the
     * battery must not be fully charged. 
     */
    bool isCharging();
    
    /** 
     * \return the battery charge status as a number in the 0..100 range.
     * The reading takes ~5ms 
     */
    int getBatteryStatus();
    
    /**
     * \return the battery voltage, in millivolts. So 3700 means 3.7V. 
     * The reading takes ~5ms 
     */
    int getBatteryVoltage();
    
private:
    PowerManagement(const PowerManagement&);
    PowerManagement& operator=(const PowerManagement&);
    
    /**
     * Constructor
     */
    PowerManagement();
    
    I2C1Driver &i2c;
    bool chargingAllowed;
    FastMutex batteryMutex;
};

/**
 * This allows to retrieve the light value
 * The class can be safely used by multiple threads concurrently.
 */
class LightSensor
{
public:
    /**
     * \return an instance of the power management class (singleton) 
     */
    static LightSensor& instance();
    
    /**
     * \return the light value. The reading takes ~5ms 
     */
    int read();
    
private:
    LightSensor(const LightSensor&);
    LightSensor& operator=(const LightSensor&);
    
    /**
     * Constructor
     */
    LightSensor();
    
    FastMutex lightMutex;
};



} //namespace miosix

#endif //BSP_IMPL_H
