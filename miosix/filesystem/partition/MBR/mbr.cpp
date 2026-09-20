#include "mbr.h"
#include <cstdio>
#include "filesystem/ioctl.h"
#include "filesystem/partition/partition.h"
#include "filesystem/fat32/fat32.h"
#include "filesystem/exfat/exfat.h"
#include "filesystem/littlefs/lfs_miosix.h"

#define DBG(x) iprintf(x)
//#define DBG(x)

using namespace miosix;
namespace MBR {
std::expected<std::unique_ptr<MBRReader>, bool> MBRReader::readMBR(
    miosix::intrusive_ref_ptr<miosix::Device> device) 
{
    std::unique_ptr<MBRReader> reader=std::make_unique<MBRReader>();
    auto result=device->readBlock(&reader->header, sizeof(MBRHeader), MBR_POSITION_LBA);
    if (result<0) {
        return std::unexpected(true);
    } 
    return reader;
}

bool MBRReader::isValidMBR() {
    return header.mbrSignature==MBR_SIGNATURE;
}

bool MBRReader::isProtectiveMBR() {
    return header.partitionRecords[0]
        .osTypeAndEndingCHS[0]==static_cast<uint8_t>(OSType::ProtectiveMBR);
}

MBRPartitionRecord MBRReader::getNextPartitionEntry()
{
    if (currEntryIdx>=NUM_OF_PARTITIONS)
        return MBRPartitionRecord::createEmpty();

    return header.partitionRecords[currEntryIdx];
    currEntryIdx++;
}

MBRFormatter::MBRFormatter(miosix::intrusive_ref_ptr<Device> device, uint32_t uniqueSignature)
 : device{device}, deviceSize{0}, partitionIndex{0}
{
    device->ioctl(IOCTL_GET_VOLUME_SIZE, &deviceSize);
    memset(&header, 0, sizeof(MBRHeader));
    header.unknown=0;
    header.mbrSignature=MBR_SIGNATURE;
    header.uniqueMBRSignature=uniqueSignature;
}

MBRFormatResult MBRFormatter::addPartition(PartitionType type,
     unsigned long long size, unsigned long long position)
{
    if (partitionIndex>=4)
    {
        return MBRFormatResult::TooManyPartitions;
    }

    if (size==0 || position<512 || size%512!=0 || position%512!=0)
    {
        return MBRFormatResult::BoundariesNotAlignedToLogicBlock;
    }

    if (position+size>deviceSize)
    {
        return MBRFormatResult::PartitionTooBig;
    }

    // for us past space is occupied, even if there is some free.
    // this allows for not having to check boundaries and there is 
    // no reason for someone to not write partitions in order
    const auto sizeLBA=size/512;
    const auto positionLBA=position/512;
    if(partitionIndex>0)
    {
        const auto& partitionEntry=header.partitionRecords[partitionIndex-1];
    
        if (positionLBA<partitionEntry.startingLBA+partitionEntry.sizeInLBA)
        {
            return MBRFormatResult::PartitionPositionMustIncrease;
        }
    }

    // also it allows us to virtually shrink the device size
    deviceSize-=position+size; 

    auto& partitionEntry=header.partitionRecords[partitionIndex];

    partitionEntry.bootIndicatorAndStartingCHS[0]=0; // never mark as boot
    partitionEntry.bootIndicatorAndStartingCHS[1]=0; // do we need to calculate the CHS addresses?
    partitionEntry.bootIndicatorAndStartingCHS[2]=0; 
    partitionEntry.bootIndicatorAndStartingCHS[3]=0;

    switch (type)
    {
#ifdef WITH_FATFS
#ifdef WITH_EXFAT
        case PartitionType::EXFAT:
            partitionEntry.osTypeAndEndingCHS[0]=static_cast<uint8_t>(OSType::EXFAT);
            break;
#endif
        case PartitionType::FAT32:
            partitionEntry.osTypeAndEndingCHS[0]=static_cast<uint8_t>(OSType::FAT32LBA);
            break;
#endif
#ifdef WITH_LITTLEFS
        case PartitionType::LITTLEFS:
            partitionEntry.osTypeAndEndingCHS[0]=static_cast<uint8_t>(OSType::MIOSIX_LITTLEFS);
            break;
#endif
        default:
            return MBRFormatResult::InvalidPartitionType;
    }

    partitionEntry.osTypeAndEndingCHS[1]=0;
    partitionEntry.osTypeAndEndingCHS[2]=0;
    partitionEntry.osTypeAndEndingCHS[3]=0;
    partitionEntry.sizeInLBA=sizeLBA;
    partitionEntry.startingLBA=positionLBA;

    partitionIndex++;

    return MBRFormatResult::Ok;
}

MBRFormatResult MBRFormatter::addPartition(PartitionType type,
     unsigned long long size)
{
    if (partitionIndex==0) {
        return addPartition(type, size, 512); // we start at the first LBA since LBA 0 is reserved for the header
    } 

    const auto& partitionEntry=header.partitionRecords[partitionIndex-1];

    return addPartition(type, size, (partitionEntry.startingLBA+partitionEntry.sizeInLBA)*512);
}

int MBRFormatter::writeMBRToDisk()
{
    //Well, committing the table is just a write
    return device->writeBlock(&header, sizeof(MBRHeader), 0);
}

} //namespace MBR