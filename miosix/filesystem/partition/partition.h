/***************************************************************************
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
#include "filesystem/devfs/devfs.h"
#include "filesystem/ioctl.h"
#include "kernel/sync.h"
#include "miosix_settings.h"
#include "partition_type.h"
#include <vector>
#include "filesystem/partition/MBR/mbr.h"

namespace miosix 
{

enum class PartitionTableType
{
    MBR,
    GPT,
    INVALID,
    DISK_ERR
};

/**
 * This class represents a partition on a device. It is a subclass of Device,
 * and it is used to access the partition as if it were a separate device.
 * It is constructed with a backend device, a starting sector, and a sector count.
 * The readBlock and writeBlock methods are overridden to access the partition
 * in the backend device within the limits defined by startSector and sectorsCount.
 * This class is lock-free, as it relies on the backend device for synchronization 
 * and it is stateless w.r.t. the operations done on the partition. 
 */
class Partition : public Device {
public:
    Partition(intrusive_ref_ptr<Device> backend, unsigned long long startSector,
        unsigned long long sectorsCount) : Device(DeviceType::BLOCK), backend(backend), 
        startSector(startSector), sectorsCount(sectorsCount) 
    {};

    /**
     * Read a block of data
     * \param buffer buffer where read data will be stored
     * \param size buffer size
     * \param where where to read from
     * \return number of bytes read or a negative number on failure
     */
    virtual ssize_t readBlock(void *buffer, size_t size, off_t where)
    {
        auto whereLBA=where/512;
        auto sizeLBA=size/512;
        if (where<0 || static_cast<unsigned long long>(whereLBA+sizeLBA)>sectorsCount) 
        {
            return -EFAULT; // out of bounds
        }
        return backend->readBlock(buffer, size, startSector*512 + where);
    };

    /**
     * Write a block of data
     * \param buffer buffer where take data to write
     * \param size buffer size
     * \param where where to write to
     * \return number of bytes written or a negative number on failure
     */
    virtual ssize_t writeBlock(const void *buffer, size_t size, off_t where)
    {
        auto whereLBA=where/512;
        auto sizeLBA=size/512;
        if (where<0 || static_cast<unsigned long long>(whereLBA+sizeLBA)>sectorsCount) 
        {
            return -EFAULT; // out of bounds
        }
        return backend->writeBlock(buffer, size, startSector * 512 + where);
    };

    /**
     * Performs device-specific operations
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    virtual int ioctl(int cmd, void *arg) override
    {
        if (cmd == IOCTL_GET_VOLUME_SIZE)
        {
            unsigned long long* sizePtr=static_cast<unsigned long long*>(arg);
            if (sizePtr==nullptr) 
            {
                return -EINVAL;
            }
            *sizePtr=sectorsCount*512;
            return 0;
        }
        return backend->ioctl(cmd, arg);
    }
    
private:
    const intrusive_ref_ptr<Device> backend; ///< the device that contains the partition, can be physical or logical
    const unsigned long long startSector;    ///< starting sector of the partition in the backend device
    const unsigned long long sectorsCount;   ///< size in sectors
};

class DevicePartitionManager
{
public:
    DevicePartitionManager(intrusive_ref_ptr<Device> device) 
        : mbrReader{nullptr}, device{device}, type{PartitionTableType::INVALID} {};

    /**
     * @brief this method loads the partition table and checks for error
     * @returns MBR, GPR, INVALID if the loading fails or DISK_ERR if there is 
     * an error with the disk. 
     */
    PartitionTableType loadPartitionTable();

    std::pair<intrusive_ref_ptr<Partition>, PartitionType> getNextEntry();

    MBR::MBRFormatter beginFormatAsMBR(unsigned long uniqueTableId);

    PartitionTableType getTableType()
    {
        return type;
    }

private:
    std::unique_ptr<MBR::MBRReader> mbrReader;
    //std::unique_ptr<GPT::GPTReader> gptReader;
    intrusive_ref_ptr<Device> device;
    PartitionTableType type;
};

} //namespace miosix
