/***************************************************************************
 *   Copyright (C) 2008 2009 2010 by Terraneo Federico                     *
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

#include "conflict_table.h"

namespace miosix {

OpenState ConflictTable::isOpen(const char *filename) const
{
    //Despite using a hash table this is still slow because every time this
    //function is called with a file not yet open (and the probability is high)
    //the whole array is scanned. Fix it someday
    int index=hash(filename);
    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        if(fileData[index].isThisFile(filename))
        {
            if(fileData[index].write) return OPEN_WRITE; else return OPEN_READ;
        } else {
            //Collision in the hashtable
            if(++index>=MAX_OPEN_FILES) index=0;
        }
    }
    return NOT_OPEN;
}

void ConflictTable::open(int fd, const char *filename, OpenState mode)
{
    int index=hash(filename);
    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        if(files[index].isEmpty())
        {

        } else {
            //Collision in the hashtable
            if(++index>=MAX_OPEN_FILES) index=0;
        }
    }
}

void ConflictTable::close(int fd)
{

}

unsigned int ConflictTable::hash(const char* filename)
{
    //Bernstein hash
    unsigned int result=5381;
    while(*filename!='\0')
    {
        result=33*result+*filename;
        filename++;
    }
    return result % MAX_OPEN_FILES;
}

} //namespace miosix
