#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <expected>
#include <miosix.h>
#include <filesystem/devfs/devfs.h>
#include "../UUID/UUID.h"
#include "../MBR/mbr.h"

namespace GPT {

// The primary GPT header is located at LBA 1, the backup GPT header is located at the end of the device
// and the address of that is given in the primary GPT header in the field Alternate LBA
constexpr off_t  MAIN_GPT_POSITION_LBA = 1; 
constexpr const char*  GPT_SIGNATURE = "EFI PART";
constexpr size_t GPT_PARTITION_NAME_SIZE = 72;
constexpr size_t MAX_GPT_PARTITIONS = 16;

#define MAKE_UUID_NAME_PAIR(uuid, name) {DEF_UUID(uuid), name}
constexpr std::initializer_list<std::pair<UUID::UUID, 
                const char*>> GPT_PARTITION_IDS = {
                    MAKE_UUID_NAME_PAIR("00000000-0000-0000-0000-000000000000", "Empty"),
                    MAKE_UUID_NAME_PAIR("024DEE41-33E7-11D3-9D69-0008C781F39F ", "MBR Partition Scheme"),
                    MAKE_UUID_NAME_PAIR("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft Basic Data Partition"),
                };

enum class ReaderResult {
    Ok = 0,
    ErrorReadingMBR,
    ErrorInvalidMBR,
    ErrorMBRIsNotProtective,
    ErrorReadingPrimaryHeader,
    ErrorReadingBackupHeader,
    ErrorExceededMaxPartitions,
    ErrorReadingPrimaryPartitions,
    ErrorReadingBackupPartitions
};

struct GPTPartitionEntry {
    uint8_t  partitionTypeGUID[UUID::UUID_LEN];
    uint8_t  uniquePartitionGUID[UUID::UUID_LEN];
    off_t    startingLBA;
    off_t    endingLBA;
    uint64_t attributes;
    char     partitionName[GPT_PARTITION_NAME_SIZE];
    /* 
     * The reserved bytes depends on GPTHeader::partitionEntrySize 
     * since it is reserved for UEFI software only we can safely ignore it
     * and since it is always located at the end of the partition entry
     * we can safely read a full logic block in a buffer then memcpy only the
     * sizeof(GPTPartitionEntry) bytes and for the next entry skip 
     * GPTHeader::partitionEntrySize bytes and memcpy again and so on until
     * we read all the partition entries.
     */
    // uint8_t* reserved;
} __attribute__((packed));

struct GPTHeader {
    char signature[8];
    uint32_t revision;
    uint32_t headerSize;
    uint32_t headerCRC32;
    uint32_t reserved1;
    off_t    myLBA;
    off_t    alternateLBA;
    off_t    firstUsableLBA;
    off_t    lastUsableLBA;
    uint8_t  diskGUID[UUID::UUID_LEN];
    off_t    partitionEntryTableLBA;
    uint32_t numberOfPartitionEntries;
    uint32_t partitionEntrySize;
    uint32_t partitionEntryTableCRC32;
    uint8_t  reserved2[420];
} __attribute__((packed));

static_assert(sizeof(GPTHeader) == 512, "GPT Header size must equal the Logic Block Size (512)");

class GPTReader {
public:
    static std::expected<GPTReader, ReaderResult> readGPT(miosix::intrusive_ref_ptr<miosix::Device> device);
    bool isValidGPT() {return true;};
    void printGPTInfo() {};
    
    auto partitionsBegin() const {
        return primaryPartitions.cbegin();
    }
    
    auto partitionsEnd() const {
        return primaryPartitions.cend();
    }

    auto backupPartitionsBegin() const {
        return backupPartitions.cbegin();
    }

    auto backupPartitionsEnd() const {
        return backupPartitions.cend();
    }

    GPTReader(GPTReader&& other); //< we want this, but private

private:
    inline ReaderResult getPartitionsReadingError(GPTHeader& header);
    ReaderResult load128BitSizeEntries(GPTHeader& header, 
        std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS>& partitions, 
        miosix::intrusive_ref_ptr<miosix::Device> device);
    ReaderResult load256BitSizeEntries(GPTHeader& header, 
        std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS>& partitions, 
        miosix::intrusive_ref_ptr<miosix::Device> device);
    ReaderResult loadGenericSizeEntries(GPTHeader& header, 
        std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS>& partitions, 
        miosix::intrusive_ref_ptr<miosix::Device> device);
    ReaderResult loadPartitionTable(GPTHeader& header, 
        std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS>& partitions, 
        miosix::intrusive_ref_ptr<miosix::Device> device);
    ReaderResult readPartitionTables(miosix::intrusive_ref_ptr<miosix::Device> device);

    GPTReader():primaryHeader{}, backupHeader{}, primaryPartitions{}, backupPartitions{} {
        iprintf("Initializing GPTReader");
    };

    GPTReader(GPTReader& other) = delete;
    GPTReader operator=(GPTReader& other) = delete;
    GPTReader& operator=(GPTReader&& other) = delete;

    GPTHeader primaryHeader;
    GPTHeader backupHeader;

    std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS> primaryPartitions;
    std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS> backupPartitions;
};

} // namespace GPT