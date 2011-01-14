/***************************************************************************
 *   Copyright (C) 2010, 2011 by Terraneo Federico                         *
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

#ifndef PARAMETERS_H
#define	PARAMETERS_H

namespace miosix {

//
// Parameters for the control based scheduler
//

const float kpi=0.5;
const float krr=0.9;//1.4f;
const float zrr=0.88f;

///Implementation detail resulting from a fixed point implementation of the
///inner integral regulators. Never change this, change kpi instead.
const int multFactor=static_cast<int>(1.0f/kpi);

///Instead of fixing a round time the current policy is to have
///roundTime=bNominal * numThreads, where bNominal is the nominal thread burst
static const int bNominal=static_cast<int>(AUX_TIMER_CLOCK*0.004);// 4ms

///minimum burst time (to avoid inefficiency caused by context switch
///time longer than burst time)
static const int bMin=static_cast<int>(AUX_TIMER_CLOCK*0.0002);// 200us

///maximum burst time (to avoid losing responsiveness/timer overflow)
static const int bMax=static_cast<int>(AUX_TIMER_CLOCK*0.02);// 20ms

///idle thread has a fixed burst length that can be modified here
///it is recomended to leave it to a high value, but that does not cause
///overflow of the auxiliary timer
static const int bIdle=static_cast<int>(AUX_TIMER_CLOCK*0.5);// 500ms

}

#endif //PARAMETERS_H
