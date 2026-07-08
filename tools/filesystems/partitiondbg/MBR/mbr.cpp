#include "mbr.h"
#include <cstdio>

using namespace miosix;
namespace MBR {
std::expected<MBRReader, bool> MBRReader::readMBR(miosix::intrusive_ref_ptr<miosix::Device> device) {
    MBRReader reader;
    auto result = device->readBlock(&reader.header, sizeof(MBRHeader), MBR_POSITION_LBA);
    if (result < 0) {
        return std::unexpected(true);
    } 
    return reader;
}

bool MBRReader::isValidMBR() {
    return header.mbrSignature == MBR_SIGNATURE;
}

void MBRReader::printOSType(uint8_t osTypeField) {
    std::map<OSType, const char*> osTypeStrings{OSTYPE_STRINGS.begin(), OSTYPE_STRINGS.end()};
    iprintf("  OS Type ID: 0x%02X\n", osTypeField);
    if (osTypeStrings.contains(static_cast<OSType>(osTypeField))) {
        iprintf("  OS Type Name: %s\n", osTypeStrings.at(static_cast<OSType>(osTypeField)));
    } else {
        iprintf("  OS Type Name: Unkown/Usupported\n");
    }
}

void MBRReader::printMBRInfo() {
    iprintf("MBR Signature: 0x%04X\n", header.mbrSignature);
    iprintf("Unique MBR Signature: 0x%08lX\n", header.uniqueMBRSignature);
    for (int i = 0; i < 4; i++) {
        const MBRPartitionRecord& record = header.partitionRecords[i];
        iprintf("Partition %d:\n", i + 1);
        printOSType(record.osTypeAndEndingCHS[0]);
        iprintf("  Starting LBA: %lu (Byte %llu)\n", record.startingLBA, static_cast<off_t>(record.startingLBA) * 512);
        iprintf("  Size in LBA: %lu (%llu Bytes)\n", record.sizeInLBA, static_cast<off_t>(record.sizeInLBA) * 512);
    }
}

bool MBRReader::isProtectiveMBR() {
    return header.partitionRecords[0].osTypeAndEndingCHS[0] == static_cast<uint8_t>(OSType::ProtectiveMBR);
}

} //namespace MBR