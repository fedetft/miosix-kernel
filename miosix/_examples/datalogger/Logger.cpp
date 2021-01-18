/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <interfaces/atomic_ops.h>
#include <tscpp/buffer.h>
#include "Logger.h"

using namespace std;
using namespace std::chrono;
using namespace miosix;
using namespace tscpp;

//
// class Logger
//

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::start()
{
    if (started)
        return;

    char filename[32];
    for (unsigned int i = 0; i < filenameMaxRetry; i++)
    {
        sprintf(filename, "/sd/%02d.dat", i);
        struct stat st;
        if (stat(filename, &st) != 0)
            break;
        // File exists
        if (i == filenameMaxRetry - 1)
            puts("Too many files, appending to last");
    }

    file = fopen(filename, "ab");
    if (file == NULL)
        throw runtime_error("Error opening log file");
    setbuf(file, NULL);

    // The boring part, start threads one by one and if they fail, undo
    // Perhaps excessive defensive programming as thread creation failure is
    // highly unlikely (only if ram is full)

    packT = Thread::create(packThreadLauncher, 1536, 1, this, Thread::JOINABLE);
    if (!packT)
    {
        fclose(file);
        throw runtime_error("Error creating pack thread");
    }

    writeT =
        Thread::create(writeThreadLauncher, 2048, 1, this, Thread::JOINABLE);
    if (!writeT)
    {
        fullQueue.put(nullptr);  // Signal packThread to stop
        packT->join();
        // packThread has pushed a buffer and a nullptr to writeThread, remove
        // it
        while (fullList.front() != nullptr)
        {
            emptyList.push(fullList.front());
            fullList.pop();
        }
        fullList.pop();  // Remove nullptr
        fclose(file);
        throw runtime_error("Error creating write thread");
    }
    if(logStatsEnabled)
    {
        statsT =
            Thread::create(statsThreadLauncher, 1536, 1, this, Thread::JOINABLE);
        if (!statsT)
        {
            fullQueue.put(nullptr);  // Signal packThread to stop
            packT->join();
            writeT->join();
            fclose(file);
            throw runtime_error("Error creating stats thread");
        }
    }
    started = true;
}

void Logger::stop()
{
    if(started == false) return;
    if(logStatsEnabled) logStats();
    started = false;
    fullQueue.put(nullptr);  // Signal packThread to stop
    packT->join();
    writeT->join();
    if(logStatsEnabled) statsT->join();
    fclose(file);
}

Logger::Logger()
{
    // Allocate buffers and put them in the empty list
    for (unsigned int i = 0; i < numBuffers; i++)
        emptyList.push(new Buffer);
    for (unsigned int i = 0; i < numRecords; i++)
        emptyQueue.put(new Record);
}

void Logger::packThreadLauncher(void* argv)
{
    reinterpret_cast<Logger*>(argv)->packThread();
}

void Logger::writeThreadLauncher(void* argv)
{
    reinterpret_cast<Logger*>(argv)->writeThread();
}

void Logger::statsThreadLauncher(void* argv)
{
    reinterpret_cast<Logger*>(argv)->statsThread();
}

LogResult Logger::logImpl(const char* name, const void* data, unsigned int size)
{
    if (started == false)
        return LogResult::Ignored;

    Record* record = nullptr;
    {
        FastInterruptDisableLock dLock;
        // We disable interrupts because IRQget() is nonblocking, unlike get()
        if (emptyQueue.IRQget(record) == false)
        {
            s.statDroppedSamples++;
            return LogResult::Dropped;
        }
    }
    
    auto result = serializeImpl(record->data, maxRecordSize, name, data, size);
    if (result == BufferTooSmall)
    {
        emptyQueue.put(record);
        atomicAdd(&s.statTooLargeSamples, 1);
        return LogResult::TooLarge;
    }
    
    record->size = result;
    fullQueue.put(record);
    atomicAdd(&s.statQueuedSamples, 1);
    return LogResult::Queued;
}

void Logger::packThread()
{
    /*
     * The first implementation of this class had the log() function write
     * directly the serialized data to the buffers. So, no Records nor
     * packThread existed. However, to be able to call log() concurrently
     * without messing up the buffer, a mutex was needed. Thus, if many
     * threads call log(), contention on that mutex would occur, serializing
     * accesses and slowing down the (potentially real-time) callers. For this
     * reason Records and the pack thread were added.
     * Now each log() works independently on its own Record, and log() accesses
     * can proceed in parallel.
     */
    try
    {
        for (;;)
        {
            Buffer* buffer = nullptr;
            {
                Lock<FastMutex> l(mutex);
                // Get an empty buffer, wait if none is available
                while (emptyList.empty())
                    cond.wait(l);
                buffer = emptyList.front();
                emptyList.pop();
                buffer->size = 0;
            }

            do
            {
                Record* record = nullptr;
                fullQueue.get(record);

                // When stop() is called, it pushes a nullptr signaling to stop
                if (record == nullptr)
                {
                    Lock<FastMutex> l(mutex);
                    fullList.push(buffer);   // Don't lose the buffer
                    fullList.push(nullptr);  // Signal writeThread to stop
                    cond.broadcast();
                    s.statBufferFilled++;
                    return;
                }

                memcpy(buffer->data + buffer->size, record->data, record->size);
                buffer->size += record->size;
                emptyQueue.put(record);
            } while (bufferSize - buffer->size >= maxRecordSize);

            {
                Lock<FastMutex> l(mutex);
                // Put back full buffer
                fullList.push(buffer);
                cond.broadcast();
                s.statBufferFilled++;
            }
        }
    }
    catch (exception& e)
    {
        printf("Error: packThread failed due to an exception: %s\n", e.what());
    }
}

void Logger::writeThread()
{
    try
    {
        for (;;)
        {
            Buffer* buffer = nullptr;
            {
                Lock<FastMutex> l(mutex);
                // Get a full buffer, wait if none is available
                while (fullList.empty())
                    cond.wait(l);
                buffer = fullList.front();
                fullList.pop();
            }

            // When packThread stops, it pushes a nullptr signaling to stop
            if (buffer == nullptr)
                return;

            // Write data to disk
            auto t = system_clock::now();
//             ledOn();

            size_t result = fwrite(buffer->data, 1, buffer->size, file);
            if (result != buffer->size)
            {
                // If this fails and your board uses SDRAM,
                // define and increase OVERRIDE_SD_CLOCK_DIVIDER_MAX
                // perror("fwrite");
                s.statWriteFailed++;
            }
            else
                s.statBufferWritten++;

//             ledOff();
            auto d = system_clock::now() - t;
            s.statWriteTime    = duration_cast<milliseconds>(d).count();
            s.statMaxWriteTime = max(s.statMaxWriteTime, s.statWriteTime);

            {
                Lock<FastMutex> l(mutex);
                // Put back empty buffer
                emptyList.push(buffer);
                cond.broadcast();
            }
        }
    }
    catch (exception& e)
    {
        printf("Error: writeThread failed due to an exception: %s\n", e.what());
    }
}

/**
 * This thread prints stats
 */
void Logger::statsThread()
{
    if(logStatsEnabled)
    {
        try
        {
            for (;;)
            {
                this_thread::sleep_for(seconds(1));
                if (started == false)
                    return;
                logStats();
//                 printf("ls:%d ds:%d qs:%d bf:%d bw:%d wf:%d wt:%d mwt:%d\n",
//                        s.statTooLargeSamples, s.statDroppedSamples,
//                        s.statQueuedSamples, s.statBufferFilled, s.statBufferWritten,
//                        s.statWriteFailed, s.statWriteTime, s.statMaxWriteTime);
            }
        }
        catch (exception& e)
        {
            printf("Error: statsThread failed due to an exception: %s\n", e.what());
        }
    }
}
