/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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

#include <termios.h>
#include <errno.h>
#include "filesystem/ioctl.h"
#include "rp2040_serial.h"

namespace miosix {

ssize_t RP2040PL011SerialBase::readBlock(void *buffer, size_t size, off_t where)
{
    if (size == 0) return 0;
    Lock<FastMutex> lock(rxMutex);
    uint8_t *bytes = reinterpret_cast<uint8_t *>(buffer);
    while(uart->fr & UART_UARTFR_RXFE_BITS) {}
    size_t i = 0;
    bytes[i++] = (uint8_t)uart->dr;
    while(i<size) {
        //Wait a bit for next character, but just a bit
        for(int j=0; j<20; j++) if(!(uart->fr & UART_UARTFR_RXFE_BITS)) break;
        if(uart->fr & UART_UARTFR_RXFE_BITS) break;
        bytes[i++] = (uint8_t)uart->dr;
    }
    return i;
}

ssize_t RP2040PL011SerialBase::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> lock(txMutex);
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(buffer);
    for (size_t i=0; i<size; i++)
    {
        while (uart->fr & UART_UARTFR_TXFF_BITS) {}
        uart->dr = bytes[i];
    }
    return size;
}

void RP2040PL011SerialBase::IRQwrite(const char *str)
{
    for (int i=0; str[i] != '\0'; i++)
    {
        while (uart->fr & UART_UARTFR_TXFF_BITS) {}
        uart->dr = str[i];
    }
}

int RP2040PL011SerialBase::ioctl(int cmd, void *arg)
{
    if(reinterpret_cast<unsigned>(arg) & 0b11) return -EFAULT; //Unaligned
    termios *t=reinterpret_cast<termios*>(arg);
    switch(cmd)
    {
        case IOCTL_SYNC:
            while (!(uart->fr & UART_UARTFR_TXFE_BITS)) {}
            return 0;
        case IOCTL_TCGETATTR:
            t->c_iflag=IGNBRK | IGNPAR;
            t->c_oflag=0;
            t->c_cflag=CS8;
            t->c_lflag=0;
            return 0;
        case IOCTL_TCSETATTR_NOW:
        case IOCTL_TCSETATTR_DRAIN:
        case IOCTL_TCSETATTR_FLUSH:
            //Changing things at runtime unsupported, so do nothing, but don't
            //return error as console_device.h implements some attribute changes
            return 0;
        default:
            return -ENOTTY; //Means the operation does not apply to this descriptor
    }
}

} // namespace miosix
