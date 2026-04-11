/***************************************************************************
 *   Copyright (C) 2023-2025 by Terraneo Federico                          *
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
#include "miosix_settings.h"

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file interrupts.h
 * This file provides a common interface to register interrupts in the Miosix
 * kernel. Contrary to Miosix 2.x that used the architecture-specific interrupt
 * registration mechanism, from Miosix 3.0 interrupts are registered at run-time
 * by calling IRQregisterIrq().
 * Additionally, interrupts can be registered with an optional void* argument,
 * or a non-static class member function can be registered as an interrupt.
 *
 * This interface is (currently) only concerned with registering the pointers
 * to the interrupt handler functions, not to setting other properties of
 * interrupts such as their priority, which if needed is still done with
 * architecture-specific code.
 *
 * The interface in this header is meant to be used by all device drivers and
 * code that deals with interrupt handlers.
 *
 * For people who need to implement this interface on a new CPU or architecture,
 * there is one additional function to implement:
 * \code
 * namespace miosix {
 * void IRQinitIrqTable() noexcept;
 * }
 * \endcode
 * that is called during the boot phase to set un the interrupt table, whose
 * implementation shall be to initialize all peripheral interrupt handlers to
 * a default handler so that unexpected interrupts do not cause undefined
 * behavior.
 */

namespace miosix {

/**
 * \name Enabling and disabling interrupts
 * \{
 */

/**
 * \internal
 * Disable interrupts only on the core it is called from.
 *
 * This is currently meant to be an implementation detail used to implement the
 * global lock and we don't expect it to be useful for other uses, that's why
 * we're currently not providing RAII-style lock classes, though this may change
 * in the future if some use case for this function is found.
 *
 * \note If you're trying to use this function for driver development maybe
 * you're looking for FastGlobalIrqLock::lock() instead?
 */
inline void fastDisableIrq() noexcept;

/**
 * \internal
 * Enable back interrupts on the core it was called from, after they have been
 * disabled by a call to fastDisableIrq().
 *
 * This is currently meant to be an implementation detail used to implement the
 * global lock and we don't expect it to be useful for other uses, that's why
 * we're currently not providing RAII-style lock classes, though this may change
 * in the future if some use case for this function is found.
 *
 * \note If you're trying to use this function for driver development maybe
 * you're looking for FastGlobalIrqLock::unlock() instead?
 */
inline void fastEnableIrq() noexcept;

/**
 * \internal
 * Provides a way to know if interrupts are enabled or not.
 *
 * \return true if interrupts are enabled
 * \warning Using this function is discouraged
 */
inline bool areInterruptsEnabled() noexcept;

/**
 * \}
 */

/**
 * \name Dynamic Interrupt Handler Registration
 * \{
 */

class GlobalIrqLock; // lock.h

#ifdef WITH_SMP
/**
 * Register an interrupt handler on a specific core.
 * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
 * \param coreId the identifier of the core that will handle the interrupt.
 * After calling this function, the same interrupt will not be register-able
 * on any other core, unless it is unregistered first.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param handler pointer to the handler function of type void (*)(void*)
 * \param arg optional void* argument. This argument is stored in the interrupt
 * handling logic and passed as-is whenever the interrupt handler is called.
 * If omitted, the handler function is called with nullptr as argument.
 *
 * \note This function calls errorHandler() causing a reboot if attempting to
 * register an already registered interrupt. If your driver can tolerate failing
 * to register an interrupt you should call IRQisIrqRegistered() to test whether
 * an interrupt is already registered for that id before calling IRQregisterIrq()
 */
void IRQregisterIrqOnCore(GlobalIrqLock& lock, unsigned char coreId, unsigned int id,
                          void (*handler)(void*), void *arg=nullptr) noexcept;
#endif

/**
 * Register an interrupt handler.
 * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param handler pointer to the handler function of type void (*)(void*)
 * \param arg optional void* argument. This argument is stored in the interrupt
 * handling logic and passed as-is whenever the interrupt handler is called.
 * If omitted, the handler function is called with nullptr as argument.
 *
 * \note This function calls errorHandler() causing a reboot if attempting to
 * register an already registered interrupt. If your driver can tolerate failing
 * to register an interrupt you should call IRQisIrqRegistered() to test whether
 * an interrupt is already registered for that id before calling IRQregisterIrq()
 */
#ifdef WITH_SMP
inline void IRQregisterIrq(GlobalIrqLock& lock, unsigned int id,
                           void (*handler)(void*), void *arg=nullptr) noexcept
{
    IRQregisterIrqOnCore(lock,0,id,handler,arg);
}
#else
void IRQregisterIrq(GlobalIrqLock& lock, unsigned int id,
                    void (*handler)(void*), void *arg=nullptr) noexcept;
#endif

/**
 * Register an interrupt handler.
 * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param handler pointer to the handler function of type void (*)()
 *
 * \note This function calls errorHandler() causing a reboot if attempting to
 * register an already registered interrupt. If your driver can tolerate failing
 * to register an interrupt you should call IRQisIrqRegistered() to test whether
 * an interrupt is already registered for that id before calling IRQregisterIrq()
 */
inline void IRQregisterIrq(GlobalIrqLock& lock, unsigned int id, void (*handler)()) noexcept
{
    IRQregisterIrq(lock,id,reinterpret_cast<void (*)(void*)>(handler));
}

/**
 * Register a class member function as an interrupt handler.
 * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param mfn member function pointer to the class method to be registered as
 * interrupt handler. The method shall take no parameters.
 * \param object class instance whose methods shall be called as interrupt
 * handler.
 *
 * \note This function calls errorHandler() causing a reboot if attempting to
 * register an already registered interrupt. If your driver can tolerate failing
 * to register an interrupt you should call IRQisIrqRegistered() to test whether
 * an interrupt is already registered for that id before calling IRQregisterIrq()
 */
template<typename T>
inline void IRQregisterIrq(GlobalIrqLock& lock, unsigned int id, void (T::*mfn)(), T *object) noexcept
{
    auto result=unmember(mfn,object);
    IRQregisterIrq(lock,id,std::get<0>(result),std::get<1>(result));
}

#ifdef WITH_SMP
/**
 * Unregister an interrupt handler from a specific core.
 * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
 * \param coreId the identifier of the core that was registered for handling the
 * interrupt. If the core is not the same one that was used for originally
 * registering the IRQ, the function's behavior is undefined.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be unregistered.
 * \param handler pointer to the currently registered handler function of type
 * void (*)(void*)
 * \param arg optional void* argument previously specified when registering
 * the handler.
 *
 * \note This function calls errorHandler() causing a reboot if attempting to
 * unregister a different interrupt than the currently registered one
 */
void IRQunregisterIrqOnCore(GlobalIrqLock& lock, unsigned char coreId,
    unsigned int id, void (*handler)(void*), void *arg=nullptr) noexcept;
#endif

/**
 * Unregister an interrupt handler.
 * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be unregistered.
 * \param handler pointer to the currently registered handler function of type
 * void (*)(void*)
 * \param arg optional void* argument previously specified when registering
 * the handler.
 *
 * \note This function calls errorHandler() causing a reboot if attempting to
 * unregister a different interrupt than the currently registered one
 */
#ifdef WITH_SMP
inline void IRQunregisterIrq(GlobalIrqLock& lock, unsigned int id,
                             void (*handler)(void*), void *arg=nullptr) noexcept
{
    IRQunregisterIrqOnCore(lock,0,id,handler,arg);
}
#else
void IRQunregisterIrq(GlobalIrqLock& lock, unsigned int id,
                      void (*handler)(void*), void *arg=nullptr) noexcept;
#endif

/**
 * Unregister an interrupt handler.
 * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param handler pointer to the currently registered handler function of type
 * void (*)()
 *
 * \note This function calls errorHandler() causing a reboot if attempting to
 * unregister a different interrupt than the currently registered one
 */
inline void IRQunregisterIrq(GlobalIrqLock& lock, unsigned int id, void (*handler)()) noexcept
{
    IRQunregisterIrq(lock,id,reinterpret_cast<void (*)(void*)>(handler));
}

/**
 * Unregister an interrupt handler.
 * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
 * \param id platform-dependent id of the peripheral for which the handler has
 * to be registered.
 * \param mfn member function pointer to the class method that has been
 * previously registered as interrupt handler.
 * \param object class instance previously specified to be used for invoking the
 * member function.
 *
 * \note This function calls errorHandler() causing a reboot if attempting to
 * unregister a different interrupt than the currently registered one
 */
template<typename T>
inline void IRQunregisterIrq(GlobalIrqLock& lock, unsigned int id, void (T::*mfn)(), T *object) noexcept
{
    auto result=unmember(mfn,object);
    IRQunregisterIrq(lock,id,std::get<0>(result),std::get<1>(result));
}

/**
 * Check whether an interrupt handler is currently registered.
 * \param id platform-dependent id of the peripheral for which to check whether
 * an interrupt handler is registered.
 * \return true if an interrupt hander is currently registered for that id.
 */
bool IRQisIrqRegistered(unsigned int id) noexcept;

/**
 * \}
 */

} //namespace miosix

/**
 * \}
 */

#include "interfaces-impl/interrupts_impl.h"
