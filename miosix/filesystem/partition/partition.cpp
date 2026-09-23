#include "partition.h"
#include "MBR/mbr.h"
#include "GPT/gpt.h"

#ifdef WITH_FILESYSTEM

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

    {
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
    }
#ifdef WITH_GPT
    // we are in a GPT partitioned device since the MBR is protective
    {
        auto result=GPT::GPTReader::readGPT(device, true);
        if (!result)
        {
            type=PartitionTableType::INVALID;
            return type;
        }

        auto gptHeaderReader=std::move(*result);

        if (gptHeaderReader->checkGPT() != GPT::ReaderResult::Ok)
        {
            type=PartitionTableType::INVALID;
            return type;
        }

        gptReader=gptHeaderReader->getPrimaryPartitionTableReader();
        type=PartitionTableType::GPT;
        return type;
    }
#endif

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
#ifdef WITH_GPT
    else if (type==PartitionTableType::GPT) {
        if (gptReader==nullptr)
        {
            return {{}, PartitionType::UNKNOWN};
        }

        auto nextEntry=gptReader->getNextPartitionEntry();
        if (!nextEntry)
        {
            return {{}, PartitionType::UNKNOWN};
        }

        if (nextEntry->isEmpty())
        {
            return {{}, PartitionType::NONE};
        }

        intrusive_ref_ptr<Partition> virtDevice;
        virtDevice=new Partition(
            device, static_cast<unsigned long long>(nextEntry->startingLBA), 
            static_cast<unsigned long long>(nextEntry->endingLBA - nextEntry->startingLBA)
        );

        // By Raul Radu: we have hints from GPT
        // but it does not make sense, we have a very big ID space thanks to the GUID
        // used for identifying the partition type.
        // And we get only hints? Like the FAT family is part of the bigger GPT
        // partition type family Microsoft Basic Data Partitions along with NTFS?
        // why? So for now, to not have to check for useless hints, we return
        // UNKWOWN as the partition hint so the mounter will directly scan for
        // all partition types (luckily we have just a few) or will try to mount
        // the formatOnFail partition type.
        return {std::move(virtDevice), PartitionType::UNKNOWN};
    }
    #endif

    return {{}, PartitionType::UNKNOWN};
}

MBR::MBRFormatter DevicePartitionManager::beginFormatAsMBR(unsigned long uniqueTableId)
{
    return MBR::MBRFormatter(device, uniqueTableId);
}

} //namespace miosix

#endif