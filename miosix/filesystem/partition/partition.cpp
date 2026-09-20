#include "partition.h"
#include "MBR/mbr.h"

namespace miosix 
{

static PartitionType getPartitionTypeFromOSType(MBR::OSType osType)
{
    // There is no OS Type defined for LITTLEFS so we return UNKNOWN
    switch (osType) 
    {
    case MBR::OSType::FAT12:
        [[fallthrough]];
    case MBR::OSType::FAT16B:
        [[fallthrough]];
    case MBR::OSType::FAT16:
        [[fallthrough]];
    case MBR::OSType::FAT32LBA:
        [[fallthrough]];
    case MBR::OSType::FAT32CHS:
        return PartitionType::FAT32;
    case MBR::OSType::EXFAT:
        return PartitionType::EXFAT;
    case MBR::OSType::Empty:
        return PartitionType::NONE;
    default:
        return PartitionType::UNKNOWN; 
    }
}

PartitionTableType DevicePartitionManager::loadPartitionTable()
{
    if (!device) return PartitionTableType::DISK_ERR;

    // try MBR (GPT has always a protective MBR first so we can try this always)
    // MBR needs always to be enabled
    auto result=MBR::MBRReader::readMBR(device);
    if (!result) 
    {
        // we failed, there is no point in trying GPT since GPT always has a protective MBR 
        // maybe we are unipartitioned? But there is no way I can get a hint from that
        // so finding the partition type will be left to the caller.
        type=PartitionTableType::INVALID;
        return type;
    }
    
    mbrReader=std::move(*result);

    if (!mbrReader->isProtectiveMBR()) 
    {
        type=PartitionTableType::MBR;
        return type;
    }

    // we do not support GPT yet
    type=PartitionTableType::INVALID; 
    return type;
}

std::pair<intrusive_ref_ptr<Partition>, PartitionType> DevicePartitionManager::getNextEntry()
{
    if (type == PartitionTableType::MBR)
    {
        if (mbrReader==nullptr)
        {
            return {};
        }
        auto nextEntry=mbrReader->getNextPartitionEntry();
        if (nextEntry.isEmpty()) return {{}, PartitionType::NONE};
        
        intrusive_ref_ptr<Partition> virtDevice;
            virtDevice=new Partition(
                device, static_cast<unsigned long long>(nextEntry.startingLBA), 
                static_cast<unsigned long long>(nextEntry.sizeInLBA)
            );

            auto partType=getPartitionTypeFromOSType(nextEntry.getOsType());

            return std::make_pair<intrusive_ref_ptr<Partition>, PartitionType>(
                std::move(virtDevice), std::move(partType));
    }

    return {{}, PartitionType::UNKNOWN};
}

MBR::MBRFormatter DevicePartitionManager::beginFormatAsMBR(unsigned long uniqueTableId)
{
    return MBR::MBRFormatter(device, uniqueTableId);
}


} //namespace miosix