#include "partition.h"
#include "MBR/mbr.h"

namespace miosix 
{

static PartitionType getPartitionTypeFromOSType(MBR::OSType osType)
{
    switch (osType) {
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
        default:
            return PartitionType::UNKNOWN; // There is no way to know if it is LITTLEFS or not, so we return UNKNOWN
    }
}

std::vector<std::pair<intrusive_ref_ptr<Partition>, PartitionType>> Partition::enumeratePartitions(
    intrusive_ref_ptr<Device> physicalDevice
) 
{
    // try MBR (GPT has always a protective MBR first so we can try this always)
    // MBR needs always to be enabled
    auto result = MBR::MBRReader::readMBR(physicalDevice);
    if (!result) {
        // we failed, there is no point in trying GPT since GPT always has a protective MBR 
        // maybe we are unipartitioned? But there is no way I can get a hint from that
        // so finding the partition type will be left to the caller.
        return {};
    }
    auto reader = *result;

    if (!reader.isProtectiveMBR()) 
    {
        auto partitions = std::vector<std::pair<intrusive_ref_ptr<Partition>, PartitionType>>(MBR::NUM_OF_PARTITIONS);

        auto mbrPartitions = reader.getPartitions();

        for (size_t i = 0; i < MBR::NUM_OF_PARTITIONS; i++) 
        {
            intrusive_ref_ptr<Partition> virtDevice;
            virtDevice = new Partition(
                physicalDevice, static_cast<unsigned long long>(mbrPartitions[i].startingLBA), 
                static_cast<unsigned long long>(mbrPartitions[i].sizeInLBA)
            );

            auto partType =getPartitionTypeFromOSType(mbrPartitions[i].getOsType());

            partitions[i] = std::make_pair<intrusive_ref_ptr<Partition>, PartitionType>(std::move(virtDevice), std::move(partType));
        }

        return partitions;
    }

    return {}; // we do not support GPT yet
    // try GPT
}

} //namespace miosix