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
#include "gpt.h"
#include "../MBR/mbr.h"
#include <util/unicode.h>

namespace GPT 
{
std::expected<GPTReader, ReaderResult>
GPTReader::readGPT(miosix::intrusive_ref_ptr<miosix::Device> device) 
{
    auto mbrResult=MBR::MBRReader::readMBR(device);
    GPTReader reader(device);

    if (!mbrResult) 
    {
        return std::unexpected{ReaderResult::ErrorReadingMBR};
    }

    auto mbrReader=*mbrResult;

    iprintf("Check mbr validity\n");
    if (!mbrReader.isValidMBR()) 
    {
        return std::unexpected{ReaderResult::ErrorInvalidMBR};
    }

    iprintf("Check if mbr is protective\n");
    if(!mbrReader.isProtectiveMBR()) 
    {
        return std::unexpected{ReaderResult::ErrorMBRIsNotProtective};
    }
 
    iprintf("Reading first LBA block\n");
    auto result=device->readBlock(&reader.primaryHeader, sizeof(GPTHeader), 
                                    MAIN_GPT_POSITION_LBA*512);
    if (result < 0) 
    {
        return std::unexpected{ReaderResult::ErrorReadingPrimaryHeader};
    }
        
    iprintf("Reading last LBA block\n");
    result=device->readBlock(&reader.backupHeader, sizeof(GPTHeader), 
                                reader.primaryHeader.alternateLBA*512);
    if (result<0) 
    {
        return std::unexpected{ReaderResult::ErrorReadingBackupHeader};
    }

    return std::move(reader);
}


ReaderResult GPTReader::checkGPT() 
{
    // Check primary header signature
    if (std::memcmp(primaryHeader.signature, "EFI PART", 8)!=0) 
    {
        return ReaderResult::ErrorInvalidPrimaryHeader;
    }

    // Check backup header signature
    if (std::memcmp(backupHeader.signature, "EFI PART", 8)!=0) 
    {
        return ReaderResult::ErrorInvalidBackupHeader;
    }

    // Check primary and backup header consistency
    if (primaryHeader.myLBA!=MAIN_GPT_POSITION_LBA 
        || primaryHeader.alternateLBA!=backupHeader.myLBA) 
    {
        return ReaderResult::ErrorInvalidPrimaryHeader;
    }

    if (backupHeader.myLBA!=primaryHeader.alternateLBA 
        || backupHeader.alternateLBA!=MAIN_GPT_POSITION_LBA)
    {
        return ReaderResult::ErrorInvalidBackupHeader;
    }

    // Is it guaranteed that the conversoin from headerCopy to GPTHeader
    // is endiannes safe? no, also headerCRC32 is not set to 0
    uint8_t headerCopy[512];
    memcpy(headerCopy, &primaryHeader, sizeof(GPTHeader));
    reinterpret_cast<GPTHeader *>(headerCopy)->headerCRC32=0;

    uint32_t initialBuff=*reinterpret_cast<uint32_t *>(headerCopy+0);
    uint32_t nextBuff=*reinterpret_cast<uint32_t *>(headerCopy+4);
    CRC32Calculator crcCalculator{initialBuff, nextBuff};

    for (int i=2; i < (512/4)-1; i++) 
    {
        crcCalculator.calculateNextStep(nextBuff);
        nextBuff=*reinterpret_cast<uint32_t *>(headerCopy+(i*4));
    }
    crcCalculator.calculateNextStep(nextBuff);
    uint32_t crc=crcCalculator.finalizeCRC32();

    if (crc!=primaryHeader.headerCRC32) 
    {
        iprintf("Primary header CRC32 mismatch: calculated 0x%08lX, expected 0x%08lX\n", 
            crc, primaryHeader.headerCRC32);
        return ReaderResult::ErrorInvalidPrimaryHeaderCRC;
    }


    // GPTHeader backupCopy = backupHeader;
    // backupCopy.headerCRC32 = 0;
    // const auto backupCRC = CRC32::calculate(reinterpret_cast<const uint8_t*>(&backupCopy), backupHeader.headerSize);
    // if (backupCRC != backupHeader.headerCRC32) {
    //     return ReaderResult::ErrorInvalidBackupHeaderCRC;
    // }

    return ReaderResult::Ok;
}

std::expected<GPTPartitionEntry, ReaderResult> GPTTableReader::getNextPartitionEntry() 
{
    if (currentPartitionIndex>=numberOfPartitionEntries) 
    {
        return std::unexpected{ReaderResult::ErrorExceededMaxPartitions};
    }

    GPTPartitionEntry entry;
    auto result=loadPartitonEntry(&entry);
    
    if (result!=ReaderResult::Ok) 
    {
        return std::unexpected{result};
    }

    currentPartitionIndex++;
    return entry;
}

ReaderResult GPTTableReader::loadPartitonEntry(GPTPartitionEntry* entry) 
{
    if (currentPartitionIndex>=numberOfPartitionEntries)
    {
        return ReaderResult::ErrorExceededMaxPartitions;
    }

    // This will avoid reading multiple times the sector if we are within the 
    // sector boundaries. The currentPartitionIndexWithinBlockis guaranteed to
    // be incremented after the first call of readBlock if entry partition size
    // is < 512 bytes, otherwise it will always be called since the index within
    // block stays at 0.
    if (currentPartitionIndexWithinBlock==0)
    {
        auto result = device->readBlock(this->buffer, 512, 
                        (partitionTableLBA+currentBlockIndex) * 512);
        if (result<0)
            return ReaderResult::ErrorReadingPartitionTableEntry;
    }

    memcpy(entry, &this->buffer[currentPartitionIndexWithinBlock], sizeof(GPTPartitionEntry));

    if (partitionEntrySize==128) 
    {
        currentPartitionIndexWithinBlock++;
        if (currentPartitionIndexWithinBlock==4)
        {
            currentBlockIndex++;
            currentPartitionIndexWithinBlock=0;
        }
    } else if (partitionEntrySize==256) 
    {
        currentPartitionIndexWithinBlock++;
        if (currentPartitionIndexWithinBlock==2)
        {
            currentBlockIndex++;
            currentPartitionIndexWithinBlock=0;
        }
    } else {
        currentBlockIndex+=partitionEntrySize/512;
    }

    currentPartitionIndex++;
    return ReaderResult::Ok;
}

void GPTReader::printHeaderInfo(GPTHeader& header)
{
    iprintf("Signature: %.8s\n", header.signature);
    iprintf("Revision: 0x%08lX\n", header.revision);
    iprintf("Header Size: %lu\n", header.headerSize);
    iprintf("Header CRC32: %lu\n", header.headerCRC32);
    iprintf("My LBA: %llu\n", header.myLBA);
    iprintf("Alternate LBA: %llu\n", header.alternateLBA);
    iprintf("First Usable LBA: %llu\n", header.firstUsableLBA);
    iprintf("Last Usable LBA: %llu\n", header.lastUsableLBA);
    iprintf("Disk GUID: ");
    UUID::UUID::fromBigEndian(header.diskGUID).printUUID();
    iprintf("\n");
    iprintf("Partition Entry Table LBA: %llu\n", header.partitionEntryTableLBA);
    iprintf("Number of Partition Entries: %lu\n", header.numberOfPartitionEntries);
    iprintf("Partition Entry Size: %lu\n", header.partitionEntrySize);
    iprintf("Partition Entry Table CRC32: %lu\n", header.partitionEntryTableCRC32);
    iprintf("\n\n");
}

void GPTReader::printTableInfo(GPTTableReader& tableReader)
{
    for (auto entry=tableReader.getNextPartitionEntry(); 
            entry.has_value() && !entry->isEmpty(); 
            entry=tableReader.getNextPartitionEntry()) 
    {
        auto partitionEntry=*entry;
        iprintf("Partition Type GUID: ");
        const auto partitionUUID=UUID::UUID::fromBigEndian(partitionEntry.partitionTypeGUID);
        partitionUUID.printUUID();
        printf("\nPartition Type Name: ");
        auto it=std::find_if(GPT_PARTITION_IDS.begin(), GPT_PARTITION_IDS.end(), 
            [& partitionUUID](auto& pair) { return pair.first==partitionUUID; });
        if (it!=GPT_PARTITION_IDS.end())
            iprintf("%s", it->second);
        else
            iprintf("Unknown");
        iprintf("\n");

        iprintf("Unique Partition GUID: ");
        UUID::UUID::fromBigEndian(partitionEntry.uniquePartitionGUID).printUUID();
        iprintf("\n");

        iprintf("Starting LBA: %llu\n", partitionEntry.startingLBA);
        iprintf("Ending LBA: %llu\n", partitionEntry.endingLBA);
        iprintf("Attributes: %llu\n", partitionEntry.attributes);
        char partName[GPT_PARTITION_NAME_SIZE*3+1]; // UTF-16 to UTF-8 conversion may take up to 3 bytes per character
        size_t bufSize=GPT_PARTITION_NAME_SIZE*3; // Initialize buffer size for UTF-8 string
        size_t bufIdx=0;
        for (size_t i=0; i<GPT_PARTITION_NAME_SIZE; i++) 
        {
            auto result=miosix::Unicode::putUtf8(&partName[bufIdx], 
                partitionEntry.partitionName[i], bufSize);
            if (result.first!=miosix::Unicode::error::OK) 
            {
                iprintf("Error converting partition name to UTF-8\n");
                break;
            }
            bufSize-=result.second;
            bufIdx+=result.second;
        }
        partName[bufSize]=0; // Null-terminate the UTF-8 string
        iprintf("Partition Name: %s\n", partName);
        iprintf("\n-----------------------------\n\n");
    }
}

void GPTReader::printGPTInfo()
{
    auto gptValid=checkGPT() == ReaderResult::Ok;
    iprintf("Is GPT Valid? %s\n", gptValid ? "Yes" : "No.\nExiting");
    if (!gptValid) return;

    iprintf("Available partition types (Size: %d):\n", GPT::GPT_PARTITION_IDS.size());
    for (const auto& [uuid, name] : GPT::GPT_PARTITION_IDS) 
    {
        iprintf("Partition Type GUID: ");
        uuid.printUUID();
        iprintf(" - %s\n", name);
    }

    iprintf("\n\n");

    iprintf("=============================\n");
    iprintf("=  Primary Partition Header =\n");
    iprintf("=============================\n");
    printHeaderInfo(primaryHeader);

    iprintf("=============================\n");
    iprintf("=  Backup Partition Header  =\n");
    iprintf("=============================\n"); 
    printHeaderInfo(backupHeader);

    iprintf("=============================\n");
    iprintf("=  Primary Partition Table  =\n");
    iprintf("=============================\n");
    auto primaryTableReader=getPrimaryPartitionTableReader();
    printTableInfo(primaryTableReader);
    iprintf("\n\n");

    iprintf("=============================\n");
    iprintf("=  Backup Partition Table   =\n");
    iprintf("=============================\n"); 
    auto backupTableReader=getBackupPartitionTableReader();
    printTableInfo(backupTableReader);
    iprintf("\n\n");
}

GPTReader::GPTReader(GPTReader&& other) 
{
    // Move primary header
    memcpy(&this->primaryHeader, &other.primaryHeader, sizeof(GPTHeader));
    memset(&other.primaryHeader, 0, sizeof(GPTHeader));

    // Move backup header
    memcpy(&this->backupHeader, &other.backupHeader, sizeof(GPTHeader));
    memset(&other.backupHeader, 0, sizeof(GPTHeader));

    this->device=std::move(other.device);
}

GPTReader& GPTReader::operator=(GPTReader&& other) 
{
    if (this!=&other) 
    {
        // Move primary header
        memcpy(&this->primaryHeader, &other.primaryHeader, sizeof(GPTHeader));
        memset(&other.primaryHeader, 0, sizeof(GPTHeader));

        // Move backup header
        memcpy(&this->backupHeader, &other.backupHeader, sizeof(GPTHeader));
        memset(&other.backupHeader, 0, sizeof(GPTHeader));

        this->device=std::move(other.device);
    }
    return *this;
}

} // namespace GPT