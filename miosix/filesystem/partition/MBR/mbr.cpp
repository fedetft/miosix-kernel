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

bool MBRReader::isProtectiveMBR() {
    return header.partitionRecords[0].osTypeAndEndingCHS[0] == static_cast<uint8_t>(OSType::ProtectiveMBR);
}

} //namespace MBR