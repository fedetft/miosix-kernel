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
#include "kernel/sync.h"

namespace miosix {

/**
 * An extension to the FileBase interface that adds a new member function,
 * which is used by the kernel to write debug information before the kernel is
 * started or in case of serious errors, right before rebooting.
 * This is not a pure virtual class, and the default implementation of all
 * member functions does nothing, acting as a null console. To have a working
 * console, board support packages should subclass it an implement read(),
 * write() and IRQwrite().
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<ConsoleDevice>
 */
class ConsoleDevice : public FileBase
{
public:
    /**
     * Constructor
     */
    ConsoleDevice() : FileBase(intrusive_ref_ptr<FilesystemBase>()) {}
    
    /**
     * This default implementation returns as if the data was written, but
     * does nothing
     */
    virtual ssize_t write(const void *data, size_t len);
    
    /**
     * This default implementation returns an error
     */
    virtual ssize_t read(void *data, size_t len);
    
    /**
     * This implementation claims to be a character device
     */
    virtual off_t lseek(off_t pos, int whence);
    
    /**
     * This implementation returns an error
     */
    virtual int fstat(struct stat *pstat) const;
    
    /**
     * This implementation claims to be a tty
     */
    virtual int isatty() const;
    
    /**
     * Write a string to the Console.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt. This default implementation ignores writes.
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
    virtual void IRQwrite(const char *str);
};

/**
 * Teriminal device, proxy object supporting additional terminal-specific
 * features
 */
class TerminalDevice : public FileBase
{
public:
    /**
     * Constructor
     * \param device proxed device.
     */
    TerminalDevice(intrusive_ref_ptr<FileBase> device);
    
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
     * Check whether the file refers to a terminal.
     * \return 1 if it is a terminal, 0 if it is not, or a negative number in
     * case of errors
     */
    virtual int isatty() const;
    
    /**
     * Wait until all I/O operations have completed on this file.
     * \return 0 on success, or a negative number in case of errors
     */
    virtual int sync();
    
    /**
     * Enables or disables echo of commands on the terminal
     * \param echo true to enable echo, false to disable it
     */
    void setEcho(bool echoMode) { echo=echoMode; }
    
    /**
     * \return true if echo is enabled 
     */
    bool isEchoEnabled() const { return echo; }
    
    /**
     * Selects whether the terminal sholud be transparent to non ASCII data
     * \param rawMode true if raw mode is required
     */
    void setBinary(bool binaryMode) { binary=binaryMode; }
    
    /**
     * \return true if the terminal allows binary data 
     */
    bool isBinary() const { return binary; }
    
private:
    intrusive_ref_ptr<FileBase> device;
    FastMutex mutex;
    bool echo;
    bool binary;
    bool skipNewline;
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
     * Notes: this has to be called in IRQbspInit(), since if it's called too
     * late the console gets initialized with a NullFile.
     * Also, calling this a second time to dynamically change the console device
     * is probably a bad idea, as the device is cached around in the filesystem
     * code and will result in some processes using the old device and some
     * other the new one.
     * \param console device file handling console I/O. Can only be called with
     * interrupts disabled. 
     */
    void IRQset(intrusive_ref_ptr<ConsoleDevice> console);
    
    /**
     * Same as IRQset(), but can be called with interrupts enabled
     * \param console device file handling console I/O. Can only be called with
     * interrupts disabled. 
     */
    void set(intrusive_ref_ptr<ConsoleDevice> console) { IRQset(console); }
    
    /**
     * \return the currently installed console device, wrapped in a
     * TerminalDevice
     */
    intrusive_ref_ptr<ConsoleDevice> get() { return console; }
    
    /**
     * \return the currently installed console device.
     * Can be called with interrupts disabled or within an interrupt routine.
     */
    intrusive_ref_ptr<ConsoleDevice> IRQget() { return console; }
    
    #ifndef WITH_FILESYSTEM
    /**
     * \return the terminal device, when filesystem support is disabled.
     * If filesystem is enabled, the terminal device can be found in the
     * FileDescriptorTable
     */
    intrusive_ref_ptr<TerminalDevice> getTerminal() { return terminal; }
    #endif //WITH_FILESYSTEM
    
private:    
    /**
     * Constructor, private as it is a singleton
     */
    DefaultConsole();
    
    DefaultConsole(const DefaultConsole&);
    DefaultConsole& operator= (const DefaultConsole&);
    
    intrusive_ref_ptr<ConsoleDevice>  console;  ///< The raw console device
    #ifndef WITH_FILESYSTEM
    intrusive_ref_ptr<TerminalDevice> terminal; ///< The wrapped console device
    #endif //WITH_FILESYSTEM
};

} //namespace miosix

#endif //CONSOLE_DEVICE_H
