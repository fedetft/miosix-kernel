/***************************************************************************
 *   Copyright (C) 2023 by Terraneo Federico                               *
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

#include "e20/unmember.h"

namespace miosix {

/**
 * Register an interrupt handler.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param handler pointer to the handler function of type void (*)(void*)
 * \param arg optional void* argument. This argument is stored in the interrupt
 * handling logic and passed as-is whenever the interrupt handler is called.
 * If omitted, the handler function is called with nullptr as argument.
 * \return true if the interrupt was succesfully registered, false otherwise.
 * An interrupt handler can only be registered if no other handler is registered
 * for the same id.
 */
bool IRQregisterIrq(unsigned int id, void (*handler)(void*), void *arg=nullptr) noexcept;

/**
 * Register an interrupt handler.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param handler pointer to the handler function of type void (*)()
 * \return true if the interrupt was succesfully registered, false otherwise.
 * An interrupt handler can only be registered if no other handler is registered
 * for the same id.
 */
inline bool IRQregisterIrq(unsigned int id, void (*handler)()) noexcept
{
    return IRQregisterIrq(id,reinterpret_cast<void (*)(void*)>(handler));
}

/**
 * Register a class member function as an interrupt handler.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param mfn member function pointer to the class method to be registered as
 * interrupt handler. The method shall take no paprameters.
 * \param object class intance whose methos shall be called as interrupt hanlder.
 * \return true if the interrupt was succesfully registered, false otherwise.
 * An interrupt handler can only be registered if no other handler is registered
 * for the same id.
 */
template<typename T>
inline bool IRQregisterIrq(unsigned int id, void (T::*mfn)(), T *object) noexcept
{
    auto result=unmember(mfn,object);
    return IRQregisterIrq(id,std::get<0>(result),std::get<1>(result));
}

/**
 * Unregister an interrupt handler.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be unregistered.
 */
void IRQunregisterIrq(unsigned int id) noexcept;

/**
 * Check whether an interrupt handler is currently registered.
 * \param id platform-dependent id of the peripheral for which to check whether
 * an interrupt handler is registered.
 * \return true if an interrupt hander is currently registered for that id.
 */
bool IRQisIrqRegistered(unsigned int id) noexcept;

/**
 * This function can be called from within an interrupt or with interrupts
 * disabled to invoke the scheduler. The request is not performed immediately,
 * it is performed as soon as the interrupt returns or the interrupts are
 * enabled again.
 *
 * As a special exception despite the name, the function is also safe to be
 * called with interrupts enabled, even though you should call Thread::yield()
 * in this case. This function is however NOT safe to be called when the kernel
 * is paused as it will lead to an unwanted context switch and likely a deadlock.
 */
void IRQinvokeScheduler() noexcept;

} //namespace miosix
