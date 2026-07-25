#pragma once
#include <cstdint>
#include <expected>
#include "miosix_settings.h"
#include "filesystem/devfs/devfs.h"

namespace MBR {

constexpr off_t    MBR_POSITION_LBA    = 0;
constexpr size_t   MBR_BOOT_CODE_SIZE  = 424;
constexpr uint16_t MBR_SIGNATURE       = 0xAA55;
constexpr uint8_t  INVALID_SIZE_IN_LBA = 0x0;
constexpr uint8_t  NUM_OF_PARTITIONS   = 4;

// Protective MBR definitions, useful for identifying GPT disks
constexpr uint32_t PROTECTIVE_MBR_DISK_SIGNATURE = 0x0;
constexpr uint32_t PROTECTIVE_MBR_STARTING_LBA = 0x1;

/**
 * \internal the OS type field in the partition record can be used to determine the filesystem
 * type of the partition.
 * For more details on the OS type values, see https://en.wikipedia.org/wiki/Partition_type#List_of_partition_IDs
 * Due to the non standardized nature of the OS type field, some values might be used for multiple filesystems, 
 * and some filesystems might be represented by multiple values, thus the OSType is used only as a hint.
 * Sometimes 
 */
enum class OSType : uint8_t {
    Empty         = 0x00,
    FAT12         = 0x01,
    FAT16         = 0x04,
    FAT16B        = 0x06,
    EXFAT         = 0x07,
    FAT32CHS      = 0x0b,
    FAT32LBA      = 0x0c, //< FAT32 with LBA support
    FAT16BLBA     = 0x0e, //< FAT16B with LBA support
    UEFIPart      = 0xef, //< UEFI Partition, usually used for EFI System Partitions (ESP), unused in miosix
    ProtectiveMBR = 0xee  //< Protective MBR for GPT disks
};

struct MBRPartitionRecord {
    uint8_t bootIndicatorAndStartingCHS[4]; //< Unused boot indicator (1 byte) + starting CHS address (3 bytes)
    uint8_t osTypeAndEndingCHS[4];          //< OS type (1 byte) + (unused) ending CHS address (3 bytes)
    uint32_t startingLBA;                   // The starting LBA of the partition.
    uint32_t sizeInLBA;                     // The size of the partition in LBAs. A value of 0 indicates an unused partition entry.     
    inline OSType getOsType() 
    {
        auto osType = osTypeAndEndingCHS[0];
        return static_cast<OSType>(osType);
    }
} __attribute__((packed));

struct MBRHeader {
    uint8_t bootCode[MBR_BOOT_CODE_SIZE];
    uint8_t unused[16];
    uint32_t uniqueMBRSignature;
    uint16_t unknown;
    MBRPartitionRecord partitionRecords[NUM_OF_PARTITIONS];
    uint16_t mbrSignature;
}  __attribute__((packed));

static_assert(sizeof(MBRHeader) == 512, "GPT Header size must not exceed Logic Block Size (512)");

class MBRReader {
public:  
    static std::expected<MBRReader, bool> readMBR(miosix::intrusive_ref_ptr<miosix::Device> device);

    bool isValidMBR();
    bool isProtectiveMBR();

    uint16_t mbrSignature() {
        return header.mbrSignature;
    }

    std::array<MBRPartitionRecord, 4> getPartitions()
    {
        return std::to_array(header.partitionRecords);
    }

    ~MBRReader() {}
private:
    MBRReader() = default;
    MBRHeader header;
};

} //namespace MBR