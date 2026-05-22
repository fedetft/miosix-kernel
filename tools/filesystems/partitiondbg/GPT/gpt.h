#pragma once
#include <cstdint>
#include <cstddef>
#include <miosix.h>
#include <filesystem/devfs/devfs.h>

namespace GPT {

// The primary GPT header is located at LBA 1, the backup GPT header is located at the end of the device
// and the address of that is given in the primary GPT header in the field Alternate LBA
constexpr off_t  MAIN_GPT_POSITION_LBA = 1; 
constexpr char*  GPT_SIGNATURE = "EFI PART";
constexpr size_t GPT_PARTITION_NAME_SIZE = 72;

typedef struct GPTPartitionEntry {
    uint32_t partitionTypeGUID[4];
    uint32_t uniquePartitionGUID[4];
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
} __attribute__((packed)) GPTPartitionEntry;

typedef struct GPTHeader {
    char signature[8];
    uint32_t revision;
    uint32_t headerSize;
    uint32_t headerCRC32;
    uint32_t reserved1;
    off_t    myLBA;
    off_t    alternateLBA;
    off_t    firstUsableLBA;
    off_t    lastUsableLBA;
    uint32_t diskGUID[4]; // TODO: this has to be rewritten to support GUID, add GUID SUPPORT IN MIOSIX
    off_t    partitionEntryTableLBA;
    uint32_t numberOfPartitionEntries;
    uint32_t partitionEntrySize;
    uint32_t partitionEntryTableCRC32;
    uint8_t  reserved2[420];
} __attribute__((packed)) GPTHeader;

static_assert(sizeof(GPTHeader) == 512, "GPT Header size must equal the Logic Block Size (512)");

class GPTReader {};

} // namespace GPT