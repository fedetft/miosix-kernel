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

#include "base_files.h"
#include <errno.h>

using namespace std;

#ifdef WITH_DEVFS

namespace miosix {

//
// class NullFile
//

ssize_t NullFile::write(const void *data, size_t len)
{
    return len;
}

ssize_t NullFile::read(void *data, size_t len)
{
    return -EBADF;
}

//
// class ZeroFile
//

ssize_t ZeroFile::write(const void *data, size_t len)
{
    return -EBADF;
}

ssize_t ZeroFile::read(void *data, size_t len)
{
    memset(data,0,len);
    return len;
}

//
// class MessageFileGenerator
//

int MessageFileGenerator::open(intrusive_ref_ptr<FileBase>& file, int flags,
        int mode)
{
    //TODO: add enable_shared_from_this!!
    Lock<FastMutex> l(mutex);
    file=intrusive_ref_ptr<FileBase>(
        new MessageFile(intrusive_ref_ptr<DeviceFileGenerator>(this),message));
    return 0;
}

//
// class MessageFileGenerator::MessageFile
//

ssize_t MessageFileGenerator::MessageFile::write(const void* data, size_t len)
{
    return -EBADF;
}

ssize_t MessageFileGenerator::MessageFile::read(void* data, size_t len)
{
    //The mutex is important, since if two threads call read the resulting race
    //condition could cause index>length(), so length()-index becomes a high
    //positive value and that could allow dumping part of the kernel memory
    Lock<FastMutex> l(mutex);
    size_t toRead=min(len,message.length()-index);
    memcpy(data,message.c_str()+index,toRead);
    index+=toRead;
    return toRead;
}

} //namespace miosix

#endif //WITH_DEVFS
