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

#include <arch/sys_arch.h>
#include <lwip/stats.h>
#include <lwip/sys.h>

#include <kernel/lock.h>
#include <kernel/queue.h>
#include <kernel/sync.h>
#include <kernel/thread.h>

using namespace miosix;

// These functions are friends of PauseKernelLock
namespace miosix {
bool networkLockImpl() { return PauseKernelLock::pushLock(); }
void networkUnlockImpl(bool pval) {
    if (!pval)
        PauseKernelLock::unlock();
}
} // namespace miosix

extern "C" {

void sys_init() {
    // Nothing to initialize
}

u32_t sys_now() {
    // NOTE: Don't care for wraparound, only used for time diffs by lwIP
    return getTime() / 1'000'000U;
}
u32_t sys_jiffies() {
    // NOTE: Don't care for wraparound, only used as a source of random by lwIP
    return getTime();
}

/* Lightweight protection functions */

sys_prot_t sys_arch_protect() { return networkLockImpl(); }
void sys_arch_unprotect(sys_prot_t pval) { networkUnlockImpl(pval); }

/* Thread functions */

void sys_msleep(u32_t ms) { Thread::sleep(ms); }

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg,
                            int stacksize, int prio) {
    LWIP_UNUSED_ARG(name);

    // Reasonable default for stacksize
    stacksize = std::max((unsigned int)stacksize, STACK_DEFAULT_FOR_PTHREAD);

    // ASKME: JOINABLE or DETACHED?
    auto *t =
        Thread::create(thread, stacksize, prio, arg, Thread::Options::DETACHED);
    assert(t != nullptr); // sys_thread_new must not fail

    return {.thread_handle = t};
}

/* Mutex functions */

// Need priority inheritance
using MutexType = Mutex;

err_t sys_mutex_new(sys_mutex_t *mutex) {
    mutex->mut = new MutexType();
    if (!mutex->mut)
        return ERR_MEM;
    return ERR_OK;
}

void sys_mutex_lock(sys_mutex_t *mutex) {
    static_cast<MutexType *>(mutex->mut)->lock();
}

void sys_mutex_unlock(sys_mutex_t *mutex) {
    static_cast<MutexType *>(mutex->mut)->unlock();
}

void sys_mutex_free(sys_mutex_t *mutex) {
    delete static_cast<MutexType *>(mutex->mut);
}

/* Semaphore functions */

err_t sys_sem_new(sys_sem_t *sem, u8_t count) {
    sem->sem = new Semaphore(count);
    if (!sem->sem)
        return ERR_MEM;
    return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem) {
    static_cast<Semaphore *>(sem->sem)->signal();
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) {
    auto *semaphore = static_cast<Semaphore *>(sem->sem);
    if (timeout == 0) {
        // Wait indefinitely
        semaphore->wait();
        return 0;
    }

    auto timeoutNs = static_cast<long long>(timeout) * 1'000'000;
    auto absTime = getTime() + timeoutNs;
    auto waitResult = semaphore->timedWait(absTime);

    return waitResult == TimedWaitResult::Timeout ? SYS_ARCH_TIMEOUT : 0;
}

void sys_sem_free(sys_sem_t *sem) { delete static_cast<Semaphore *>(sem->sem); }

/* Mailbox functions */

using QueueType = DynQueue<void *>;

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
    mbox->mbx = new QueueType(size);
    if (!mbox->mbx)
        return ERR_MEM;

    return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg) {
    auto *queue = static_cast<QueueType *>(mbox->mbx);

    queue->put(msg);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg) {
    auto *queue = static_cast<QueueType *>(mbox->mbx);

    if (queue->isFull())
        return ERR_MEM;

    queue->put(msg);
    return ERR_OK;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg) {
    auto *queue = static_cast<QueueType *>(mbox->mbx);

    bool posted = queue->IRQput(msg);
    if (!posted)
        return ERR_MEM;

    return ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout) {
    auto *queue = static_cast<QueueType *>(mbox->mbx);

    if (timeout == 0) {
        // Wait indefinitely
        queue->get(*msg);
        return 0;
    }

    auto timeoutNs = static_cast<long long>(timeout) * 1'000'000;
    auto absTime = getTime() + timeoutNs;
    auto waitResult = queue->timedGet(*msg, absTime);
    return waitResult == TimedWaitResult::Timeout ? SYS_ARCH_TIMEOUT : 0;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg) {
    auto *queue = static_cast<QueueType *>(mbox->mbx);

    if (queue->isEmpty())
        return SYS_MBOX_EMPTY;

    queue->get(*msg);
    return 0;
}

void sys_mbox_free(sys_mbox_t *mbox) {
    delete static_cast<QueueType *>(mbox->mbx);
}

} // extern "C"
