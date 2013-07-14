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

#include "devfs.h"
#include "console_device.h"

using namespace std;

namespace miosix {

//
// class DevFs
//

DevFs::DevFs()
{
    string null="/null";
    string zero="/zero";
    string console="/console";
    
    files[StringPart(null)]=intrusive_ref_ptr<FileBase>(new NullFile);
    files[StringPart(zero)]=intrusive_ref_ptr<FileBase>(new ZeroFile);
    
    intrusive_ref_ptr<FileBase> consoleDev=DefaultConsole::instance().get();
    if(consoleDev) files[StringPart(console)]=consoleDev;
    else files[StringPart(console)]=files[StringPart(null)];
}

int DevFs::open(intrusive_ref_ptr<FileBase>& file, StringPart& name, int flags,
        int mode)
{
    //TODO: mode & flags
    Lock<FastMutex> l(mutex);
    map<StringPart,intrusive_ref_ptr<FileBase> >::iterator it=files.find(name);
    if(it==files.end()) return -ENOENT;
    file=it->second;
    return 0;
}

int DevFs::lstat(StringPart& name, struct stat *pstat)
{
    Lock<FastMutex> l(mutex);
    map<StringPart,intrusive_ref_ptr<FileBase> >::iterator it=files.find(name);
    if(it==files.end()) return -ENOENT;
    it->second->fstat(pstat);
    return 0;
}

int DevFs::mkdir(StringPart& name, int mode)
{
    return -EACCES; // No directories support in DevFs yet
}

bool DevFs::areAllFilesClosed()
{
    Lock<FastMutex> l(mutex);
    //Can't use openFileCount in devFS, as one instance of each file is stored
    //in the map. Rather, check the reference count value. No need to use
    //atomic ops to make a copy of the file before calling use_count() as the
    //existence of at least one reference in the map guarantees the file won't
    //be deleted.
    map<StringPart,intrusive_ref_ptr<FileBase> >::iterator it;
    for(it=files.begin();it!=files.end();++it)
        if(it->second.use_count()>1) return false;
    return true;
}

} //namespace miosix
