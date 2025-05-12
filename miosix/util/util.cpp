/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013 by Terraneo Federico *
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

/*
 * Part of Miosix Embedded OS
 * some utilities
 */
#include <cstdio>
#include <malloc.h>
#include "util.h"
#include "kernel/lock.h"
#include "kernel/thread.h"
#include "kercalls/libc_integration.h"
#include "config/miosix_settings.h" //For WATERMARK_FILL and STACK_FILL
#include "interfaces/cpu_const.h"

using namespace std;

namespace miosix {

//
// MemoryProfiling class
//

void MemoryProfiling::print()
{
    unsigned int curFreeStack=getCurrentFreeStack();
    unsigned int absFreeStack=getAbsoluteFreeStack();
    unsigned int stackSize=getStackSize();
    unsigned int curFreeHeap=getCurrentFreeHeap();
    unsigned int absFreeHeap=getAbsoluteFreeHeap();
    unsigned int heapSize=getHeapSize();

    iprintf("Stack memory statistics.\n"
            "Size: %u\n"
            "Used (current/max): %u/%u\n"
            "Free (current/min): %u/%u\n"
            "Heap memory statistics.\n"
            "Size: %u\n"
            "Used (current/max): %u/%u\n"
            "Free (current/min): %u/%u\n",
            stackSize,stackSize-curFreeStack,stackSize-absFreeStack,
            curFreeStack,absFreeStack,
            heapSize,heapSize-curFreeHeap,heapSize-absFreeHeap,
            curFreeHeap,absFreeHeap);
}

unsigned int MemoryProfiling::getStackSize()
{
    return Thread::getStackSize();
}

unsigned int MemoryProfiling::getAbsoluteFreeStack()
{
    const unsigned int *walk=Thread::getStackBottom();
    const unsigned int stackSize=Thread::getStackSize();
    unsigned int count=0;
    while(count<stackSize && *walk==STACK_FILL)
    {
        //Count unused stack
        walk++;
        count+=4;
    }
    //This takes in account CTXSAVE_ON_STACK. It might underestimate
    //the absolute free stack (by a maximum of CTXSAVE_ON_STACK) but
    //it will never overestimate it, which is important since this
    //member function can be used to select stack sizes.
    if(count<=CTXSAVE_ON_STACK) return 0;
    return count-CTXSAVE_ON_STACK;
}

unsigned int MemoryProfiling::getCurrentFreeStack()
{
    #ifdef __ARM_EABI__
    void *stack_ptr;
    asm volatile("mov %0, sp" : "=r"(stack_ptr));
    #else //__ARM_EABI__
    //NOTE: __builtin_frame_address doesn't add the size of the current function
    //frame, likely __builtin_stack_address would be better but GCC 9.2.0 does
    //not have it
    void *stack_ptr=__builtin_frame_address(0);
    #endif //__ARM_EABI__
    const unsigned int *walk=Thread::getStackBottom();
    unsigned int freeStack=(reinterpret_cast<unsigned int>(stack_ptr)
                          - reinterpret_cast<unsigned int>(walk));
    //This takes into account CTXSAVE_ON_STACK.
    if(freeStack<=CTXSAVE_ON_STACK) return 0;
    return freeStack-CTXSAVE_ON_STACK;
}

unsigned int MemoryProfiling::getHeapSize()
{
    //These extern variables are defined in the linker script
    //Pointer to begin of heap
    extern char _end asm("_end");
    //Pointer to end of heap
    extern char _heap_end asm("_heap_end");

    return reinterpret_cast<unsigned int>(&_heap_end)
         - reinterpret_cast<unsigned int>(&_end);
}

unsigned int MemoryProfiling::getAbsoluteFreeHeap()
{
    //This extern variable is defined in the linker script
    //Pointer to end of heap
    extern char _heap_end asm("_heap_end");

    unsigned int maxHeap=getMaxHeap();

    return reinterpret_cast<unsigned int>(&_heap_end) - maxHeap;
}

unsigned int MemoryProfiling::getCurrentFreeHeap()
{
    struct mallinfo mallocData=_mallinfo_r(__getreent());
    return getHeapSize()-mallocData.uordblks;
}

char *formatHex(char *out, unsigned long n, unsigned int len)
{
    unsigned int i=len;
    while(i--)
    {
        unsigned long digit=n&0xF; n>>=4;
        if(digit<10) out[i]=digit+'0';
        else out[i]=(digit-10)+'a';
    }
    return out+len;
}

/**
 * \internal
 * used by memDump
 */
static void memPrint(const char *data, char len)
{
    char buffer[79+1];
    char *p=buffer;
    *p++='0'; *p++='x';
    p=formatHex(p,reinterpret_cast<unsigned int>(data),8);
    *p++=' ';
    for(int i=0;i<len;i++)
    {
        p=formatHex(p,static_cast<unsigned char>(data[i]),2);
        *p++=' ';
    }
    for(int i=0;i<(16-len)*3;i++) *p++=' ';
    *p++='|'; *p++=' ';
    for(int i=0;i<len;i++)
    {
        if((data[i]>=32)&&(data[i]<127)) *p++=data[i];
        else *p++='.';
    }
    *p++='\0';
    puts(buffer);
}

void memDump(const void *start, int len)
{
    const char *data=reinterpret_cast<const char*>(start);
    while(len>16)
    {
        memPrint(data,16);
        len-=16;
        data+=16;
    }
    if(len>0) memPrint(data,len);
}

#ifdef WITH_CPU_TIME_COUNTER

static long long printSingleThreadInfo(long long approxDt,
    CPUTimeCounter::Data *oldData, CPUTimeCounter::Data *newData,
    long long allThdDelta[CPU_NUM_CORES])
{
    if(!newData)
    {
        iprintf("%p killed\n", oldData->thread);
        return 0;
    } else {
        Thread *thread=newData->thread;
        int perc[CPU_NUM_CORES];
        long long allCpuDelta=0;
        if(oldData)
        {
            bool revived=false;
            for(unsigned char i=0;i<CPU_NUM_CORES;i++)
            {
                long long td;
                if(newData->usedCpuTime[i]>=oldData->usedCpuTime[i])
                {
                    td=newData->usedCpuTime[i]-oldData->usedCpuTime[i];
                } else {
                    // CPU time is incrementing. If it doesn't, a thread was
                    // killed, and then another one created immediately after,
                    // and the two thread pointers are coincidentially the same.
                    // This is rare but not impossible!
                    td=newData->usedCpuTime[i];
                    revived=true;
                }
                allThdDelta[i]+=td;
                perc[i]=static_cast<int>(td>>16)*100/approxDt;
                allCpuDelta+=td;
            }
            if(revived) iprintf("%p killed\n", oldData->thread);
        } else {
            for(unsigned char i=0;i<CPU_NUM_CORES;i++)
            {
                long long td=newData->usedCpuTime[i];
                allThdDelta[i]+=td;
                perc[i]=static_cast<int>(td>>16)*100/approxDt;
                allCpuDelta+=td;
            }
        }
        iprintf("%p %c %10lld %2d.%1d%%",thread,newData->state,allCpuDelta,
                                         perc[0]/10,perc[0]%10);
        for(unsigned char i=1;i<CPU_NUM_CORES;i++)
        {
            iprintf(" %2d.%1d%%",perc[i]/10,perc[i]%10);
        }
        if(!oldData) iprintf(" new");
        iprintf("\n");
        return allCpuDelta;
    }
}

//
// CPUProfiler class
//

long long CPUProfiler::update()
{
    lastSnapshotIndex ^= 1;
    snapshots[lastSnapshotIndex].collect();
    return snapshots[lastSnapshotIndex].time;
}

void CPUProfiler::print()
{
    Snapshot& oldSnap = snapshots[lastSnapshotIndex ^ 1];
    Snapshot& newSnap = snapshots[lastSnapshotIndex];
    std::vector<CPUTimeCounter::Data>& oldInfo = oldSnap.threadData;
    std::vector<CPUTimeCounter::Data>& newInfo = newSnap.threadData;
    long long dt = newSnap.time - oldSnap.time;
    int approxDt = static_cast<int>(dt >> 16) / 10;
    long long allThdDelta[CPU_NUM_CORES]={0};

    iprintf("%d threads, last interval %lld ns\n", newInfo.size(), dt);
    iprintf("%10s S %10s %5s","TID","time [ns]","cpu 0");
    for(int i=1;i<CPU_NUM_CORES;i++) iprintf(" cpu%2d",i);
    iprintf("\n");

    // Compute the difference between oldInfo and newInfo
    auto oldIt = oldInfo.begin();
    auto newIt = newInfo.begin();
    while(newIt!=newInfo.end() && oldIt!=oldInfo.end())
    {
        // Skip old threads that were killed
        while(oldIt!=oldInfo.end() && newIt->thread!=oldIt->thread)
        {
            printSingleThreadInfo(approxDt,&(*oldIt),nullptr,allThdDelta);
            oldIt++;
        }
        if(oldIt!=oldInfo.end())
        {
            // Found a thread that exists in both lists
            printSingleThreadInfo(approxDt,&(*oldIt),&(*newIt),allThdDelta);
            newIt++;
            oldIt++;
        }
    }
    // Skip last killed threads
    while(oldIt != oldInfo.end())
    {
        printSingleThreadInfo(approxDt,&(*oldIt),nullptr,allThdDelta);
        oldIt++;
    }
    // Print info about newly created threads
    while(newIt != newInfo.end())
    {
        printSingleThreadInfo(approxDt,nullptr,&(*newIt),allThdDelta);
        newIt++;
    }
    // Print global stats
    iprintf("%-23s", "Total load");
    long long allCpuThdDelta=0;
    for(unsigned char i=0;i<CPU_NUM_CORES;i++)
    {
        allCpuThdDelta+=allThdDelta[i];
        int totalPerc=static_cast<int>(allThdDelta[i]>>16)*100/approxDt;
        iprintf(" %2d.%1d%%",totalPerc/10,totalPerc%10);
    }
    iprintf("\n");
    if(CPU_NUM_CORES>1)
    {
        int totalPerc=static_cast<int>(allCpuThdDelta>>16)*100/approxDt;
        iprintf("Total load (all cpus) %4d.%1d%%\n",totalPerc/10,totalPerc%10);
    }
}

void CPUProfiler::Snapshot::collect()
{
    // We cannot expand the threadData vector while the GIL is taken.
    //   Therefore we need to expand the vector earlier, lock the GIL, and
    // then check if the number of threads stayed the same. If it didn't,
    // we must unlock the GIL and try again. Otherwise we can fill the
    // vector.
    bool success = false;
    do {
        // Resize the vector with the current number of threads
        unsigned int nThreads = CPUTimeCounter::getThreadCount();
        this->threadData.resize(nThreads);
        {
            // Pause scheduling on all cores!
            FastGlobalIrqLock pLock;

            // If the number of threads changed, try again
            unsigned int nThreads2 = CPUTimeCounter::getThreadCount();
            if(nThreads2 != nThreads)
                continue;
            // Otherwise, stop trying
            success = true;

            // Get the current time now. This makes the time accurate with
            // respect to the data collected, at the cost of making the
            // update interval imprecise (if this timestamp is then used
            // to mantain the update interval)
            this->time = IRQgetTime();
            // Fetch the CPU time data for all threads
            auto i1 = this->threadData.begin();
            auto i2 = CPUTimeCounter::IRQbegin(this->time);
            do *i1++ = *i2++; while(i2 != CPUTimeCounter::IRQend());
        }
    } while(!success);
}

void CPUProfiler::thread(long long nsInterval)
{
    CPUProfiler profiler;
    long long t = profiler.update();
    while(!Thread::testTerminate())
    {
        t += nsInterval;
        Thread::nanoSleepUntil(t);
        profiler.update();
        profiler.print();
        iprintf("\n");
    }
}

#endif // WITH_CPU_TIME_COUNTER

} //namespace miosix
