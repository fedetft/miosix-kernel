/***************************************************************************
 *   Copyright (C) 2011-2024 by Terraneo Federico                          *
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

// Code started in 2011 as ResourceFs (part of mxgui), renamed romfs in 2024

#pragma once

/**
 * Filesystem header, stored @ offset 0 from the filesystem image start
 */
struct RomFsHeader
{
    char marker[6];            ///< 5 'w' characters, null terminated
    char fsName[11];           ///< "RomFs 2.00", null terminated
    char osName[7];            ///< "Miosix", null terminated
    unsigned int imageSize;    ///< Size of the entire filesystem image
    unsigned int unused;       ///< Reserved for future use, set as 0 for now
};

/**
 * Every directory starts with an entry of this type
 */
struct RomFsFirstEntry
{
    unsigned int parentInode; ///< Inode of the parent directory, 0 if root dir
};

/**
 * Regualr directory entry
 */
struct __attribute__((packed)) RomFsDirectoryEntry
{
    unsigned int inode;       ///< File/directory content start offset
    unsigned int size;        ///< File size
    unsigned short mode;      ///< Type reg/dir/symlink and permissions
    unsigned short uid, gid;  ///< File owner and group
    char name[];              ///< File name, null teminated
};

/// Alignment of all filesystem data structures. Must be a power of 2. Chosen as
/// 4 bytes for compatibility to architectures without unaligned memory accesses
const unsigned int romFsStructAlignment=4;
/// Minimum alignment of files stored in the filesystem. Must be a power of 2.
/// Chosen as 8 bytes since most elf files require 8 byte alignment, and some
/// binary data files may contain 64 bit types.
const unsigned int romFsFileAlignment=8;
/// Alignment of filesystem image start and end. Must be a power of 2 and
/// greater or equal than romFsStructAlignment and romFsFileAlignment.
/// See elf_program.cpp for the choice of 64 bytes alignment.
const unsigned int romFsImageAlignment=64;

static_assert(romFsImageAlignment>=romFsFileAlignment,"");
static_assert(romFsImageAlignment>=romFsStructAlignment,"");
static_assert(sizeof(RomFsHeader)==32,"");
static_assert(sizeof(RomFsFirstEntry)==4,"");
static_assert(sizeof(RomFsDirectoryEntry)==14,"");
