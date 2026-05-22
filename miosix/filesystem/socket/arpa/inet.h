/***************************************************************************
 *   Copyright (C) 2026 by Niccolò Betto                                   *
 *   Copyright (C) 1997-2026 Free Software Foundation, Inc.                *
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

#ifndef _ARPA_INET_H_
#define	_ARPA_INET_H_

#include <sys/types.h>
#include <netinet/in.h>

#ifndef __socklen_t_defined
typedef uint32_t socklen_t;
#define __socklen_t_defined
#endif

#define inet_addr_from_ip4addr(target_inaddr, source_ipaddr) \
    ((target_inaddr)->s_addr = ip4_addr_get_u32(source_ipaddr))

#define inet_addr_to_ip4addr(target_ipaddr, source_inaddr) \
    (ip4_addr_set_u32(target_ipaddr, (source_inaddr)->s_addr))

#ifdef __cplusplus
extern "C" {
#endif

/* Convert the unsigned integer hostlong from host byte order
   to network byte order.  */
extern uint32_t htonl(uint32_t hostlong);

/* Convert the unsigned short integer hostshort from host byte order
   to network byte order.  */
extern uint16_t htons(uint16_t hostshort);

/* Convert the unsigned integer netlong from network byte order
   to host byte order.  */
extern uint32_t ntohl(uint32_t netlong);

/* Convert the unsigned short integer netshort from network byte order
    to host byte order.  */
extern uint16_t ntohs(uint16_t netshort);

/* directly map these to the lwip internal functions */

/* Convert Internet host address from numbers-and-dots notation in CP
   into binary data in network byte order.  */
#define inet_addr(cp) ipaddr_addr(cp)

/* Convert Internet host address from numbers-and-dots notation in CP
   into binary data and store the result in the structure ADDR.  */
#define inet_aton(cp, addr) ip4addr_aton(cp, (ip4_addr_t*)addr)

/* Convert Internet number in IN to ASCII representation.  The return value
   is a pointer to an internal array containing the string.  */
#define inet_ntoa(addr) ip4addr_ntoa((const ip4_addr_t*)&(addr))

/* Convert Internet number in IN to ASCII representation and place result
   in buffer starting at BUF with length of LEN bytes.  */
#define inet_ntoa_r(addr, buf, buflen) \
    ip4addr_ntoa_r((const ip4_addr_t*)&(addr), buf, buflen)

/* Convert from presentation format of an Internet number in buffer
   starting at CP to the binary network format and store result for
   interface type AF in buffer starting at BUF.  */
extern int inet_pton(int af, const char *__restrict cp,
		     void *__restrict buf) __THROW;

/* Convert a Internet address in binary network format for interface
   type AF in buffer starting at CP to presentation form and place
   result in buffer of length LEN astarting at BUF.  */
extern const char *inet_ntop(int af, const void *__restrict cp,
			     char *__restrict buf, socklen_t len) __THROW;

#ifdef __cplusplus
}
#endif

#endif /* _ARPA_INET_H_ */
