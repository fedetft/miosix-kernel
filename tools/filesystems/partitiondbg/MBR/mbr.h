#pragma once
#include <cstdint>
#include <cstddef>
#include <miosix.h>
#include <filesystem/devfs/devfs.h>

namespace MBR {
    
constexpr size_t   MBR_BOOT_CODE_SIZE  = 424;
constexpr uint16_t MBR_SIGNATURE       = 0xAA55;
constexpr uint8_t  INVALID_SIZE_IN_LBA = 0x0;


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

constexpr std::initializer_list<std::pair<OSType, const char*>> OSTYPE_STRINGS = {
    { OSType::Empty,         "Empty Partition" },
    { OSType::FAT12,         "FAT12" },
    { OSType::FAT16,         "FAT16" },
    { OSType::FAT16B,        "FAT16B" },
    { OSType::EXFAT,         "exFAT" },
    { OSType::FAT32CHS,      "FAT32 (CHS Addressing Mode)" },
    { OSType::FAT32LBA,      "FAT32 (LBA Addressing Mode)" },
    { OSType::FAT16BLBA,     "FAT16B (LBA Addressing Mode)" },
    { OSType::UEFIPart,      "UEFI Partition" },
    { OSType::ProtectiveMBR, "Protective MBR (GPT Drive)"}
};

typedef struct MBRPartitionRecord {
    uint8_t bootIndicatorAndStartingCHS[4]; //< Unused boot indicator (1 byte) + starting CHS address (3 bytes)
    uint8_t osTypeAndEndingCHS[4];          //< OS type (1 byte) + (unused) ending CHS address (3 bytes)
    uint32_t startingLBA;                   // The starting LBA of the partition.
    uint32_t sizeInLBA;                     // The size of the partition in LBAs. A value of 0 indicates an unused partition entry.     
} __attribute__((packed)) MBRPartitionRecord;

typedef struct MBRHeader {
    uint8_t bootCode[MBR_BOOT_CODE_SIZE];
    uint8_t unused[16];
    uint32_t uniqueMBRSignature;
    uint16_t unknown;
    MBRPartitionRecord partitionRecords[4];
    uint16_t mbrSignature;
}  __attribute__((packed)) MBRHeader;

class MBRReader {
public:  
    static std::pair<bool, MBRReader> readMBR(miosix::intrusive_ref_ptr<miosix::Device> device);

    bool isValidMBR();
    void printMBRInfo();
    bool isProtectiveMBR();

    uint16_t mbrSignature() {
        return header.mbrSignature;
    }
private:
    void printOSType(uint8_t osTypeField);
    MBRReader() = default;
    MBRHeader header;
};

} //namespace MBR