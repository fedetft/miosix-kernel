/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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

#include "error.h"
#include "config/miosix_settings.h"
#include "lock.h"
#include "interfaces/poweroff.h"
#include "interfaces_private/smp.h"
#include "logging.h"

namespace miosix {

void errorHandler(Error e)
{
    // Disable interrupts
    fastDisableIrq();
    #ifdef WITH_SMP
    // On multicore try to make the other cores hang up. Do NOT take the GIL,
    // as it could cause a deadlock if it is already taken by this core, or any
    // other ones if they are waiting for some peripheral interrupt that will
    // never happen. This is not strictly correct, of course, but this is an
    // emergency situation anyway.
    // The only real risk is corruption on the serial while logging.
    IRQlockupOtherCores();
    #endif
    
    //Unrecoverable errors
    switch(e)
    {
        case Error::UNEXPECTED:
            IRQerrorLog("\r\n***Unexpected error\r\n");
            break;
        case Error::OUT_OF_MEMORY:
            IRQerrorLog("\r\n***Out of memory\r\n");
            break;
        case Error::STACK_OVERFLOW:
            IRQerrorLog("\r\n***Stack overflow\r\n");
            break;
        case Error::MUTEX_ERROR:
            IRQerrorLog("\r\n***Mutex error\r\n");
            break;
        case Error::INTERRUPTS_ENABLED_AT_BOOT:
            IRQerrorLog("\r\n***Interrupts enabled at boot\r\n");
            break;
        case Error::INTERRUPT_REGISTRATION_ERROR:
            IRQerrorLog("\r\n***Interrupt registration error\r\n");
            break;
        default:
            break;
    }
    IRQsystemReboot();
}

} //namespace miosix
