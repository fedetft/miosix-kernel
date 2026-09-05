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
#include <array>
#include <cstdint>
#include <cstddef>
#include <expected>
#include <miosix.h>
#include <filesystem/devfs/devfs.h>
#include "../UUID/UUID.h"
#include "../MBR/mbr.h"
#include "partition_uuids.h"

namespace GPT 
{
// The primary GPT header is located at LBA 1, the backup GPT header is located at the end of the device
// and the address of that is given in the primary GPT header in the field Alternate LBA
constexpr off_t  MAIN_GPT_POSITION_LBA=1; 
constexpr const char*  GPT_SIGNATURE="EFI PART";
constexpr size_t GPT_PARTITION_NAME_SIZE=72/2; // 36 UTF-16 characters, 2 bytes each
constexpr size_t MAX_GPT_PARTITIONS=16;

enum class ReaderResult 
{
    Ok = 0,
    ErrorReadingMBR,
    ErrorInvalidMBR,
    ErrorMBRIsNotProtective,
    ErrorReadingPartitionTableEntry,
    ErrorReadingPrimaryHeader,
    ErrorInvalidPrimaryHeader,
    ErrorInvalidPrimaryHeaderCRC,
    ErrorReadingBackupHeader,
    ErrorInvalidBackupHeader,
    ErrorInvalidBackupHeaderCRC,
    ErrorExceededMaxPartitions,
    ErrorReadingPrimaryPartitions,
    ErrorReadingBackupPartitions
};

struct GPTPartitionEntry 
{
    uint8_t  partitionTypeGUID[UUID::UUID_LEN];
    uint8_t  uniquePartitionGUID[UUID::UUID_LEN];
    off_t    startingLBA;
    off_t    endingLBA;
    uint64_t attributes;
    char16_t partitionName[GPT_PARTITION_NAME_SIZE];
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
    
    /*
     * Check if the partition entry is empty
     * \return true if the partition entry is empty, false otherwise
     * An empty partition entry is defined as having a partition type GUID of
     * all zeros
     */
    bool isEmpty() const
    {
        return memcmp(partitionTypeGUID, 
            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", UUID::UUID_LEN) == 0;
    };
} __attribute__((packed));

struct GPTHeader 
{
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


class CRC32Calculator
{
public:

    // This CRC32 calculates CRC32 only for 32 bits multiples
    CRC32Calculator(uint32_t initialBuff, uint32_t nextBuff) 
    {
        buff=(static_cast<uint64_t>(initialBuff)<<32) | static_cast<uint64_t>(nextBuff);
    };


    /**
     * Computes the remainder of the polynomial division of the current buffer by 
     * the CRC32 polynomial which is 0x04C11DB7, the standard polynomial used in 
     * the IEEE 802.3 Ethernet CRC32 algorithm.
     * The method always leaves the buffer as 0xXXXXXXXX00000000, where XXXXXXXX 
     * is the remainder of the division.
     */
    void computeRemainder() 
    {
        iprintf("Computing remainder for buffer: 0x%016llX\n", buff);
        for (size_t i=0; i<32; i++) 
        {
            if (buff&0x8000000000000000) // the first bit is 1 
            {
                uint64_t part=buff>>32;
                part^=0x04C11DB7; // we compute the remainder of the first 32 bits
                buff=(part<<32)|(buff&0xFFFFFFFF); // we substitute the first 32 bits with the remainder
                buff<<=1; //then we discard the first bit
            } 
            else 
            {
                buff<<=1;
            }
            iprintf("Step[%u]: 0x%016llX\n", i, buff);
        }    
    }

    void calculateNextStep(uint32_t nextBuff) 
    {
        if (isFinal) 
        {
            return; // we are at the final step, no more calculations
        }

        computeRemainder();

        iprintf("Calculating next step with buffer: 0x%016llX and nextBuff: 0x%08X\n", buff, nextBuff);

        // computeRemainder is guaranteed to leave the first lsb 32 bits of
        // buff to 0 so we can just add the next 32 bits to the end of the buffer
        buff|=static_cast<uint64_t>(nextBuff);
        iprintf("After adding nextBuff: 0x%016llX\n", buff);
    }

    uint32_t finalizeCRC32() 
    {
        computeRemainder();

        return buff>>32; // the first 32 bits of the buffer are the CRC
    };

private: 
    bool isFinal=false;
    uint64_t buff;
};


class GPTTableReader 
{
public:
    std::expected<GPTPartitionEntry, ReaderResult> getNextPartitionEntry();
    void reset() 
    { 
        currentPartitionIndexWithinBlock=0;
        currentBlockIndex=0; 
        currentPartitionIndex=0; 
    }

private:
    GPTTableReader(miosix::intrusive_ref_ptr<miosix::Device> device, 
        unsigned long long partitionTableLBA, uint32_t partitionEntrySize, 
        uint32_t numberOfPartitionEntries)
        : device{device}, partitionTableLBA{partitionTableLBA}, partitionEntrySize{partitionEntrySize}, 
          numberOfPartitionEntries{numberOfPartitionEntries}, currentPartitionIndexWithinBlock{0}, 
          currentBlockIndex{0}, currentPartitionIndex{0}
    {}

    ReaderResult loadPartitonEntry(GPTPartitionEntry* entry);

    friend class GPTReader;
    
    miosix::intrusive_ref_ptr<miosix::Device> device;
    unsigned long long partitionTableLBA;
    const uint32_t partitionEntrySize;
    const uint32_t numberOfPartitionEntries;
    uint8_t  currentPartitionIndexWithinBlock;
    uint64_t currentBlockIndex;
    uint32_t currentPartitionIndex;
    GPTPartitionEntry buffer[4];
};

class GPTReader {
public:
    static std::expected<GPTReader, ReaderResult> readGPT(miosix::intrusive_ref_ptr<miosix::Device> device);
    ReaderResult checkGPT();

    void printGPTInfo();

    GPTTableReader getPrimaryPartitionTableReader() 
    {
        return GPTTableReader(device, primaryHeader.partitionEntryTableLBA, 
            primaryHeader.partitionEntrySize, primaryHeader.numberOfPartitionEntries);
    }

    GPTTableReader getBackupPartitionTableReader() 
    {
        return GPTTableReader(device, backupHeader.partitionEntryTableLBA, 
            backupHeader.partitionEntrySize, backupHeader.numberOfPartitionEntries);
    }

    GPTReader(GPTReader&& other);
    GPTReader& operator=(GPTReader&& other);
private:
    GPTReader(miosix::intrusive_ref_ptr<miosix::Device> device) : 
        device{device}, primaryHeader{}, backupHeader{}
    {}

    void printHeaderInfo(GPTHeader& header);
    void printTableInfo(GPTTableReader& tableReader);


    GPTReader(GPTReader& other) = delete;
    GPTReader operator=(GPTReader& other) = delete;

    miosix::intrusive_ref_ptr<miosix::Device> device;
    GPTHeader primaryHeader;
    GPTHeader backupHeader;
};

} // namespace GPT