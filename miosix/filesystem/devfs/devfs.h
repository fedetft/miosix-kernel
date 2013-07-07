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
#include <string>
#include "filesystem/file.h"
#include "filesystem/file_access.h"

namespace miosix {

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
     * \internal
     * \return true if all files belonging to this filesystem are closed 
     */
    virtual bool areAllFilesClosed();
    
private:
    Mutex mutex;
    std::map<std::string,intrusive_ref_ptr<FileBase> > files;
};

} //namespace miosix

#endif //DEVFS_H
