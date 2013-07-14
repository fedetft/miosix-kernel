/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

#ifndef DEVFS_H
#define	DEVFS_H

#include <map>
#include "filesystem/file.h"
#include "filesystem/file_access.h"

namespace miosix {

/**
 * DevFs is a special filesystem meant to access devices as they were files.
 * For this reason, it is a little different from other filesystems. Normal
 * filesystems create FileBase objects ondemand, to answer an open() call. Such
 * files have a parent pointer that points to the filesystem. On the contrary,
 * DevFs is a collection of device files. Each device file is a different
 * subclass of FileBase that overrides some of its member functions to access
 * the handled device. These FileBase subclasses do not have a parent pointer
 * into DevFs, instead, DevFs holds a map of device names and pointers into
 * these derived FileBase objects, which always exist, even if they are not
 * opened anywere. Opening the same file name multiple times returns the same
 * instance of the file object.
 * 
 * The reason why device files can't have the parent pointer that points into
 * DevFs is that DevFs already has a pointer into those files, and since they're
 * both refcounted pointers that would be a cycle of refcounted pointer that
 * would cause problem if DevFs is ever umounted.
 * 
 * Also a note on the behaviour when umounting DevFs: since the files don't
 * have a parent pointer into DevFs umounting DevFs will immediately delete
 * the filesystem, while the individual files won't be deleted until the
 * processes that have them opened close them. This isn't an unreasonable
 * behaviour, and also it must be considered that umounting DevFs shouldn't
 * happen in most practical cases.
 */
class DevFs : public FilesystemBase
{
public:
    /**
     * Constructor
     */
    DevFs();
    
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
     * Create a directory
     * \param name directory name
     * \param mode directory permissions
     * \return 0 on success, or a negative number on failure
     */
    virtual int mkdir(StringPart& name, int mode);
    
    /**
     * \internal
     * \return true if all files belonging to this filesystem are closed 
     */
    virtual bool areAllFilesClosed();
    
private:
    FastMutex mutex;
    std::map<StringPart,intrusive_ref_ptr<FileBase> > files;
};

} //namespace miosix

#endif //DEVFS_H
