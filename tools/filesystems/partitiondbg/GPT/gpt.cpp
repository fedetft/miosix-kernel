#include "gpt.h"
#include "../MBR/mbr.h"

namespace GPT {
std::expected<GPTReader, ReaderResult> GPTReader::readGPT(miosix::intrusive_ref_ptr<miosix::Device> device) {
    auto mbrResult = MBR::MBRReader::readMBR(device);
    GPTReader reader;

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

    if (reader.primaryHeader.numberOfPartitionEntries < MAX_GPT_PARTITIONS) {
        iprintf("Too little partitions %ld of size %ld\n", 
            reader.primaryHeader.numberOfPartitionEntries, 
            reader.primaryHeader.partitionEntrySize);
        return std::unexpected{ReaderResult::ErrorExceededMaxPartitions};
    }

    iprintf("Reading Partition Tables\n");
    auto readerResult = reader.readPartitionTables(device);

    if (readerResult != ReaderResult::Ok) {
        return std::unexpected{readerResult};
    }

    return std::move(reader);
}

inline ReaderResult GPTReader::getPartitionsReadingError(GPTHeader &header)
{
    return header.myLBA == MAIN_GPT_POSITION_LBA ? 
        ReaderResult::ErrorReadingPrimaryPartitions : 
        ReaderResult::ErrorReadingBackupPartitions;
}

ReaderResult GPTReader::load128BitSizeEntries(GPTHeader &header, 
    std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS> &partitions, 
    miosix::intrusive_ref_ptr<miosix::Device> device)
{
    GPTPartitionEntry buff[4];
    auto numPartitions = std::min(size_t{header.numberOfPartitionEntries}, MAX_GPT_PARTITIONS);
    
    size_t result = 0;
    
    size_t idx;
    off_t blockReadIdx;

    for (idx = 0, blockReadIdx = 0; numPartitions >= 4; idx += 4, numPartitions -= 4, blockReadIdx++) {
        result = device->readBlock(buff, 512, (header.partitionEntryTableLBA + blockReadIdx) * 512);
        
        if (result < 0) {
            return getPartitionsReadingError(header);
        }

        memcpy(&partitions.at(idx), &buff[0], sizeof(GPTPartitionEntry));
        // this line adds ~290kb in size at the final binary??? is std::format bloated too? yes it is due to the unicode stuff...
        // printf("Partition type GUID: %s\n", std::format("{}", UUID::UUID(partitions.at(idx).partitionTypeGUID)).c_str());
        memcpy(&partitions.at(idx + 1), &buff[1], sizeof(GPTPartitionEntry));
        memcpy(&partitions.at(idx + 2), &buff[2], sizeof(GPTPartitionEntry));
        memcpy(&partitions.at(idx + 3), &buff[3], sizeof(GPTPartitionEntry));
    }

    if (numPartitions > 0) {
        result = device->readBlock(buff, 512, (header.alternateLBA + blockReadIdx) * 512);

        if (result < 0) {
            return getPartitionsReadingError(header);
        }

        for (;numPartitions > 0; idx++, numPartitions--) {
            memcpy(&partitions.at(idx), &buff[idx], sizeof(GPTPartitionEntry));
        }   
    }

    return ReaderResult::Ok;
}

ReaderResult GPTReader::load256BitSizeEntries(GPTHeader &header, 
        std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS> &partitions, 
        miosix::intrusive_ref_ptr<miosix::Device> device)
{
    GPTPartitionEntry buff[4];
    auto numPartitions = std::min(size_t{header.numberOfPartitionEntries}, MAX_GPT_PARTITIONS);

    size_t result = 0;

    size_t idx;
    off_t blockReadIdx;

    for (idx = 0, blockReadIdx = 0; numPartitions >= 2; idx += 2, numPartitions -= 2, blockReadIdx++) {
        result = device->readBlock(buff, 512, (header.alternateLBA + blockReadIdx) * 512);
        
        if (result < 0) {
            return getPartitionsReadingError(header);
        }

        memcpy(&partitions.at(idx), &buff[0], sizeof(GPTPartitionEntry));
        memcpy(&partitions.at(idx + 1), &buff[2], sizeof(GPTPartitionEntry));
    }

    if (numPartitions > 0) {
        result = device->readBlock(buff, 512, (header.alternateLBA + blockReadIdx) * 512);

        if (result < 0) {
            return getPartitionsReadingError(header);
        }

        for (;numPartitions > 0; idx++, numPartitions--) {
            memcpy(&partitions.at(idx), &buff[idx * 2], sizeof(GPTPartitionEntry));
        }   
    }

    return ReaderResult::Ok;
}

ReaderResult GPTReader::loadGenericSizeEntries(GPTHeader &header, 
    std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS> &partitions, 
    miosix::intrusive_ref_ptr<miosix::Device> device)
{
    GPTPartitionEntry buff[4];
    auto numPartitions = std::min(size_t{header.numberOfPartitionEntries}, MAX_GPT_PARTITIONS);
    auto entrySize = header.partitionEntrySize / 512;

    size_t result = 0;

    size_t idx;
    off_t blockReadIdx;

    for (idx = 0, blockReadIdx = 0; numPartitions >= 1; idx += 1, numPartitions -= 1, blockReadIdx += entrySize) {
        result = device->readBlock(buff, 512, (header.alternateLBA + blockReadIdx) * 512);
        
        if (result < 0) {
            return getPartitionsReadingError(header);
        }

        memcpy(&partitions.at(idx), &buff[0], sizeof(GPTPartitionEntry));
    }
    
    return ReaderResult::Ok;
}

ReaderResult GPTReader::loadPartitionTable(GPTHeader &header, 
    std::array<GPTPartitionEntry, MAX_GPT_PARTITIONS> &partitions, 
    miosix::intrusive_ref_ptr<miosix::Device> device)
{
    // If the partition entries are of standard size we can load them fast, otherwise we need a slower approach
    // This likely happens every time.
    // Also we are "lucky" since the specification allows only for 128, 256, 512, 1024, 2048, ecc. (128 x 2^n, where n = {0, 1, 2, 3, ...})
    // so we don't have strange boundaries in the block offset. In fact for the case where partitionEntrySize is >= 512 we have only one 
    // partition entry per logic block in the first block and we can just skip paritionEntrySize/512 blocks for the next entry, without
    // the need to read the remaining blocks since that data is reserved for UEFI software only.
    // The only concern of that area of data, if the size is not 128, is that it is never checked and some malicious attacker might exploit it 
    // to hide stuff

    if (header.partitionEntrySize == sizeof(GPTPartitionEntry)) [[likely]] {
        return load128BitSizeEntries(header, partitions, device);
    } else if (header.partitionEntrySize == 2 * sizeof(GPTPartitionEntry)) {
        return load256BitSizeEntries(header, partitions, device);
    } else {
        return loadGenericSizeEntries(header, partitions, device);
    }
}

ReaderResult GPTReader::readPartitionTables(miosix::intrusive_ref_ptr<miosix::Device> device)
{
    auto result = loadPartitionTable(primaryHeader, primaryPartitions, device);
 
    if (result != ReaderResult::Ok) {
        return result;
    }

    return loadPartitionTable(backupHeader, backupPartitions, device);
}

GPTReader::GPTReader(GPTReader&& other) {
        // Move primary header
        memcpy(&this->primaryHeader, &other.primaryHeader, sizeof(GPTHeader));
        memset(&other.primaryHeader, 0, sizeof(GPTHeader));

        // Move backup header
        memcpy(&this->backupHeader, &other.backupHeader, sizeof(GPTHeader));
        memset(&other.backupHeader, 0, sizeof(GPTHeader));

        for (size_t idx = 0; idx < MAX_GPT_PARTITIONS; idx++) {
            // Move primary partition entries
            memcpy(&this->primaryPartitions[idx], &other.primaryPartitions[idx], sizeof(GPTPartitionEntry));
            memset(&other.primaryPartitions[idx], 0, sizeof(GPTPartitionEntry));

            // Move backup partition entries
            memcpy(&this->backupPartitions[idx], &other.backupPartitions[idx], sizeof(GPTPartitionEntry));
            memset(&other.backupPartitions[idx], 0, sizeof(GPTPartitionEntry));
        }
    }

} // namespace GPT