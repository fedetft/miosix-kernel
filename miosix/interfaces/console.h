/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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

#ifndef CONSOLE_H
#define	CONSOLE_H

namespace miosix {

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file console.h
 * This file only contains the Console class
 */

/**
 * This is an abstraction over a character device.
 * The kernel uses it to implement stdin/stdout/stderr I/O of the C and C++
 * standard library, and if bootlogs/error logs are enabled in miosix_settings.h
 * also these logs are redirected here.
 * The underlying implementation depends on the architecture chosen when
 * compiling Miosix and can be for example a serial port, an USB endpoint or
 * can also be empty (/dev/null like behaviour).
 *
 * The implementation of this class is in
 * arch/arch name/board name/interfaces-impl
 */
class Console
{
public:

    /**
     * Write a string to the Console. Can be safely called by multiple threads.
     * If the underlying communication channel is not open this function returns
     * without doing anything.
     * \param str the string to write. The string must be NUL terminated.
     */
    static void write(const char *str);

    /**
     * Write data to the Console.
     * Can be safely called by multiple threads. If the underlying communication
     * channel is not open this function returns without doing anything.
     * \param data data to write
     * \param length length of data
     */
    static void write(const char *data, int length);

    /**
     * Since the implementation of the Console class can use buffering, this
     * memeber function is provided to know if all data has been sent, for
     * example to wait until all data has been sent before performing a reboot.
     * \return true if all write buffers are empty.
     */
    static bool txComplete();

    /**
     * Write a string to the Console.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * If the underlying communication channel is not open this function returns
     * without doing anything.
     * If for some reason in an architecture Miosix is ported to it is not
     * possible to access the underlying communication channel when the kernel
     * is not started it is possible that this function always returns without
     * doing anything.
     * \param str the string to write. The string must be NUL terminated.
     */
    static void IRQwrite(const char *str);

    /**
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * Since the implementation of the Console class can use buffering, this
     * memeber function is provided to know if all data has been sent, for
     * example to wait until all data has been sent before performing a reboot.
     * \return true if all write buffers are empty.
     */
    static bool IRQtxComplete();

    /**
     * Read a character from the Console. Blocking.
     * Can be safely called by multiple threads. If the underlying communication
     * channel is not open this function blocks until it is opened and data
     * arrives.
     * \return the character read.
     */
    static char readChar();

    /**
     * Read a character from the Console. Nonblocking.
     * Can be safely called by multiple threads.
     * \param c a reference to the character to read. If the function returns
     * true then the read character is stored here.
     * \return true if the underlying communication channel was open and there
     * was data available. False otherwise.
     */
    static bool readCharNonBlocking(char& c);
    
};

/**
 * \}
 */

} //namespace miosix

#endif	/* CONSOLE_H */
