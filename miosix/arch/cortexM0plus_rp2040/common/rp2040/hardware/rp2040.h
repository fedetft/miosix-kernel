/** \name rp2040.h
 * Umbrella header for RP2040 hardware register definitions.
 */

#ifndef _HARDWARE_RP2040_H
#define _HARDWARE_RP2040_H

#include <CMSIS/Device/RaspberryPi/RP2040/Include/RP2040.h>
#include "regs/m0plus.h"
#include "structs/adc.h"
#include "structs/bus_ctrl.h"
#include "structs/clocks.h"
#include "structs/dma.h"
#include "structs/i2c.h"
#include "structs/interp.h"
#include "structs/iobank0.h"
#include "structs/ioqspi.h"
#include "structs/mpu.h"
#include "structs/nvic.h"
#include "structs/pads_qspi.h"
#include "structs/padsbank0.h"
#include "structs/pio.h"
#include "structs/pll.h"
#include "structs/psm.h"
#include "structs/pwm.h"
#include "structs/resets.h"
#include "structs/rosc.h"
#include "structs/rtc.h"
#include "structs/scb.h"
#include "structs/sio.h"
#include "structs/spi.h"
#include "structs/ssi.h"
#include "structs/syscfg.h"
#include "structs/systick.h"
#include "structs/timer.h"
#include "structs/uart.h"
#include "structs/usb.h"
#include "structs/vreg_and_chip_reset.h"
#include "structs/watchdog.h"
#include "structs/xip_ctrl.h"
#include "structs/xosc.h"

#endif