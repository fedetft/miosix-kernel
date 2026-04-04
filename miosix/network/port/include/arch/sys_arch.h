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

#pragma once

#include <lwip/opt.h>
#include <lwip/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lightweight protection: mapped to PauseKernelLock */
#if SYS_LIGHTWEIGHT_PROT
/* bool is not available in C, use u32_t to store PauseKernelLock state */
typedef u32_t sys_prot_t;
#endif

/* Mutexes: mapped to miosix::Mutex */
#if !LWIP_COMPAT_MUTEX
struct _miosix_mutex_wrapper {
  void *mut; // miosix::Mutex*
};
typedef struct _miosix_mutex_wrapper sys_mutex_t;
#define sys_mutex_valid_val(mutex)   ((mutex).mut != NULL)
#define sys_mutex_valid(mutex)       (((mutex) != NULL) && sys_mutex_valid_val(*(mutex)))
#define sys_mutex_set_invalid(mutex) ((mutex)->mut = NULL)
#endif /* !LWIP_COMPAT_MUTEX */

/* Semaphores: mapped to miosix::Semaphore */
struct _miosix_semaphore_wrapper {
  void *sem; // miosix::Semaphore*
};
typedef struct _miosix_semaphore_wrapper sys_sem_t;
#define sys_sem_valid_val(sema)   ((sema).sem != NULL)
#define sys_sem_valid(sema)       (((sema) != NULL) && sys_sem_valid_val(*(sema)))
#define sys_sem_set_invalid(sema) ((sema)->sem = NULL)

/* Mailboxes: mapped to miosix::Queue */
struct _miosix_queue_wrapper {
  void *mbx; // miosix::DynQueue*
  void *mutex; // miosix::Mutex*
};
typedef struct _miosix_queue_wrapper sys_mbox_t;
#define sys_mbox_valid_val(mbox)   ((mbox).mbx != NULL)
#define sys_mbox_valid(mbox)       (((mbox) != NULL) && sys_mbox_valid_val(*(mbox)))
#define sys_mbox_set_invalid(mbox) ((mbox)->mbx = NULL)

/* Threads: mapped to miosix::Thread */
struct _miosix_thread_wrapper {
  void *thread_handle; // type-erased pointer to miosix::Thread
};
typedef struct _miosix_thread_wrapper sys_thread_t;
    
#ifdef __cplusplus
}
#endif
