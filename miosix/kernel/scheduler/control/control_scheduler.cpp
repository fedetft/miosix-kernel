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

#include "control_scheduler.h"
#include "kernel/error.h"
#include <limits>

using namespace std;

#ifdef SCHED_TYPE_CONTROL_BASED

namespace miosix {

//These are defined in kernel.cpp
extern volatile Thread *cur;
extern unsigned char kernel_running;

//
// class ControlScheduler
//

bool ControlScheduler::PKaddThread(Thread *thread,
        ControlSchedulerPriority priority)
{
    #ifdef SCHED_CONTROL_FIXED_POINT
    if(threadList.size()>=64) return false;
    #endif //SCHED_CONTROL_FIXED_POINT
    thread->schedData.priority=priority;
    threadList.push_front(thread);
    SP_Tr+=bNominal; //One thread more, increase round time
    {
        //Note: can't use FastInterruptDisableLock here since this code is
        //also called *before* the kernel is started.
        //Using FastInterruptDisableLock would enable interrupts prematurely
        //and cause all sorts of misterious crashes
        InterruptDisableLock dLock;
        IRQrecalculateAlfa();
    }
    return true;
}

bool ControlScheduler::PKexists(Thread *thread)
{
    list<Thread *>::iterator it;
    it=find(threadList.begin(),threadList.end(),thread);
    if(it==threadList.end()) return false;
    if((*it)->flags.isDeleted()) return false;
    return true;
}

void ControlScheduler::PKremoveDeadThreads()
{
    for(list<Thread *>::iterator it=threadList.begin();it!=threadList.end();++it)
    {
        if((*it)->flags.isDeleted())
        {
            void *base=(*it)->watermark;
            (*it)->~Thread();
            free(base); //Delete ALL thread memory
            *it=0; //Mark with null, remove later
            SP_Tr-=bNominal; //One thread less, reduce round time
        }
    }
    threadList.remove(0); //Remove NULLs
    {
        FastInterruptDisableLock dLock;
        IRQrecalculateAlfa();
    }
}

void ControlScheduler::PKsetPriority(Thread *thread,
        ControlSchedulerPriority newPriority)
{
    thread->schedData.priority=newPriority;
    {
        FastInterruptDisableLock dLock;
        IRQrecalculateAlfa();
    }
}

void ControlScheduler::IRQsetIdleThread(Thread *idleThread)
{
    idleThread->schedData.priority=-1;
    idle=idleThread;
    //Initializing curInRound to end() so that the first time
    //IRQfindNextThread() is called the scheduling algorithm runs
    if(threadList.size()!=1) errorHandler(UNEXPECTED);
    curInRound=threadList.end();
}

Thread *ControlScheduler::IRQgetIdleThread()
{
    return idle;
}

void ControlScheduler::IRQfindNextThread()
{
    // Warning: since this function is called within interrupt routines, it
    //is not possible to add/remove elements to threadList, since that would
    //require dynamic memory allocation/deallocation which is forbidden within
    //interrupts. Iterating the list is safe, though

    if(kernel_running!=0) return;//If kernel is paused, do nothing

    if(cur!=idle)
    {
        //Not preempting from the idle thread, store actual burst time of
        //the preempted thread
        int Tp=miosix_private::AuxiliaryTimer::IRQgetValue();
        cur->schedData.Tp=Tp;
        Tr+=Tp;
    }

    //Find next thread to run
    for(;;)
    {
        if(curInRound!=threadList.end()) ++curInRound;
        if(curInRound==threadList.end()) //Note: do not replace with an else
        {
            //Check these two statements:
            //- If all threads are not ready, the scheduling algorithm must be
            //  paused and the idle thread is run instead
            //- If the inner integral regulator of all ready threads saturated
            //  then the integral regulator of the outer regulator must stop
            //  increasing because the set point cannot be attained anyway.
            bool allThreadNotReady=true;
            bool allReadyThreadsSaturated=true;
            list<Thread *>::iterator it;
            for(it=threadList.begin();it!=threadList.end();++it)
            {
                if((*it)->flags.isReady())
                {
                    allThreadNotReady=false;
                    if((*it)->schedData.bo<bMax*multFactor)
                    {
                        allReadyThreadsSaturated=false;
                        //Found a counterexample for both statements,
                        //no need to scan the list further.
                        break;
                    }
                }
            }
            if(allThreadNotReady)
            {
                //No thread is ready, run the idle thread

                //This is very important: the idle thread can *remove* dead
                //threads from threadList, so it can invalidate iterators
                //to any element except theadList.end()
                curInRound=threadList.end();
                cur=idle;
                ctxsave=cur->ctxsave;
                miosix_private::AuxiliaryTimer::IRQsetValue(bIdle);
                return;
            }

            //End of round reached, run scheduling algorithm
            curInRound=threadList.begin();
            IRQrunRegulator(allReadyThreadsSaturated);
        }

        if((*curInRound)->flags.isReady())
        {
            //Found a READY thread, so run this one
            cur=*curInRound;
            ctxsave=cur->ctxsave;
            miosix_private::AuxiliaryTimer::IRQsetValue(
                    (*curInRound)->schedData.bo/multFactor);
            return;
        } else {
            //If we get here we have a non ready thread that cannot run,
            //so regardless of the burst calculated by the scheduler
            //we do not run it and set Tp to zero.
            (*curInRound)->schedData.Tp=0;
        }
    }
}

void ControlScheduler::IRQrecalculateAlfa()
{
    //Sum of all priorities of all threads
    //Note that since priority goes from 0 to PRIORITY_MAX-1
    //but priorities we need go from 1 to PRIORITY_MAX we need to add one
    unsigned int sumPriority=0;
    for(list<Thread *>::iterator it=threadList.begin();it!=threadList.end();++it)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Count only ready threads
        if((*it)->flags.isReady())
            sumPriority+=(*it)->schedData.priority.get()+1;//Add one
        #else //ENABLE_FEEDFORWARD
        //Count all threads
        sumPriority+=(*it)->schedData.priority.get()+1;//Add one
        #endif //ENABLE_FEEDFORWARD
    }
    //This can happen when ENABLE_FEEDFORWARD is set and no thread is ready
    if(sumPriority==0) return;
    #ifndef SCHED_CONTROL_FIXED_POINT
    float base=1.0f/((float)sumPriority);
    for(list<Thread *>::iterator it=threadList.begin();it!=threadList.end();++it)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Assign zero bursts to blocked threads
        if((*it)->flags.isReady())
        {
            (*it)->schedData.alfa=base*((float)((*it)->schedData.priority.get()+1));
        } else {
            (*it)->schedData.alfa=0;
        }
        #else //ENABLE_FEEDFORWARD
        //Assign bursts irrespective of thread blocking status
        (*it)->schedData.alfa=base*((float)((*it)->schedData.priority.get()+1));
        #endif //ENABLE_FEEDFORWARD
    }
    #else //FIXED_POINT_MATH
    //Sum of all alfa is maximum value for an unsigned short
    unsigned int base=4096/sumPriority;
    for(list<Thread *>::iterator it=threadList.begin();it!=threadList.end();++it)
    {
        #ifdef ENABLE_FEEDFORWARD
        //Assign zero bursts to blocked threads
        if((*it)->flags.isReady())
        {
            (*it)->schedData.alfa=base*((*it)->schedData.priority.get()+1);
        } else {
            (*it)->schedData.alfa=0;
        }
        #else //ENABLE_FEEDFORWARD
        //Assign bursts irrespective of thread blocking status
        (*it)->schedData.alfa=base*((*it)->schedData.priority.get()+1);
        #endif //ENABLE_FEEDFORWARD
    }
    #endif //FIXED_POINT_MATH
    reinitRegulator=true;
}

std::list<Thread *> ControlScheduler::threadList;
std::list<Thread *>::iterator ControlScheduler::curInRound;
Thread *ControlScheduler::idle=0;
int ControlScheduler::SP_Tr=0;
int ControlScheduler::Tr=bNominal;
int ControlScheduler::bco=0;
int ControlScheduler::eTro=0;
bool ControlScheduler::reinitRegulator=false;

} //namespace miosix

#endif //SCHED_TYPE_CONTROL_BASED
