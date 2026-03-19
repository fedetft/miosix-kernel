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

#include <cstdio>
#include <thread>
#include <deque>
#include "miosix.h"

using namespace std;
using namespace miosix;

constexpr unsigned int MAX_WORKERS=10;
constexpr long long SIM_DT=1000LL; // ms
constexpr int PERC_LOAD_PER_WORKER=(100*CPU_NUM_CORES)/MAX_WORKERS;

void *worker(void *)
{
    while(!Thread::testTerminate())
    {
        Thread::sleep(100-PERC_LOAD_PER_WORKER);
        delayMs(PERC_LOAD_PER_WORKER);
    }
    return nullptr;
}

int main()
{
    #ifdef WITH_CPU_TIME_COUNTER
    CPUProfiler p;
    #endif // WITH_CPU_TIME_COUNTER
    std::deque<Thread *> workers;

    /*
     * The idea is that we want to produce a segmented load graph like this:
     *
     * n. worker threads           
     * ^              /'-.
     * |      .-'\   /|   '-.
     * |   .-'  | \ / |      '-.
     * |.-'     |  |  |        |
     * +--------+--+-----------+> time
     * <-period-><-><---------->
     * 
     * A period is a interval of time where the number of worker threads is
     * monotonically increasing or decreasing with the same rate (called slope).
     * Each thread should take 10% of the total CPU time available in the
     * system, allowing to estimate the load that the CPU profiler should see.
     * The slope is scaled by a factor of 10 to provide a minimum of fractional
     * control over the process.
     * 
     * Note that when we terminate a thread, to the profiler their CPU usage
     * is completely lost. So it is *normal and not a bug* that after
     * a thread termination period (slope<0) the counter will return very low
     * load percentages with respect to the estimate.
     * 
     * Also, the counter's estimate does not take into account scheduler
     * behavior so it will be very rough.
     */
    int slope=-1,slopeAccum=0,periodSpent=0,periodLeft=0,threadDelta=0;
    int approxLoad=0;
    for(;;)
    {
        if(periodLeft<=0)
        {
            if(periodSpent>0)
            {
                #ifdef WITH_CPU_TIME_COUNTER
                p.update();
                p.print();
                #endif // WITH_CPU_TIME_COUNTER
                approxLoad/=periodSpent;
                iprintf("added (removed) %d threads, ", threadDelta);
                iprintf("expected total load %d%%\n\n", approxLoad);
            }
            int slopeMagnitude=rand()%19+2; // 0.2 ~ 2 new threads/interval
            int maxPeriod;
            // switch between adding and removing threads, unless we are
            // at the maximum/minimum number of workers.
            if((workers.size()>=MAX_WORKERS || slope>0) && workers.size()>0)
            {
                maxPeriod=workers.size()*slopeMagnitude;
                slope=-slopeMagnitude;
            } else {
                maxPeriod=(MAX_WORKERS-workers.size())*slopeMagnitude;
                slope=slopeMagnitude;
            }
            periodLeft=((rand()%maxPeriod)/10)+1;
            periodSpent=0;
            threadDelta=0;
            approxLoad=0;
            //iprintf("slope=%d period=%d\n",slope,period);
        } else {
            //iprintf("slopeAccum=%d, period=%d\n",slopeAccum,period);
        }
        slopeAccum+=slope;
        bool earlyStop=false;
        while(slopeAccum>=5)
        {
            if(workers.size()>=MAX_WORKERS) { earlyStop=true; break; }
            Thread *newWorker=Thread::create(worker,STACK_MIN,DEFAULT_PRIORITY,
                                             nullptr,Thread::DETACHED);
            workers.push_back(newWorker);
            iprintf("+ %p\n", newWorker);
            threadDelta++;
            slopeAccum-=10;
        }
        while(slopeAccum<-5)
        {
            if(workers.size()==0) { earlyStop=true; break; }
            Thread *restingWorker=workers[0];
            workers.pop_front();
            restingWorker->terminate();
            iprintf("- %p\n", restingWorker);
            threadDelta--;
            slopeAccum+=10;
        }
        if(earlyStop)
        {
            periodLeft=0;
        } else {
            Thread::sleep(SIM_DT);
            approxLoad+=PERC_LOAD_PER_WORKER*workers.size();
            periodLeft--;
            periodSpent++;
        }
    }
}
