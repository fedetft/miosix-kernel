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

#include "file.h"
#include <cstdio>
#include <string>
#include "file_access.h"

using namespace std;

namespace miosix {

//
// class File
//

int FileBase::isatty() const
{
    return 0;
}

int FileBase::sync()
{
    return 0;
}

FileBase::~FileBase()
{
    if(parent) parent->fileCloseHook();
}

//
// class FilesystemBase
//

FilesystemBase::FilesystemBase() :
        filesystemId(FilesystemManager::getFilesystemId()), openFileCount(0) {}

int FilesystemBase::readlink(StringPart& name, string& target)
{
    return -EINVAL; //Default implementation, for filesystems without symlinks
}

bool FilesystemBase::supportsSymlinks() const { return false; }

void FilesystemBase::fileCloseHook() { atomicAdd(&openFileCount,-1); }

void FilesystemBase::newFileOpened() { atomicAdd(&openFileCount,1); }

FilesystemBase::~FilesystemBase() {}

} //namespace miosix
