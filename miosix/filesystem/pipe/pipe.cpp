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

#include "pipe.h"
#include <algorithm>

using namespace std;

#ifdef WITH_FILESYSTEM

namespace miosix {

Pipe::Pipe() : FileBase(intrusive_ref_ptr<FilesystemBase>(),O_RDWR), put(0),
    get(0), size(0), capacity(defaultSize), buffer(new char[defaultSize]) {}

ssize_t Pipe::write(const void *data, size_t len)
{
    auto d=reinterpret_cast<const char*>(data);
    Lock<FastMutex> l(m);
    ssize_t written=0;
    while(len>0)
    {
        if(unconnected()) return -EPIPE;
        int writable=min<int>(len,capacity-size);
        if(writable==0) cv.wait(l);
        else {
            for(int i=0;i<writable;i++)
            {
                buffer[put]=d[i];
                if(++put>=capacity) put=0;
            }
            size+=writable;
            d+=writable;
            len-=writable;
            written+=writable;
        }
    }
    return written;
}

ssize_t Pipe::read(void *data, size_t len)
{
    if(len==0) return 0;
    auto d=reinterpret_cast<char*>(data);
    Lock<FastMutex> l(m);
    for(;;)
    {
        if(unconnected()) return 0;
        int readable=min<int>(len,size);
        //HACK: if the other end of the pipe is closed after we wait on the
        //condition variable, we'll wait forever. To fix that, we set a timeout
        if(readable==0) cv.timedWait(l,getTime()+pollTime);
        else {
            for(int i=0;i<readable;i++)
            {
                d[i]=buffer[get];
                if(++get>=capacity) get=0;
            }
            size-=readable;
            return readable;
        }
    }
}

off_t Pipe::lseek(off_t pos, int whence) { return -ESPIPE; }

int Pipe::ftruncate(off_t size) { return -EINVAL; }

int Pipe::fstat(struct stat *pstat) const
{
    return -EFAULT; //TODO
}

int Pipe::fcntl(int cmd, int opt)
{
    return -EFAULT; //TODO
}

Pipe::~Pipe() { delete[] buffer; }

bool Pipe::unconnected()
{
    //How many references does an unconnected pipe have? One in the file
    //descriptor table. Then, since we check whether we are unconnected within
    //either a read or write operation, either FileDescriptorTable::read or
    //FileDescriptorTable::write hold a temporary reference to make sure a
    //concurrent close doesn't lead to a dangling pointer in the middle of a
    //syscall, bringing the reference count to 2. Finally, to get the reference
    //count, we need to get the intrusive_shared_ptr first as a temporary through
    //shared_from_this() and this adds to the reference count, reaching 3.
    //This should be done with the mutex locked to prevent concurrent calls from
    //not spotting that we are unconnected
    return shared_from_this().use_count()<=3;
}

} //namespace miosix

#endif //WITH_FILESYSTEM
