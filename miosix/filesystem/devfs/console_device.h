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

#ifndef CONSOLE_DEVICE_H
#define	CONSOLE_DEVICE_H

#include "filesystem/file.h"
#include "filesystem/file_access.h"

namespace miosix {

/**
 * An extension to the FileBase interface that adds a new member function,
 * which is used by the kernel to write debug information before the kernel is
 * started or in case of serious errors, right before rebooting.
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class ConsoleDevice : public FileBase
{
public:
    /**
     * Constructor
     */
    ConsoleDevice() : FileBase(intrusive_ref_ptr<FilesystemBase>()) {}
    
    /**
     * Write a string to the Console.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * \param str the string to write. The string must be NUL terminated.
     */
    virtual void IRQwrite(const char *str)=0;
};

/**
 * Dummy device that ignores all writes an reads
 */
class NullConsoleDevice : public ConsoleDevice
{
public:
    /**
     * Write data to the file, if the file supports writing.
     * \param data the data to write
     * \param len the number of bytes to write
     * \return the number of written characters, or a negative number in case
     * of errors
     */
    virtual ssize_t write(const void *data, size_t len);
    
    /**
     * Read data from the file, if the file supports reading.
     * \param data buffer to store read data
     * \param len the number of bytes to read
     * \return the number of read characters, or a negative number in case
     * of errors
     */
    virtual ssize_t read(void *data, size_t len);
    
    /**
     * Move file pointer, if the file supports random-access.
     * \param pos offset to sum to the beginning of the file, current position
     * or end of file, depending on whence
     * \param whence SEEK_SET, SEEK_CUR or SEEK_END
     * \return the offset from the beginning of the file if the operation
     * completed, or a negative number in case of errors
     */
    virtual off_t lseek(off_t pos, int whence);
    
    /**
     * Return file information.
     * \param pstat pointer to stat struct
     * \return 0 on success, or a negative number on failure
     */
    virtual int fstat(struct stat *pstat) const;
    
    /**
     * Write a string to the Console.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * \param str the string to write. The string must be NUL terminated.
     */
    virtual void IRQwrite(const char *str);
};

/**
 * FIXME remove this when removing Console interface!
 */
class ConsoleAdapter : public ConsoleDevice
{
public:
    virtual ssize_t write(const void *data, size_t len);
    virtual ssize_t read(void *data, size_t len);
    virtual off_t lseek(off_t pos, int whence);
    virtual int fstat(struct stat *pstat) const;
    virtual int isatty() const;
    virtual void IRQwrite(const char *str);
};

/**
 * This class holds the file object related to the console, that is set by
 * the board support package, and used to populate /dev/console in DevFs
 */
class DefaultConsole
{
public:
    /**
     * \return an instance of this class (singleton) 
     */
    static DefaultConsole& instance();
    
    /**
     * Called by the board support package, in particular IRQbspInit(), to pass
     * to the kernel the console device. This device file is used as the default
     * one for stdin/stdout/stderr.
     * \param console device file handling console I/O. Can only be called with
     * interrupts disabled. 
     */
    void IRQset(intrusive_ref_ptr<ConsoleDevice> console);
    
    /**
     * \return the currently installed console device, wrapped in a TerminalDevice
     */
    intrusive_ref_ptr<TerminalDevice> get()
    {
        checkInit();
        return terminal;
    }
    
    /**
     * \return the currently installed console device.
     * Can be called with interrupts disabled or within an interrupt routine.
     */
    intrusive_ref_ptr<ConsoleDevice> IRQget()
    {
        checkInit();
        return rawConsole;
    }
    
private:    
    /**
     * Constructor, private as it is a singleton
     */
    DefaultConsole() {}
    
    /**
     * Check if the console has been set
     */
    void checkInit();
    
    DefaultConsole(const DefaultConsole&);
    DefaultConsole& operator= (const DefaultConsole&);
    
    intrusive_ref_ptr<ConsoleDevice> rawConsole; ///< The raw console device
    intrusive_ref_ptr<TerminalDevice> terminal;  ///< The wrapped console device
};

} //namespace miosix

#endif //CONSOLE_DEVICE_H
