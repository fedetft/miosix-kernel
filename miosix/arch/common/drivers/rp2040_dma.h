/***************************************************************************
 *   Copyright (C) 2025 by Daniele Cattaneo                                *
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

#include "interfaces/arch_registers.h"
#include "kernel/lock.h"

namespace miosix {

/**
 * Class for reserving DMA channels and registering interrupt handlers on the
 * RP2040.
 * 
 * The RP2040 DMA peripheral is very good because all channels are the same.
 * But it's also very bad because all channels share the same interrupt handler
 * -- well, not exactly; two interrupt handlers in fact.
 * So the simple option for using DMA -- i.e. hardcoding channels -- is a bit of
 * a mess because you can't hardcode the interrupt handler ID as in DMAs with
 * different interrupt handlers per channel.
 * This class resolves the interrupt handling problem and also allows to share
 * the DMA peripheral more easily.
 * 
 * Note that individual peripheral drivers are trusted to not touch any
 * DMA channel they do not own, and must configure the DMA peripheral manually
 * There is no abstraction of the DMA peripheral here, just the resource
 * management bit.
 */
class RP2040Dma
{
public:
    /**
     * Reserve a DMA channel and registers an interrupt handler for it.
     * \param lock a GlobalIrqLock (GIL) lock that must be already taken here
     * \param handler pointer to the handler function of type void (*)(void*).
     * The handler will be called with the GIL already fully locked (this is
     * different than what is required in handlers registered by IRQregisterIrq)
     * \param arg optional void* argument. This argument is stored in the
     * interrupt handling logic and passed as-is whenever the interrupt handler
     * is called. If omitted, the handler function is called with nullptr as
     * argument.
     * \returns the ID of the DMA channel that has been reserved.
     * 
     * \note This function calls errorHandler() causing a reboot if attempting
     * to reserve more DMA channels than there are available.
     */
    static unsigned int IRQregisterChannel(GlobalIrqLock& lock, 
            void (*handler)(void*), void *arg=nullptr) noexcept;

    /**
     * Reserve a DMA channel and registers a class member function as its
     * interrupt handler.
     * \param lock a GlobalIrqLock (GIL) lock that must be already taken here
     * \param mfn member function pointer to the class method to be registered
     * as interrupt handler. The method shall take no parameters.
     * The handler will be called with the GIL already fully locked (this is
     * different than what is required in handlers registered by IRQregisterIrq)
     * \param object class instance whose methods shall be called as interrupt
     * handler.
     * \returns the ID of the DMA channel that has been reserved.
     * 
     * \note This function calls errorHandler() causing a reboot if attempting
     * to reserve more DMA channels than there are available.
     */
    template<typename T>
    static unsigned int IRQregisterChannel(GlobalIrqLock& lock,
            void (T::*mfn)(), T *object) noexcept
    {
        auto res=unmember(mfn,object);
        return IRQregisterChannel(lock,std::get<0>(res),std::get<1>(res));
    }

    /**
     * Free a DMA channel and unregisters its interrupt handler.
     * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
     * \param channel DMA channel identifier to be freed.
     * \param handler pointer to the currently registered handler function of
     * type void (*)(void*)
     * \param arg optional void* argument previously specified when registering
     * the handler.
     *
     * \note This function calls errorHandler() causing a reboot if attempting
     * to free a DMA channel that has not been reserved, or if the channel's
     * interrupt handler is different than the one currently registered.
     */
    static void IRQunregisterChannel(GlobalIrqLock& lock, unsigned int channel,
            void (*handler)(void*), void *arg=nullptr) noexcept;

    /**
     * Free a DMA channel and unregisters its interrupt handler.
     * \param lock a GlobalIrqLock (GIL) lock that must be already taken here.
     * \param channel DMA channel identifier to be freed.
     * \param mfn member function pointer to the class method that has been
     * previously registered as interrupt handler.
     * \param object class instance previously specified to be used for invoking
     * the member function.
     *
     * \note This function calls errorHandler() causing a reboot if attempting
     * to free a DMA channel that has not been reserved, or if the channel's
     * interrupt handler is different than the one currently registered.
     */
    template<typename T>
    static void IRQunregisterChannel(GlobalIrqLock& lock, unsigned int channel,
            void (T::*mfn)(), T *object) noexcept
    {
        auto res=unmember(mfn,object);
        IRQunregisterChannel(lock,channel,std::get<0>(res),std::get<1>(res));
    }

private:
    /// \internal Lazy initializer for the DMA reservation table
    static void IRQinitialize(GlobalIrqLock& lock);

    /// \internal Common interrupt handler for all channels
    template<unsigned int IrqId>
    static void IRQinterruptHandler();

    static bool initialized;
    static constexpr unsigned int numChannels=12;
    static unsigned int irqAllocMask;
    struct DmaIrqEntry
    {
        void (*handler)(void *);
        void *arg;
    };
    static DmaIrqEntry irqEntries[numChannels];
};

} // namespace miosix
