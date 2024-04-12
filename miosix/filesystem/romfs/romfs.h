/***************************************************************************
 *   Copyright (C) 2024 by Terraneo Federico                               *
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

#include "filesystem/file.h"
#include "filesystem/stringpart.h"
#include "config/miosix_settings.h"

#ifdef WITH_FILESYSTEM

//Forward decl
struct RomFsDirectoryEntry;

namespace miosix {

/**
 * Assuming a firmware is made where the RomFs is appended after the kernel,
 * try to find the address of the first byte of the RomFs image
 * \return the address of the RomFs image, or nullptr in case of errors
 */
const void *getRomFsAddressAfterKernel();

/**
 * MemoryMappedRomFs allows to mount a portion of a microcontroller
 * memory-mapped flash memory as a RomFs read-only filesystem.
 */
class MemoryMappedRomFs : public FilesystemBase
{
public:
    /**
     * Constructor
     * \param baseAddress base address in the microcontroller flash memory
     * where the RomFs is stored
     */
    MemoryMappedRomFs(const void *baseAddress);
    
    /**
     * Open a file
     * \param file the file object will be stored here, if the call succeeds
     * \param name the name of the file to open, relative to the local
     * filesystem
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int open(intrusive_ref_ptr<FileBase>& file, StringPart& name,
            int flags, int mode);
    
    /**
     * Obtain information on a file, identified by a path name. Does not follow
     * symlinks
     * \param name path name, relative to the local filesystem
     * \param pstat file information is stored here
     * \return 0 on success, or a negative number on failure
     */
    virtual int lstat(StringPart& name, struct stat *pstat);

    /**
     * Change file size
     * \param name path name, relative to the local filesystem
     * \param size new file size
     * \return 0 on success, or a negative number on failure
     */
    virtual int truncate(StringPart& name, off_t size);
    
    /**
     * Remove a file or directory
     * \param name path name of file or directory to remove
     * \return 0 on success, or a negative number on failure
     */
    virtual int unlink(StringPart& name);
    
    /**
     * Rename a file or directory
     * \param oldName old file name
     * \param newName new file name
     * \return 0 on success, or a negative number on failure
     */
    virtual int rename(StringPart& oldName, StringPart& newName);
     
    /**
     * Create a directory
     * \param name directory name
     * \param mode directory permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int mkdir(StringPart& name, int mode);
    
    /**
     * Remove a directory if empty
     * \param name directory name
     * \return 0 on success, or a negative number on failure
     */
    virtual int rmdir(StringPart& name);

    /**
     * Follows a symbolic link
     * \param path path identifying a symlink, relative to the local filesystem
     * \param target the link target is returned here if the call succeeds.
     * Note that the returned path is not relative to this filesystem, and can
     * be either relative or absolute.
     * \return 0 on success, a negative number on failure
     */
    virtual int readlink(StringPart& name, std::string& target);

    /**
     * \return true if the filesystem supports symbolic links.
     * In this case, the filesystem should override readlink
     */
    virtual bool supportsSymlinks() const;

    /**
     * \return true if the filesystem failed to mount
     */
    bool mountFailed() const { return failed; }

    /**
     * \internal
     * Convert a filesystem offset to the corresponding pointer
     * \param offset filesystem offset 0=first byte of the filesystem image...
     * \return pointer to the corresponding byte in the address space
     */
    template<typename T=const char*>
    T ptr(unsigned int offset) { return reinterpret_cast<T>(base+offset); }
    
private:
    /**
     * \param name file/directory/symlink name
     * \return corresponding entry if found, or nullptr
     */
    const RomFsDirectoryEntry *findEntry(StringPart& name);

    const char * const base;
    bool failed; ///< Failed to mount
};

} //namespace miosix

#endif //WITH_FILESYSTEM
