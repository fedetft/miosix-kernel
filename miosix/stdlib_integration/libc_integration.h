/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013 by Terraneo Federico *
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

#ifndef LIBC_INTEGRATION_H
#define	LIBC_INTEGRATION_H

#include <reent.h>
#include <cstdlib>
#include <cstring>

//Can't yet make this header private, as it's included by kernel.h
//#ifndef COMPILING_MIOSIX
//#error "This is header is private, it can't be used outside Miosix itself."
//#error "If your code depends on a private header, it IS broken."
//#endif //COMPILING_MIOSIX

namespace miosix {

/**
 * \internal
 * \return the heap high watermark. Note that the returned value is a memory
 * address and not a size in bytes. This is just an implementation detail, what
 * you'd want to call is most likely MemoryProfiling::getAbsoluteFreeHeap().
 */
unsigned int getMaxHeap();

//Forward declaration of a class to hide accessors to CReentrancyData
class CReentrancyAccessor;

/**
 * \internal
 * This is a wrapper class that contains all per-thread data required to make
 * the C standard library thread safe.
 */
class CReentrancyData
{
public:
    /**
     * Constructor, creates a new newlib reentrancy structure and initializes it
     */
    CReentrancyData(bool useDefault=false)
    {
        if(useDefault==false)
        {
            threadReent=(struct _reent*)malloc(sizeof(_reent));
            if(threadReent) _REENT_INIT_PTR(threadReent);
        } else threadReent=_GLOBAL_REENT;
    }
    
    /**
     * \return true if the C reentrancy data was initialized successfully 
     */
    bool isInitialized() const { return threadReent!=0; }

    /**
     * Destructor, reclaims the reentrancy structure
     */
    ~CReentrancyData()
    {
        _reclaim_reent(threadReent);
        if(threadReent && threadReent!=_GLOBAL_REENT) free(threadReent);
    }

private:
    CReentrancyData(const CReentrancyData&);
    CReentrancyData& operator=(const CReentrancyData&);

    struct _reent *threadReent; ///< Pointer to the reentrancy structure
    
    /**
     * \return the reentrancy structure 
     */
    struct _reent *getReent() const { return threadReent; } 

    friend class CReentrancyAccessor;
};

} //namespace miosix

#endif //LIBC_INTEGRATION_H
