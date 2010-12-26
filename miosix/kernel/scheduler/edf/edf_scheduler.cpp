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

#include "edf_scheduler.h"
#include "kernel/kernel.h"
#include "kernel/error.h"
#include "interfaces-impl/bsp_impl.h"
#include "interfaces/console.h"
#include <algorithm>

using namespace std;

#ifdef SCHED_TYPE_EDF

namespace miosix {

//These are defined in kernel.cpp
extern volatile Thread *cur;
extern unsigned char kernel_running;

//
// class EDFSchedulerPriority
//

bool EDFSchedulerPriority::validate() const
{
    return this->deadline>=0;
}

//
// class EDFScheduler
//

void EDFScheduler::PKaddThread(Thread *thread, EDFSchedulerPriority priority)
{

}

bool EDFScheduler::PKexists(Thread *thread)
{

}

void EDFScheduler::PKremoveDeadThreads()
{

}

void EDFScheduler::PKsetPriority(Thread *thread,
        EDFSchedulerPriority newPriority)
{

}

EDFSchedulerPriority EDFScheduler::getPriority(Thread *thread)
{
    return thread->schedData.deadline;
}


EDFSchedulerPriority EDFScheduler::IRQgetPriority(Thread *thread)
{
    return thread->schedData.deadline;
}

void EDFScheduler::IRQsetIdleThread(Thread *idleThread)
{

}

void EDFScheduler::IRQfindNextThread()
{

}

} //namespace miosix

#endif //SCHED_TYPE_EDF
