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
#include <cstdint>
#include <expected>
#include <miosix.h>
#include <filesystem/devfs/devfs.h>

namespace MBR 
{

constexpr off_t    MBR_POSITION_LBA    = 0;
constexpr size_t   MBR_BOOT_CODE_SIZE  = 424;
constexpr uint16_t MBR_SIGNATURE       = 0xAA55;
constexpr uint8_t  INVALID_SIZE_IN_LBA = 0x0;
constexpr size_t   MBR_NUM_OF_PARTS    = 4;


// Protective MBR definitions, useful for identifying GPT disks
constexpr uint32_t PROTECTIVE_MBR_DISK_SIGNATURE=0x0;
constexpr uint32_t PROTECTIVE_MBR_STARTING_LBA=0x1;

/**
 * \internal the OS type field in the partition record can be used to determine the filesystem
 * type of the partition.
 * For more details on the OS type values, see https://en.wikipedia.org/wiki/Partition_type#List_of_partition_IDs
 * Due to the non standardized nature of the OS type field, some values might be used for multiple filesystems, 
 * and some filesystems might be represented by multiple values, thus the OSType is used only as a hint.
 * Sometimes 
 */
enum class OSType : uint8_t 
{
    Empty         = 0x00,
    FAT12         = 0x01,
    FAT16         = 0x04,
    FAT16B        = 0x06,
    EXFAT         = 0x07,
    FAT32CHS      = 0x0b,
    FAT32LBA      = 0x0c, //< FAT32 with LBA support
    FAT16BLBA     = 0x0e, //< FAT16B with LBA support
    UEFIPart      = 0xef, //< UEFI Partition, usually used for EFI System Partitions (ESP), unused in miosix
    ProtectiveMBR = 0xee  //< Protective MBR for GPT disks
};

constexpr std::initializer_list<std::pair<OSType, const char*>> OSTYPE_STRINGS = {
    { OSType::Empty,         "Empty Partition" },
    { OSType::FAT12,         "FAT12" },
    { OSType::FAT16,         "FAT16" },
    { OSType::FAT16B,        "FAT16B" },
    { OSType::EXFAT,         "exFAT" },
    { OSType::FAT32CHS,      "FAT32 (CHS Addressing Mode)" },
    { OSType::FAT32LBA,      "FAT32 (LBA Addressing Mode)" },
    { OSType::FAT16BLBA,     "FAT16B (LBA Addressing Mode)" },
    { OSType::UEFIPart,      "UEFI Partition" },
    { OSType::ProtectiveMBR, "Protective MBR (GPT Drive)"}
};

struct MBRPartitionRecord 
{
    uint8_t bootIndicatorAndStartingCHS[4]; //< Unused boot indicator (1 byte) + starting CHS address (3 bytes)
    uint8_t osTypeAndEndingCHS[4];          //< OS type (1 byte) + (unused) ending CHS address (3 bytes)
    uint32_t startingLBA;                   // The starting LBA of the partition.
    uint32_t sizeInLBA;                     // The size of the partition in LBAs. A value of 0 indicates an unused partition entry.     
    
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
    MBRPartitionRecord partitionRecords[MBR_NUM_OF_PARTS];
    uint16_t mbrSignature;
}  __attribute__((packed));

static_assert(sizeof(MBRHeader) == 512, "GPT Header size must not exceed Logic Block Size (512)");

class MBRReader 
{
public:  
    static std::expected<MBRReader, bool> readMBR(miosix::intrusive_ref_ptr<miosix::Device> device);

    bool isValidMBR();
    void printMBRInfo();
    bool isProtectiveMBR();

    uint16_t mbrSignature() 
    {
        return header.mbrSignature;
    }

    MBRPartitionRecord getNextPartitionEntry();
    inline void reset() { currEntryIdx = 0;}

    ~MBRReader() {}
private:
    void printOSType(uint8_t osTypeField);
    MBRReader() : header{}, currEntryIdx{} {};
    MBRHeader header;
    uint8_t currEntryIdx;
};

} //namespace MBR