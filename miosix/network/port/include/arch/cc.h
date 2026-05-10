/***************************************************************************
 *   Copyright (C) 2026 by Niccolò Betto                                   *
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

#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Use the system provided struct timeval */
#define LWIP_TIMEVAL_PRIVATE 0
#include <sys/time.h>

/* Use the system provided errno */
#define LWIP_ERRNO_STDINCLUDE 1

// TODO: implement good source of random to LWIP_RAND()

// TODO: byte order swap functions
/*
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

// Include the impl header directly to facilitate inlining
#include <interfaces-impl/endianness_impl.h>

#ifndef lwip_htons
u16_t lwip_htons(u16_t x);
#endif
#define lwip_ntohs(x) lwip_htons(x)

#ifndef lwip_htonl
u32_t lwip_htonl(u32_t x);
#endif
#define lwip_ntohl(x) lwip_htonl(x)
*/

#ifdef __cplusplus
}
#endif

#endif /* LWIP_ARCH_CC_H */
