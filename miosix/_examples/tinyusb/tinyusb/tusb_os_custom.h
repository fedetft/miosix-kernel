/***************************************************************************
 *   Copyright (c) 2023 by Daniele Cattaneo                                *
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
 
// tinyUSB OS abstraction layer for Miosix
// Based on osal_none.h, (c) 2019 Ha Thach

#ifndef OSAL_MIOSIX_H
#define OSAL_MIOSIX_H

#ifndef __cplusplus
#error "tinyUSB for Miosix must be compiled with the C++ compiler!"
#endif

extern "C++" {

#include "miosix.h"
#include <pthread.h>

//--------------------------------------------------------------------+
// TASK API
//--------------------------------------------------------------------+
TU_ATTR_ALWAYS_INLINE static inline void osal_task_delay(uint32_t msec)
{
    miosix::Thread::sleep(msec);
}

//--------------------------------------------------------------------+
// Binary Semaphore API
//--------------------------------------------------------------------+
// Only CDC RNDIS host code is using this. Implementation uses Miosix
// semaphores.
typedef miosix::Semaphore osal_semaphore_def_t;
typedef miosix::Semaphore *osal_semaphore_t;

TU_ATTR_ALWAYS_INLINE static inline osal_semaphore_t osal_semaphore_create(osal_semaphore_def_t* semdef)
{
    return semdef;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_semaphore_post(osal_semaphore_t sem_hdl, bool in_isr)
{
    if(in_isr) sem_hdl->IRQsignal();
    else sem_hdl->signal();
    return true;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_semaphore_wait(osal_semaphore_t sem_hdl, uint32_t msec)
{
    if(msec == OSAL_TIMEOUT_WAIT_FOREVER) sem_hdl->wait();
    else sem_hdl->timedWait(miosix::getTime() + (long long)msec*1000000LL);
    return true;
}

TU_ATTR_ALWAYS_INLINE static inline void osal_semaphore_reset(osal_semaphore_t sem_hdl)
{
    // Not implemented, scheduled to be removed
}

//--------------------------------------------------------------------+
// MUTEX API
// Within tinyusb, mutex is never used in ISR context
//--------------------------------------------------------------------+
// Just use pthread
typedef pthread_mutex_t osal_mutex_def_t;
typedef pthread_mutex_t *osal_mutex_t;

TU_ATTR_ALWAYS_INLINE static inline osal_mutex_t osal_mutex_create(osal_mutex_def_t *mdef)
{
    pthread_mutex_init(mdef, NULL);
    return mdef;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_mutex_lock(osal_mutex_t mutex_hdl, uint32_t msec)
{
    return pthread_mutex_lock(mutex_hdl)==0;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_mutex_unlock(osal_mutex_t mutex_hdl)
{
    return pthread_mutex_unlock(mutex_hdl)==0;
}

//--------------------------------------------------------------------+
// QUEUE API
//--------------------------------------------------------------------+
// Miosix's templated Queue would be great for implementing this, but tinyUSB's
// coding style makes it impossible to instantiate the correct templated
// functions because osal_queue_t is the only object passed around after
// initialization and cannot be templated.
//   This could be solved through better API design by the tinyUSB people, but
// that will not happen as except for Miosix the embedded/RT ecosystem is
// C-centric and the inertia is too great to overcome.
//   Therefore we use tusb's fifo class with our own synchronization using
// low-level Miosix APIs. We use FastInterruptDisableLock for protecting the
// fifo because it might be changed from interrupt context.
#include "common/tusb_fifo.h"

struct osal_queue_def_t {
    // Cannot use the macro because it's C++-incompatible
    osal_queue_def_t(uint8_t *buf, uint16_t depth, uint16_t itmSz, bool overwr)
    {
        fifo.buffer = buf;
        fifo.depth = depth;
        fifo.item_size = itmSz;
        fifo.overwritable = overwr;
    }
    miosix::Semaphore itemAvailable;
    tu_fifo_t fifo;
};
typedef osal_queue_def_t* osal_queue_t;

// _int_set is used as mutex in OS NONE (disable/enable USB ISR)
#define OSAL_QUEUE_DEF(_int_set, _name, _depth, _type) \
    uint8_t _name##_buf[_depth*sizeof(_type)]; \
    osal_queue_def_t _name(_name##_buf, _depth, sizeof(_type), false);

TU_ATTR_ALWAYS_INLINE static inline osal_queue_t osal_queue_create(osal_queue_def_t *qdef)
{
    tu_fifo_clear(&qdef->fifo);
    return (osal_queue_t)qdef;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_queue_receive(osal_queue_t qhdl, void *data, uint32_t msec)
{
    if (msec == OSAL_TIMEOUT_WAIT_FOREVER) qhdl->itemAvailable.wait();
    else qhdl->itemAvailable.timedWait(miosix::getTime() + (long long)msec*1000000LL);
    bool success;
    {
        miosix::FastInterruptDisableLock dLock;
        success = tu_fifo_read(&qhdl->fifo, data);
    }
    return success;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_queue_send(osal_queue_t qhdl, void const *data, bool in_isr)
{
    bool success;
    if(in_isr)
    {
        success=tu_fifo_write(&qhdl->fifo,data);
        qhdl->itemAvailable.IRQsignal();
    } else {
        {
            miosix::FastInterruptDisableLock dLock;
            success=tu_fifo_write(&qhdl->fifo,data);
        }
        qhdl->itemAvailable.signal();
    }
    TU_ASSERT(success);
    return success;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_queue_empty(osal_queue_t qhdl)
{
    // Skip queue lock/unlock since this function is primarily called
    // with interrupt disabled before going into low power mode
    return tu_fifo_empty(&qhdl->fifo);
}

}

#endif
