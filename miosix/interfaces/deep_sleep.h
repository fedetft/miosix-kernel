/***************************************************************************
 *   Copyright (C) 2013, 2014, 2015 by Terraneo Federico                   *
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

#ifndef DEEP_SLEEP_H
#define DEEP_SLEEP_H 

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file deep_sleep.h
 * This file contains required functions to implement automatic deep sleep state
 * switch.
 * 
 * This solution on supported hardware allows to power off also the peripherals when the system
 * is in idle state and doens't require any peripheral action.
 */

namespace miosix {

/** 
 * \param abstime : selected absolute time to wake up from deep sleep state. At this time the interrupt
 * from RTC will be executed
**/
void IRQdeepSleep(long long abstime);

} //namespace miosix

#endif DEEP_SLEEP_H
