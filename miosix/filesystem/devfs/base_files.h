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

#ifndef BASE_FILES_H
#define	BASE_FILES_H

#include "devfs.h"
#include "config/miosix_settings.h"

#ifdef WITH_DEVFS

namespace miosix {

/**
 * A file where write operations do nothing at all
 */
class NullFile : public StatelessDeviceFile
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    NullFile() {}
    
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
};

/**
 * A file where read operations return zeroed out memory
 */
class ZeroFile : public StatelessDeviceFile
{
public:
    /**
     * Constructor
     * \param parent the filesystem to which this file belongs
     */
    ZeroFile() {}
    
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
};

/**
 * A DeviceFileGenerator that produces files like /proc/cpuinfo, with
 * a message that can be read by the application. Message is sampled when
 * the file is opened. 
 */
class MessageFileGenerator : public DeviceFileGenerator,
        private IntrusiveRefCountedSharedFromThis<MessageFileGenerator>
{
public:
    /**
     * Return an instance of the file type managed by this DeviceFileGenerator
     * \param file the file object will be stored here, if the call succeeds
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int open(intrusive_ref_ptr<FileBase>& file, int flags, int mode);
    
    /**
     * Set the returned message
     * \param message the new message
     */
    void setMessage(std::string message)
    {
        Lock<FastMutex> l(mutex);
        this->message=message;
    }
    
    /**
     * \return the current message
     */
    std::string getMessage() const
    {
        Lock<FastMutex> l(mutex);
        return message;
    }
    
private:
    /**
     * The stateful class which handles reading the message
     */
    class MessageFile : public StatefulDeviceFile
    {
    public:
        /**
         * Constructor
         * \param parent the filesystem to which this file belongs
         */
        MessageFile(intrusive_ref_ptr<DeviceFileGenerator> dfg, std::string m)
            : StatefulDeviceFile(dfg), message(m), index(0) {}
        
        /**
         * Write data to the file, if the file supports writing.
         * \param data the data to write
         * \param len the number of bytes to write
         * \return the number of written characters, or a negative number in
         * case of errors
         */
        virtual ssize_t write(const void *data, size_t len);
        
        /**
         * Read data from the file, if the file supports reading.
         * \param data buffer to store read data
         * \param len the number of bytes to read
         * \return the number of read characters, or a negative number in
         * case of errors
         */
        virtual ssize_t read(void *data, size_t len);
        
    private:
        FastMutex mutex;
        std::string message;
        int index;
    };
    
    mutable FastMutex mutex;
    std::string message;
};

} //namespace miosix

#endif //WITH_DEVFS

#endif //BASE_FILES_H
