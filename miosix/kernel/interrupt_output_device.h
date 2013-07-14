/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

#ifndef INTERRUPT_OUTPUT_DEVICE_H
#define	INTERRUPT_OUTPUT_DEVICE_H

#include "intrusive.h"

namespace miosix {

/**
 * Used by the kernel to write debug information before the kernel is started,
 * or in case of serious errors, right before rebooting.
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class InterruptOutputDevice : public IntrusiveRefCounted
{
public:
    /**
     * Constructor
     */
    InterruptOutputDevice() {}
    
    /**
     * Write a string to the Console.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * \param str the string to write. The string must be NUL terminated.
     */
    virtual void IRQwrite(const char *str)=0;
    
    /**
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * Since the implementation of the Console class can use buffering, this
     * memeber function is provided to know if all data has been sent, for
     * example to wait until all data has been sent before performing a reboot.
     * \return true if all write buffers are empty.
     */
    virtual bool IRQtxComplete()=0;
    
    /**
     * Destructor
     */
    virtual ~InterruptOutputDevice();
    
    /**
     * \return an instance of the currently installed output device
     */
    static intrusive_ref_ptr<InterruptOutputDevice> IRQget();
    
    /**
     * Install a new output device
     * \param device new device
     */
    static void setDevice(intrusive_ref_ptr<InterruptOutputDevice> device);

private:
    InterruptOutputDevice(const InterruptOutputDevice&);
    InterruptOutputDevice& operator= (const InterruptOutputDevice&);
    
    /**
     * \return a pointer to the currently installed output device
     */
    static intrusive_ref_ptr<InterruptOutputDevice> *getDevice();
};

/**
 * Dummy device that ignores all writes
 */
class NullInterruptOutputDevice
{
public:
    /**
     * Write a string to the Console.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * \param str the string to write. The string must be NUL terminated.
     */
    virtual void IRQwrite(const char *str);
    
    /**
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt.
     * Since the implementation of the Console class can use buffering, this
     * memeber function is provided to know if all data has been sent, for
     * example to wait until all data has been sent before performing a reboot.
     * \return true if all write buffers are empty.
     */
    virtual bool IRQtxComplete();
};

} //namespace miosix

#endif //INTERRUPT_OUTPUT_DEVICE_H
