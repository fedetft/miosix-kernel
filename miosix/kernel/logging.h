/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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

#ifndef LOGGING_H
#define	LOGGING_H

#include "config/miosix_settings.h"
#include "interfaces/console.h"

namespace miosix {

/**
 * Print boot logs on the Console. Can only be called when the kernel is
 * running.
 * \param string to print
 */
inline void bootlog(const char *string)
{
    #ifdef WITH_BOOTLOG
    Console::write(string);
    #endif //WITH_BOOTLOG
}

/**
 * Print boot logs on the Console. Can only be called when the kernel is
 * not yet running or paused, or within an IRQ.
 * \param string to print
 */
inline void IRQbootlog(const char *string)
{
    #ifdef WITH_BOOTLOG
    Console::IRQwrite(string);
    #endif //WITH_BOOTLOG
}

/**
 * Print eror logs on the Console. Can only be called when the kernel is
 * running.
 * \param string to print
 */
inline void errorLog(const char *string)
{
    #ifdef WITH_ERRLOG
    Console::write(string);
    #endif //WITH_ERRLOG
}

/**
 * Print error logs on the Console. Can only be called when the kernel is
 * not yet running or paused, or within an IRQ.
 * \param string to print
 */
inline void IRQerrorLog(const char *string)
{
    #ifdef WITH_ERRLOG
    Console::IRQwrite(string);
    #endif //WITH_ERRLOG
}

} //namespace miosix

#endif	/* LOGGING_H */
