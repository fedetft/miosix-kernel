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
#include "mbr.h"
#include <cstdio>

using namespace miosix;
namespace MBR 
{
std::expected<MBRReader, bool> MBRReader::readMBR(miosix::intrusive_ref_ptr<miosix::Device> device)
{
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

void MBRReader::printOSType(uint8_t osTypeField) 
{
    std::map<OSType, const char*> osTypeStrings{OSTYPE_STRINGS.begin(), OSTYPE_STRINGS.end()};
    iprintf("  OS Type ID: 0x%02X\n", osTypeField);
    if (osTypeStrings.contains(static_cast<OSType>(osTypeField))) 
    {
        iprintf("  OS Type Name: %s\n", osTypeStrings.at(static_cast<OSType>(osTypeField)));
    } else {
        iprintf("  OS Type Name: Unkown/Usupported\n");
    }
}

void MBRReader::printMBRInfo() 
{
    iprintf("MBR Signature: 0x%04X\n", header.mbrSignature);
    iprintf("Unique MBR Signature: 0x%08lX\n", header.uniqueMBRSignature);
    for (int i=0; i<MBR_NUM_OF_PARTS; i++) 
    {
        const MBRPartitionRecord& record = header.partitionRecords[i];
        iprintf("Partition %d:\n", i+1);
        printOSType(record.osTypeAndEndingCHS[0]);
        iprintf("  Starting LBA: %lu (Byte %llu)\n", 
            record.startingLBA, static_cast<off_t>(record.startingLBA)*512);
        iprintf("  Size in LBA: %lu (%llu Bytes)\n", 
            record.sizeInLBA, static_cast<off_t>(record.sizeInLBA)*512);
    }
}

bool MBRReader::isProtectiveMBR() 
{
    return header.partitionRecords[0].osTypeAndEndingCHS[0]==static_cast<uint8_t>(OSType::ProtectiveMBR);
}

MBRPartitionRecord MBRReader::getNextPartitionEntry()
{
    if (currEntryIdx>=MBR_NUM_OF_PARTS)
        return MBRPartitionRecord::createEmpty();

    return header.partitionRecords[currEntryIdx];
    currEntryIdx++;
}

} //namespace MBR