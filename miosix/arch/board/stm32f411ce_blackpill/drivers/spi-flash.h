/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *   Copyright (C) 2026 by Radu Raul                                       *    
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
#include <miosix.h>
#include <filesystem/devfs/devfs.h>
#include <kernel/sync.h>
#include <filesystem/ioctl.h>

/**
 * Class to access the raw spi flash memory
 */
class SPIFlash : public miosix::Device
{
public:
    SPIFlash();
    
    /**
     * \return the flash memory's size in bytes 
     */
    unsigned int size() const;
    
    virtual ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    virtual ssize_t writeBlock(const void *buffer, size_t size, off_t where);

    virtual int ioctl(int cmd, void *arg);

    void eraseBlock(off_t where);

    void doWrite(const char *data, int size);
    void doRead(char *data, int size);
    
    ~SPIFlash();
private:
    /**
     * Constructor
     */
    SPIFlash(const SPIFlash&)=delete;
    SPIFlash& operator= (const SPIFlash&)=delete;
    void dmaRxHandler();
    void dmaTxHandler();

    miosix::Thread *waiting;
    bool error;
    const unsigned long long eraseBlockSize=4*1024; // 4 KiB erase size
    const unsigned long long cardSize=4*1024*1024; // 32 Mib = 4 MiB 
    miosix::KernelMutex mutex{miosix::MutexOptions::RECURSIVE};
};
