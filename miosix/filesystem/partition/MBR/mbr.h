#pragma once
#include <cstdint>
#include <expected>
#include "miosix_settings.h"
#include "filesystem/devfs/devfs.h"
#include "filesystem/partition/partition_type.h"
#include <memory>

namespace MBR {

constexpr off_t    MBR_POSITION_LBA    = 0;
constexpr size_t   MBR_BOOT_CODE_SIZE  = 424;
constexpr uint16_t MBR_SIGNATURE       = 0xAA55;
constexpr uint8_t  INVALID_SIZE_IN_LBA = 0x0;
constexpr uint8_t  NUM_OF_PARTITIONS   = 4;

// Protective MBR definitions, useful for identifying GPT disks
constexpr uint32_t PROTECTIVE_MBR_DISK_SIGNATURE = 0x0;
constexpr uint32_t PROTECTIVE_MBR_STARTING_LBA   = 0x1;

/**
 * \internal the OS type field in the partition record can be used to determine the filesystem
 * type of the partition.
 * For more details on the OS type values, see https://en.wikipedia.org/wiki/Partition_type#List_of_partition_IDs
 * Due to the non standardized nature of the OS type field, some values might be used for multiple filesystems, 
 * and some filesystems might be represented by multiple values, thus the OSType is used only as a hint. 
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
    // This is not an official ID (nor there exist an official OS Type table), 
    // but is reserved for custom OS types, we can used it for LittleFS 
    MIOSIX_LITTLEFS = 0x7f,
    UEFIPart      = 0xef, //< UEFI Partition, usually used for EFI System Partitions (ESP), unused in miosix
    ProtectiveMBR = 0xee  //< Protective MBR for GPT disks
};

struct MBRPartitionRecord {
    uint8_t bootIndicatorAndStartingCHS[4]; //< Unused boot indicator (1 byte) + starting CHS address (3 bytes)
    uint8_t osTypeAndEndingCHS[4];          //< OS type (1 byte) + (unused) ending CHS address (3 bytes)
    uint32_t startingLBA;                   // The starting LBA of the partition.
    uint32_t sizeInLBA;                     // The size of the partition in LBAs. A value of 0 indicates an unused partition entry.     
    inline OSType getOsType() const
    {
        auto osType=osTypeAndEndingCHS[0];
        return static_cast<OSType>(osType);
    }
        
    inline bool isEmpty()
    {
        return osTypeAndEndingCHS[0]==static_cast<uint8_t>(OSType::Empty);
    }

    static MBRPartitionRecord createEmpty()
    {
        MBRPartitionRecord record;
        memset(record.bootIndicatorAndStartingCHS, 0, 4);
        memset(record.osTypeAndEndingCHS, 0, 4);
        record.startingLBA=0;
        record.sizeInLBA=0;
        return record;
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

class MBRReader 
{
public:  
    static std::expected<std::unique_ptr<MBRReader>, bool> readMBR(miosix::intrusive_ref_ptr<miosix::Device> device);

    bool isValidMBR();
    void printMBRInfo();
    bool isProtectiveMBR();

    uint16_t mbrSignature() 
    {
        return header.mbrSignature;
    }

    MBRPartitionRecord getNextPartitionEntry();
    inline void reset() { currEntryIdx=0;}

    ~MBRReader() {}
    MBRReader() : header{}, currEntryIdx{} {};
private:
    MBRHeader header;
    uint8_t currEntryIdx;
};

enum class MBRFormatResult
{
    Ok,
    TooManyPartitions,
    PartitionPositionMustIncrease,
    PartitionTooBig,
    BoundariesNotAlignedToLogicBlock,
    InvalidPartitionType
};

class MBRFormatter
{
public:
    /**
     * This class is a helper class that allow to create a MBR
     * partition table and format the disk with such table.
     * To allow for boundary and partition type checking without altering the 
     * disk, thus reducing reads and writes, the action of 
     * committing the format to disk is deferred to the user by the simple
     * call to writeMBRToDisk.
     * 
     * @param device The memory device that is wanted to be formatted
     * @param uniqueSignature this parameter allows to set a custom 32 bit signature
     *                        that should uniquely identify the partition within the OS
     *                        Miosix discards this information when reading the partition
     *                        table so it might just be useful for debugging the partition
     *                        table.
     */
    MBRFormatter(miosix::intrusive_ref_ptr<miosix::Device> device, uint32_t uniqueSignature);

    /**
     * @brief This method allows to insert a partition in the MBR partition table
     * @note The changes to the partition table will not be immediately written to disk
     *       and should be committed by calling writeMBRToDisk()
     *       Faulty partitions (any time this function returns anything but MBRFormatResult::Ok)
     *       will not be committed to disk, so be sure to check the result of this function
     *       This function will keep partition close to one another and the starting position
     *       of each partition will be automatically calculated from the previous inserted one.
     * 
     * @param type The type of the partition, this will format with such partition 
     *             when committing to disk. Note that subsequent commitments to disk
     *             will overwrite the partition, too.
     * @param size The size of the partition in bytes. Size must be multiple of 512 bytes
     * @returns The result of the operation. While data is not committed to disk the 
     *          format is emulated and errors will be detected at runtime instead of
     *          when trying to commit. 
     */
    MBRFormatResult addPartition(PartitionType type, unsigned long long size);

    /**
     * @brief This method allows to insert a partition in the MBR partition table
     * @note The changes to the partition table will not be immediately written to disk
     *       and should be committed by calling writeMBRToDisk()
     *       Faulty partitions (any time this function returns anything but MBRFormatResult::Ok)
     *       will not be committed to disk, so be sure to check the result of this function
     * 
     * @param type The type of the partition, this will format with such partition 
     *             when committing to disk. Note that subsequent commitments to disk
     *             will overwrite the partition, too.
     * @param size The size of the partition in bytes. Size must be multiple of 512 bytes
     * @param startingPoint the starting point of the partition in bytes. 
     *              Must be multiple of 512 bytes
     * @returns The result of the operation. If the result is not MBRFormatResult::Ok the partition
     *          is NOT written into the partition table.
     */
    MBRFormatResult addPartition(PartitionType type, unsigned long long size, 
        unsigned long long startingPoint);

    /**
     * @brief This method will commit the partition table to disk, it will not format the partitions.
     * @returns Returns any error generated by writing the disk
     *          Returns the number of written bytes on success (should be 512)
     */
    int writeMBRToDisk();

    MBRFormatter(MBRFormatter& other)
    {
        this->device=other.device;
        this->deviceSize=other.deviceSize;
        this->header=other.header;
        this->partitionIndex=other.partitionIndex;
    }

    MBRFormatter& operator=(MBRFormatter& other)
    {
        if (this == &other) return *this;

        this->device=other.device;
        this->deviceSize=other.deviceSize;
        this->header=other.header;
        this->partitionIndex=other.partitionIndex;
        return *this;
    }
private:
miosix::intrusive_ref_ptr<miosix::Device> device;
    unsigned long long deviceSize;
    MBRHeader header;    
    unsigned char partitionIndex;
};

} //namespace MBR