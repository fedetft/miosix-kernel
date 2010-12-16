/***************************************************************************
 *   Copyright (C) 2008 by Terraneo Federico                               *
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
* serial.h Part of the Miosix Embedded OS.
* Interrupts based serial driver.
************************************************************************/

#ifndef SERIAL_H
#define SERIAL_H

namespace miosix {

/**
 * \internal
 * initializes serial port.
 * Can only be called when interrupts are disabled, inside an IRQ or before the
 * kernel is started. If you need to enable serial port from normal code,
 * disable interrupts before callng this function, and re-enable them after.
 * \param div to set baudrate use div=(peripheral clock)/(16*baudrate)
 */
void IRQserialInit(unsigned int div);

/**
 * \internal
 * This shuts down serial port. Hardware pins are released.
 * Calling write functions when serial port is disabled, will return
 * immediately.<br>
 * Calling read function will block the thread until serial is re-enabled and a
 * character is received.<br>
 * To enable serial port, use IRQserialInit.
 * Can only be called when interrupts are disabled, inside an IRQ or before the
 * kernel is started. If you need to disable serial port from normal code,
 * disable interrupts before callng this function, and re-enable them after.
 */
void IRQserialDisable();

/**
 * \internal
 * Can only be called when interrupts are disabled, inside an IRQ or before the
 * kernel is started. If you need to query serial port status from normal code,
 * disable interrupts before callng this function, and re-enable them after.
 * \return true if serial port is enabled.
 */
bool IRQisSerialEnabled();

/**
 * \internal
 * write data to serial port. Can be safely called by multiple threads.
 * Cannot be used if the kernel is not started, paused, or inside an IRQ.
 * \param str data to write
 * \param len length of data
 */
void serialWrite(const char *str, unsigned int len);

/**
 * \internal
 * \return true if all tx buffers (software and hardware) are empty. This
 * means that all data has been sent. Cannot be used if the kernel is not
 * started, paused, or inside an IRQ.
 */
bool serialTxComplete();

/**
 * \internal
 * read one char from serial port. (blocking) Can be safely called by multple
 * threads.<br>
 * Cannot be used if the kernel is not started, paused, or inside an IRQ.
 * \result char read
 */
char serialReadChar();

/**
 * \internal
 * Read a character from the serial port. Nonblocking.<br>
 * Can be safely called by multiple threads.
 * \param c a reference to the character to read. If the function returns
 * true then the read character is stored here.
 * \return true if the underlying communication channel was open and there
 * was data available. False otherwise.<br>
 * Cannot be used if the kernel is not started, paused, or inside an IRQ.
 */
bool serialReadCharNonblocking(char& c);

/**
 * \internal
 * \return true if some character in the rx queue has been lost and clears the
 * lost flag.<br>
 * Cannot be used if the kernel is not started, paused, or inside an IRQ.
 */
bool serialRxLost();

/**
 * \internal
 * Reset rx buffer.
 */
void serialRxFlush();

/**
 * \internal
 * Write a null terminated string to serial port. Can be used ONLY inside an
 * IRQ, and/or when the scheduler is not started or paused.<br>
 * Note that a call to this function will take precedence over a call to
 * serial_write_str, so the string will be printed before any character in the
 * software queue for serial_write_str, but after any character in the hardware
 * queue. Therefore, use of this function is only recomended for bootlogs
 * before the kernel is started and to warn about errors in IRQs.
 * \param str string to write
 */
void IRQserialWriteString(const char *str);

/**
 * \internal
 * Can be used after a call to IRQ_serial_write_string() to ensure all bytes
 * have been sent before rebooting the system.
 * \return true if the tx hardware buffer is empty.
 */
bool IRQserialTxFifoEmpty();

};//Namespace miosix

#endif //SERIAL_H
