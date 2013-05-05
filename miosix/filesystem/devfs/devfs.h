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
     * Open a file
     * \param file the file object will be stored here, if the call succeeds
     * \param name the name of the file to open, relative to the local
     * filesystem
     * \param flags file flags (open for reading, writing, ...)
     * \param mode file permissions
     */
    virtual int open(intrusive_ref_ptr<FileBase>& file, std::string name,
            int flags, int mode);
    
    /**
     * \return true if all files belonging to this filesystem are closed 
     */
    virtual bool areAllFilesClosed();
    
private:
    Mutex mutex;
    std::map<std::string,intrusive_ref_ptr<FileBase> > files;
};

} //namespace miosix

#endif //DEVFS_H
