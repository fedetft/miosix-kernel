/***************************************************************************
 *   Copyright (C) 2024,2025 by Daniele Cattaneo                           *
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

#pragma once

#include "filesystem/console/console_device.h"
#include "kernel/sync.h"
#include "kernel/queue.h"
#include "interfaces/arch_registers.h"
#include "arm_pl011_serial.h"

namespace miosix {

/**
 * RP2040 no DMA driver for the PL011 serial hardware.
 */
class RP2040PL011Serial : public PL011Serial
{
public:
    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(Error::UNEXPECTED) if the port is already being used by
     * another instance of this driver.
     * \param number usart number
     * \param baudrate serial port baudrate
     * \param tx GPIO to configure as usart tx, see datasheet for restrictions
     * \param rx GPIO to configure as usart rx, see datasheet for restrictions
     * \param rts GPIO to configure as usart rts, see datasheet for restrictions
     * \param cts GPIO to configure as usart cts, see datasheet for restrictions
     */
    RP2040PL011Serial(int number, int baudrate,
        GpioPin tx, GpioPin rx, GpioPin rts=GpioPin(), GpioPin cts=GpioPin())
        : PL011Serial(getBase(number), getIrqn(number), peripheralFrequency, 32+baudrate/500)
    {
        GlobalIrqLock lock;
        switch(number)
        {
            case 0:
                unreset_block_wait(RESETS_RESET_UART0_BITS);
                break;
            case 1:
                unreset_block_wait(RESETS_RESET_UART1_BITS);
                break;
            default:
                errorHandler(Error::UNEXPECTED);
        }
        bool hwFlowCtl = rts.isValid()||cts.isValid();
        initialize(lock,baudrate,hwFlowCtl);
        tx.function(Function::UART); tx.mode(Mode::OUTPUT);
        rx.function(Function::UART); rx.mode(Mode::INPUT);
        if(rts.isValid()) { rts.function(Function::UART); rts.mode(Mode::OUTPUT); }
        if(cts.isValid()) { cts.function(Function::UART); cts.mode(Mode::INPUT); }
    }

private:
    static void *getBase(int n) { return reinterpret_cast<void*>(n==0 ? uart0_hw : uart1_hw); }
    static unsigned int getIrqn(int n) { return n==0 ? UART0_IRQ_IRQn : UART1_IRQ_IRQn; }
};

} //namespace miosix
