/***************************************************************************
 *   Copyright (C) 2026 by Niccolò Betto                                   *
 *   Copyright (C) 2001-2004 by Swedish Institute of Computer Science      *
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

#pragma once

#include "lwip/opt.h"

#include "lwip/err.h"
#include "sys/socket.h"
#include "lwip/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_SOCKETS MEMP_NUM_NETCONN

/** This is overridable for the rare case where more than 255 threads
 * select on the same socket...
 */
#ifndef SELWAIT_T
#define SELWAIT_T u8_t
#endif

union lwip_sock_lastdata {
  struct netbuf *netbuf;
  struct pbuf *pbuf;
};

/** Contains all internal pointers and states used for a socket */
struct lwip_sock {
  /** sockets currently are built on netconns, each socket has one netconn */
  struct netconn *conn;
  /** data that was left from the previous read */
  union lwip_sock_lastdata lastdata;
#if LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL
  /** number of times data was received, set by event_callback(),
      tested by the receive and select functions */
  s16_t rcvevent;
  /** number of times data was ACKed (free send buffer), set by event_callback(),
      tested by select */
  u16_t sendevent;
  /** error happened for this socket, set by event_callback(), tested by select */
  u16_t errevent;
  /** counter of how many threads are waiting for this socket using select */
  SELWAIT_T select_waiting;
#endif /* LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL */
#if LWIP_NETCONN_FULLDUPLEX
  /* counter of how many threads are using a struct lwip_sock (not the 'int') */
  u8_t fd_used;
  /* status of pending close/delete actions */
  u8_t fd_free_pending;
#define LWIP_SOCK_FD_FREE_TCP  1
#define LWIP_SOCK_FD_FREE_FREE 2
#endif
};

#if !LWIP_TCPIP_CORE_LOCKING
/** Maximum optlen used by setsockopt/getsockopt */
#define LWIP_SETGETSOCKOPT_MAXOPTLEN LWIP_MAX(16, sizeof(struct ifreq))

/** This struct is used to pass data to the set/getsockopt_impl
 * functions running in tcpip_thread context (only a void* is allowed) */
struct lwip_setgetsockopt_data {
  /** socket index for which to change options */
  lwip_sock* sock;
  /** level of the option to process */
  int level;
  /** name of the option to process */
  int optname;
  /** set: value to set the option to
    * get: value of the option is stored here */
#if LWIP_MPU_COMPATIBLE
  u8_t optval[LWIP_SETGETSOCKOPT_MAXOPTLEN];
#else
  union {
    void *p;
    const void *pc;
  } optval;
#endif
  /** size of *optval */
  socklen_t optlen;
  /** if an error occurs, it is temporarily stored here */
  int err;
  /** semaphore to wake up the calling task */
  void* completed_sem;
};
#endif /* !LWIP_TCPIP_CORE_LOCKING */

#ifdef __cplusplus
}
#endif

#if LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL

#if LWIP_NETCONN_SEM_PER_THREAD
#define SELECT_SEM_T        sys_sem_t*
#define SELECT_SEM_PTR(sem) (sem)
#else /* LWIP_NETCONN_SEM_PER_THREAD */
#define SELECT_SEM_T        sys_sem_t
#define SELECT_SEM_PTR(sem) (&(sem))
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

/** Description for a task waiting in select */
struct lwip_select_cb {
  /** Pointer to the next waiting task */
  struct lwip_select_cb *next;
  /** Pointer to the previous waiting task */
  struct lwip_select_cb *prev;
#if LWIP_SOCKET_SELECT
  /** readset passed to select */
  fd_set *readset;
  /** writeset passed to select */
  fd_set *writeset;
  /** unimplemented: exceptset passed to select */
  fd_set *exceptset;
#endif /* LWIP_SOCKET_SELECT */
#if LWIP_SOCKET_POLL
  /** fds passed to poll; NULL if select */
  struct pollfd *poll_fds;
  /** nfds passed to poll; 0 if select */
  nfds_t poll_nfds;
#endif /* LWIP_SOCKET_POLL */
  /** don't signal the same semaphore twice: set to 1 when signalled */
  int sem_signalled;
  /** semaphore to wake up a task waiting for select */
  SELECT_SEM_T sem;
};
#endif /* LWIP_SOCKET_SELECT || LWIP_SOCKET_POLL */
