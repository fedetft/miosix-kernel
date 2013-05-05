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

using namespace std;

namespace miosix {

//
// class DevFs
//

int DevFs::open(intrusive_ref_ptr<FileBase>& file, std::string name, int flags,
        int mode)
{
    
}

int DevFs::waitUntilNoOpenFiles()
{
    const int timeout=100; //In milliseconds
    for(int i=0;i<timeout;i++)
    {
        {
            Lock l(mutex);
            map<string,intrusive_ref_ptr<FileBase> >::iterator it;
            bool ok=true;
            for(it=files.begin();it!=files.end();++it)
            {
                if(it->use_count()==1) continue;
                ok=false;
                break;
            }
            if(ok) return 0;//All files have no other references outside the map
        }
        Thread::sleep(1);
    }
    //This may happen, as operations on devfs may be blocking
    return -EBUSY;
}

} //namespace miosix
