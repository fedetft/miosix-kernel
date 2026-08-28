#include "gpt.h"
#include "../MBR/mbr.h"

namespace GPT {
std::expected<GPTReader, ReaderResult> GPTReader::readGPT(miosix::intrusive_ref_ptr<miosix::Device> device) {
    auto mbrResult = MBR::MBRReader::readMBR(device);
    GPTReader reader(device);

    if (!mbrResult) {
        return std::unexpected{ReaderResult::ErrorReadingMBR};
    }

    auto mbrReader = *mbrResult;

    iprintf("Check mbr validity\n");
    if (!mbrReader.isValidMBR()) {
        return std::unexpected{ReaderResult::ErrorInvalidMBR};
    }

    iprintf("Check if mbr is protective\n");
    if(!mbrReader.isProtectiveMBR()) {
        return std::unexpected{ReaderResult::ErrorMBRIsNotProtective};
    }
 
    iprintf("Reading first LBA block\n");
    auto result = device->readBlock(&reader.primaryHeader, sizeof(GPTHeader), MAIN_GPT_POSITION_LBA * 512);
    if (result < 0) {
        return std::unexpected{ReaderResult::ErrorReadingPrimaryHeader};
    }
        
    iprintf("Reading last LBA block\n");
    result = device->readBlock(&reader.backupHeader, sizeof(GPTHeader), reader.primaryHeader.alternateLBA * 512);
    if (result < 0) {
        return std::unexpected{ReaderResult::ErrorReadingBackupHeader};
    }

    return std::move(reader);
}


ReaderResult GPTReader::checkGPT() {
    // Check primary header signature
    if (std::memcmp(primaryHeader.signature, "EFI PART", 8) != 0) {
        return ReaderResult::ErrorInvalidPrimaryHeader;
    }

    // Check backup header signature
    if (std::memcmp(backupHeader.signature, "EFI PART", 8) != 0) {
        return ReaderResult::ErrorInvalidBackupHeader;
    }

    // Check primary and backup header consistency
    if (primaryHeader.myLBA != MAIN_GPT_POSITION_LBA || primaryHeader.alternateLBA != backupHeader.myLBA) {
        return ReaderResult::ErrorInvalidPrimaryHeader;
    }

    if (backupHeader.myLBA != primaryHeader.alternateLBA || backupHeader.alternateLBA != MAIN_GPT_POSITION_LBA) {
        return ReaderResult::ErrorInvalidBackupHeader;
    }

    // GPTHeader primaryCopy = primaryHeader;
    // primaryCopy.headerCRC32 = 0;
    // const auto primaryCRC = CRC32::calculate(reinterpret_cast<const uint8_t*>(&primaryCopy), primaryHeader.headerSize);
    // if (primaryCRC != primaryHeader.headerCRC32) {
    //     return ReaderResult::ErrorInvalidPrimaryHeaderCRC;
    // }


    // GPTHeader backupCopy = backupHeader;
    // backupCopy.headerCRC32 = 0;
    // const auto backupCRC = CRC32::calculate(reinterpret_cast<const uint8_t*>(&backupCopy), backupHeader.headerSize);
    // if (backupCRC != backupHeader.headerCRC32) {
    //     return ReaderResult::ErrorInvalidBackupHeaderCRC;
    // }

    return ReaderResult::Ok;
}

std::expected<GPTPartitionEntry, ReaderResult> GPTTableReader::getNextPartitionEntry() {
    if (currentPartitionIndex >= numberOfPartitionEntries) {
        return std::unexpected(ReaderResult::ErrorExceededMaxPartitions);
    }

    GPTPartitionEntry entry;
    auto result = loadPartitonEntry(&entry);
    
    if (result != ReaderResult::Ok) {
        return std::unexpected(result);
    }

    currentPartitionIndex++;
    return entry;
}

ReaderResult GPTTableReader::loadPartitonEntry(GPTPartitionEntry* entry) {
    if (currentPartitionIndex >= numberOfPartitionEntries) {
        return ReaderResult::ErrorExceededMaxPartitions;
    }

    GPTPartitionEntry buff[4];

    auto result = device->readBlock(buff, 512, (partitionTableLBA + currentBlockIndex) * 512);
    if (result < 0) {
        return ReaderResult::ErrorReadingPartitionTableEntry;
    }

    if (partitionEntrySize == 128) {
        memcpy(entry, &buff[currentPartitionIndex % 4], sizeof(GPTPartitionEntry));
    } else if (partitionEntrySize == 256) {
        memcpy(entry, &buff[currentPartitionIndex % 2], sizeof(GPTPartitionEntry));
    } else {
        memcpy(entry, &buff[0], sizeof(GPTPartitionEntry));
    }

    if (partitionEntrySize == 128) {
        currentPartitionIndex++;
        if (currentPartitionIndex % 4 == 0) {
            currentBlockIndex++;
        }
    } else if (partitionEntrySize == 256) {
        currentPartitionIndex++;
        if (currentPartitionIndex % 2 == 0) {
            currentBlockIndex++;
        }
    } else {
        currentPartitionIndex++;
        currentBlockIndex += partitionEntrySize / 512;
    }

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
    UUID::UUID(header.diskGUID).printUUID();
    iprintf("\n");
    iprintf("Partition Entry Table LBA: %llu\n", header.partitionEntryTableLBA);
    iprintf("Number of Partition Entries: %lu\n", header.numberOfPartitionEntries);
    iprintf("Partition Entry Size: %lu\n", header.partitionEntrySize);
    iprintf("Partition Entry Table CRC32: %lu\n", header.partitionEntryTableCRC32);
}

void GPTReader::printTableInfo(GPTTableReader& tableReader)
{
    for (auto entry = tableReader.getNextPartitionEntry(); 
            entry.has_value(); 
            entry = tableReader.getNextPartitionEntry()) 
    {
        auto partitionEntry = *entry;
        iprintf("Partition Type GUID: ");
        UUID::UUID(partitionEntry.partitionTypeGUID).printUUID();
        iprintf("\n");

        iprintf("Unique Partition GUID: ");
        UUID::UUID(partitionEntry.uniquePartitionGUID).printUUID();
        iprintf("\n");

        iprintf("Starting LBA: %llu\n", partitionEntry.startingLBA);
        iprintf("Ending LBA: %llu\n", partitionEntry.endingLBA);
        iprintf("Attributes: %llu\n", partitionEntry.attributes);
        iprintf("Partition Name: %s\n", partitionEntry.partitionName);
        iprintf("-----------------------------\n");
    }
}

void GPTReader::printGPTInfo() {
    auto gptValid = checkGPT() == ReaderResult::Ok;
    iprintf("Is GPT Valid? %s\n", gptValid ? "Yes" : "No.\nExiting");
    if (!gptValid) return;
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
    auto primaryTableReader = getPrimaryPartitionTableReader();
    printTableInfo(primaryTableReader);
    iprintf("\n\n");

    iprintf("=============================\n");
    iprintf("=  Backup Partition Table   =\n");
    iprintf("=============================\n"); 
    auto backupTableReader = getBackupPartitionTableReader();
    printTableInfo(backupTableReader);
    iprintf("\n\n");
}

GPTReader::GPTReader(GPTReader&& other) {
    // Move primary header
    memcpy(&this->primaryHeader, &other.primaryHeader, sizeof(GPTHeader));
    memset(&other.primaryHeader, 0, sizeof(GPTHeader));

    // Move backup header
    memcpy(&this->backupHeader, &other.backupHeader, sizeof(GPTHeader));
    memset(&other.backupHeader, 0, sizeof(GPTHeader));

    this->device = std::move(other.device);
}

GPTReader& GPTReader::operator=(GPTReader&& other) {
    if (this != &other) {
        // Move primary header
        memcpy(&this->primaryHeader, &other.primaryHeader, sizeof(GPTHeader));
        memset(&other.primaryHeader, 0, sizeof(GPTHeader));

        // Move backup header
        memcpy(&this->backupHeader, &other.backupHeader, sizeof(GPTHeader));
        memset(&other.backupHeader, 0, sizeof(GPTHeader));

        this->device = std::move(other.device);
    }
    return *this;
}

} // namespace GPT