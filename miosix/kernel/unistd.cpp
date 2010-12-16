/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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

/*
 * unistd.cpp Part of the Miosix Embedded OS. Provides a preliminary
 * implementation of some of the functions in <unistd.h>
 */

#include "miosix.h"
#include <unistd.h>

using namespace miosix;

//These functions needs to be callable from C
extern "C" {

/**
 * Sleep at least the specified number of seconds.
 * Always return 0 (success)
 */
unsigned int sleep(unsigned int __seconds)
{
    Thread::sleep(__seconds*1000);
    return 0;
}

/**
 * Sleep at least the specified number of microseconds.
 * Always return 0 (success)
 */
int usleep(useconds_t __useconds)
{
    if(__useconds>1000) Thread::sleep(__useconds / 1000);
    delayUs(__useconds % 1000);
    return 0;
}


}// extern C
