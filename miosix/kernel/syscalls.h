/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010 by Terraneo Federico                   *
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

#ifndef SYSCALLS_H
#define	SYSCALLS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * \internal
 * This function has nothing to do with syscalls, it is used by system.cpp
 * to implement heap usege statistics.
 * FIXME: If this function is called when _sbrk_r has never been called it will
 * return a wrong value (0)
 */
unsigned int getMaxHeap();

/**
 * \internal
 * _sbrk_r, malloc calls it to increase the heap size
 * Note Jul 4, 2010: incr can also be a negative value, indicating a request to
 * decrease heap space
 */
void *_sbrk_r(struct _reent *ptr, ptrdiff_t incr);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif	//SYSCALLS_H
