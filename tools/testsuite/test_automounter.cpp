/***************************************************************************
 *   Copyright (C) 2026 by Lorenzo Pigato                                  *
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

#include "test_automounter.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "miosix.h"
#include "config/miosix_settings.h"
#include "interfaces/poweroff.h"
#include "filesystem/automounter/sd_automounter.h"

using namespace miosix;

#ifdef WITH_FILESYSTEM
#define AM_SENTINEL_DIR  "/sd/automounter_test"
#define AM_SENTINEL      AM_SENTINEL_DIR "/sentinel.txt"

namespace {

const char AM_SENTINEL_CONTENT[] = "miosix automounter sentinel\n";

constexpr unsigned int AM_TIMEOUT_MS   = 5000;
constexpr unsigned int AM_POLL_MS      = 100;
constexpr int          AM_DIR_MODE     = 0755;
constexpr unsigned int AM_SENTINEL_BUF = 64;
constexpr int          AM_FILE_MODE    = 0644;
constexpr int          AM_LOGIC_DEBOUNCE_N = 3;

constexpr unsigned int estThreadHeapUsage(unsigned int stack)
{
    return (((16 + 32 + stack + sizeof(Thread)) + 8) + 16);
}

// Common helpers shared by the logic tests and by the interactive hardware
// scenarios.
bool checkAvailHeap(unsigned int minimum)
{
    unsigned int free = MemoryProfiling::getCurrentFreeHeap();
    if(free < minimum)
    {
        iprintf("Skipping, low heap (%u<%u).\n", free, minimum);
        return false;
    }
    return true;
}

void testName(const char *name)
{
    iprintf("Testing %s... ", name);
    fflush(stdout);
}

void pass()
{
    iprintf("Ok.\n");
}

[[noreturn]] void fail(const char *cause)
{
    // Avoid stack-heavy printing here because some test threads are small.
    write(STDOUT_FILENO, "Failed:\n", 8);
    write(STDOUT_FILENO, cause, strlen(cause));
    write(STDOUT_FILENO, "\n", 1);
    reboot();
    for(;;) ;
}

const char *edgeName(SdAutomounterEdge edge)
{
    switch(edge)
    {
        case SdAutomounterEdge::None:     return "none";
        case SdAutomounterEdge::Inserted: return "inserted";
        case SdAutomounterEdge::Removed:  return "removed";
    }
    return "unknown";
}

bool askYesNo(const char *prompt)
{
    iprintf("%s [y/N] ", prompt);
    for(;;)
    {
        int c = getchar();
        if(c == '\n') continue;
        return c == 'y' || c == 'Y';
    }
}

void waitForAck(const char *prompt)
{
    iprintf("%s Type 'y' when done. ", prompt);
    for(;;)
    {
        int c = getchar();
        if(c == '\n') continue;
        if(c == 'y' || c == 'Y') return;
    }
}

bool isMounted()
{
    struct stat rootStat, sdStat;
    if(stat("/", &rootStat) != 0 || stat("/sd", &sdStat) != 0) return false;
    return rootStat.st_dev != sdStat.st_dev;
}

bool canReadSentinel()
{
    if(!isMounted()) return false;

    FILE *f = fopen(AM_SENTINEL, "rb");
    if(!f) return false;

    char buf[AM_SENTINEL_BUF];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return strcmp(buf, AM_SENTINEL_CONTENT) == 0;
}

bool ensureSentinel()
{
    if(!isMounted()) return false;

    mkdir(AM_SENTINEL_DIR, AM_DIR_MODE);
    if(canReadSentinel()) return true;

    FILE *f = fopen(AM_SENTINEL, "wb");
    if(!f) return false;

    size_t expected = strlen(AM_SENTINEL_CONTENT);
    bool ok = fwrite(AM_SENTINEL_CONTENT, 1, expected, f) == expected;
    ok = ok && fclose(f) == 0;
    return ok;
}

bool waitMounted(bool expected, unsigned int timeoutMs)
{
    for(unsigned int t = 0; t <= timeoutMs; t += AM_POLL_MS)
    {
        if(isMounted() == expected) return true;
        Thread::sleep(AM_POLL_MS);
    }
    return false;
}

bool waitSentinel(unsigned int timeoutMs)
{
    for(unsigned int t = 0; t <= timeoutMs; t += AM_POLL_MS)
    {
        if(isMounted() && ensureSentinel() && canReadSentinel())
            return true;
        Thread::sleep(AM_POLL_MS);
    }
    return false;
}

template<int requiredStableSamples>
void checkAdvance(const char *name,
                  SdAutomounterPollingState<requiredStableSamples> state,
                  const bool *samples,
                  const SdAutomounterEdge *expected,
                  unsigned int count,
                  bool expectedStable)
{
    testName(name);
    for(unsigned int i = 0; i < count; i++)
    {
        SdAutomounterEdge got = state.advance(samples[i]);
        if(got != expected[i])
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "sample %u: expected %s, got %s",
                     i, edgeName(expected[i]), edgeName(got));
            fail(buf);
        }
    }
    if(state.isPresent() != expectedStable)
        fail("final stablePresent mismatch");
    pass();
}

template<int requiredStableSamples>
SdAutomounterPollingState<requiredStableSamples> stableState(bool present)
{
    SdAutomounterPollingState<requiredStableSamples> state(present);
    if(present) state.advance(true);
    return state;
}

// Logic-only tests for the debouncing state machine.
void logicTest1()
{
    const bool samples[] = {true, true, true};
    const SdAutomounterEdge expected[] = {
        SdAutomounterEdge::Inserted,
        SdAutomounterEdge::None,
        SdAutomounterEdge::None};

    checkAdvance("[logic] [1] boot present",
        SdAutomounterPollingState<AM_LOGIC_DEBOUNCE_N>(true),
        samples, expected, 3, true);
}

void logicTest2()
{
    const bool samples[] = {true, false, true, true, true};
    const SdAutomounterEdge expected[] = {
        SdAutomounterEdge::None, SdAutomounterEdge::None,
        SdAutomounterEdge::None, SdAutomounterEdge::None,
        SdAutomounterEdge::Inserted};

    checkAdvance("[logic] [2] insert debounce",
        SdAutomounterPollingState<AM_LOGIC_DEBOUNCE_N>(false),
        samples, expected, 5, true);
}

void logicTest3()
{
    const bool samples[] = {false, true, false, false, false};
    const SdAutomounterEdge expected[] = {
        SdAutomounterEdge::None, SdAutomounterEdge::None,
        SdAutomounterEdge::None, SdAutomounterEdge::None,
        SdAutomounterEdge::Removed};

    SdAutomounterPollingState<AM_LOGIC_DEBOUNCE_N> running =
        stableState<AM_LOGIC_DEBOUNCE_N>(true);

    checkAdvance("[logic] [3] remove debounce",
        running, samples, expected, 5, false);
}

void logicTest4()
{
    const bool samples[] = {true, true, true};
    const SdAutomounterEdge expected[] = {
        SdAutomounterEdge::None, SdAutomounterEdge::None,
        SdAutomounterEdge::None};

    SdAutomounterPollingState<AM_LOGIC_DEBOUNCE_N> running =
        stableState<AM_LOGIC_DEBOUNCE_N>(true);

    checkAdvance("[logic] [4] no duplicate edges",
        running, samples, expected, 3, true);
}

void logicTest5()
{
    const bool samples[] = {true, false, true, false, true};
    const SdAutomounterEdge expected[] = {
        SdAutomounterEdge::None, SdAutomounterEdge::None,
        SdAutomounterEdge::None, SdAutomounterEdge::None,
        SdAutomounterEdge::None};

    checkAdvance("[logic] [5] glitch rejection",
        SdAutomounterPollingState<AM_LOGIC_DEBOUNCE_N>(false),
        samples, expected, 5, false);
}

// Interactive hardware tests for ordinary mount and unmount behavior.
void hwBootWithCard()
{
    testName("[hw] [1] boot with card");
    if(!waitSentinel(AM_TIMEOUT_MS))
        fail("card not mounted after boot");
    pass();
}

void hwInsertCard()
{
    testName("[hw] [2] insert card");
    if(isMounted())
        fail("already mounted - reboot without card to run this test");

    waitForAck("Insert the SD card now.");
    if(!waitSentinel(AM_TIMEOUT_MS))
        fail("card not mounted after insertion");
    pass();
}

void hwRemoveCard()
{
    testName("[hw] [3] remove card");
    if(!waitSentinel(AM_TIMEOUT_MS))
        fail("card not mounted before removal test");

    waitForAck("Remove the SD card now.");
    if(!waitMounted(false, AM_TIMEOUT_MS))
        fail("/sd still mounted after removal");
    if(canReadSentinel())
        fail("sentinel still readable after removal");
    pass();
}

void hwReinsertCard()
{
    testName("[hw] [4] reinsert card");
    waitForAck("Reinsert the SD card now.");
    if(!waitSentinel(AM_TIMEOUT_MS))
        fail("card not mounted after reinsertion");
    pass();
}

// Busy extraction is an extra test, isolated in its own internal namespace.
// It tests the scenario where the user removes the SD card while there are
// still files being actively used.
namespace busy {

constexpr unsigned int SCENARIO_TIMEOUT_MS = 15000;     // Max time allowed after removal.
constexpr unsigned int STALL_TIMEOUT_MS    = 2000;      // No-progress window treated as a stall.
constexpr unsigned int POLL_MS             = 50;        // Poll period used by the main test thread.
constexpr unsigned int WARMUP_TIMEOUT_MS   = 3000;      // Max time allowed to see initial I/O progress.
constexpr unsigned int WARMUP_PROGRESS     = 2;         // Minimum progress samples before removal.
constexpr unsigned int IO_CHUNK            = 512;       // I/O size used by the busy worker.
constexpr unsigned int PRECREATE_SIZE      = 64 * 1024; // Size of the prebuilt read file.
constexpr unsigned int WORKER_STACK        = 2048;      // Stack reserved for the busy worker thread.

const char READ_FILE[]  = AM_SENTINEL_DIR "/busy_read.bin";
const char WRITE_FILE[] = AM_SENTINEL_DIR "/busy_write.bin";

enum class WorkerMode
{
    SequentialRead, // Repeatedly read the same existing file in a loop.
    SyncWrite       // Append synchronously so writes reach the storage path.
};

// Shared state observed by the main test thread while a worker keeps the SD
// path busy. Only coarse progress and final status are tracked because the
// goal is to distinguish a normal I/O failure from a stall.
struct WorkerState
{
    std::atomic<unsigned int> progressCounter;
    std::atomic<long long> lastProgressNs;
    std::atomic<bool> finished;
    std::atomic<bool> sawIoError;
    std::atomic<int> lastError;

    WorkerState()
        : progressCounter(0), lastProgressNs(0), finished(false),
          sawIoError(false), lastError(0) {}
};

struct WorkerContext
{
    WorkerState *state;
    WorkerMode mode;
    const char *path;
};

struct Scenario
{
    const char *name;
    WorkerMode mode;
    const char *path;
    bool precreateFile;
    bool requireReinsertionAfter;
};

bool writeFull(int fd, const void *buf, size_t count)
{
    const unsigned char *ptr = reinterpret_cast<const unsigned char*>(buf);
    while(count > 0)
    {
        ssize_t written = write(fd, ptr, count);
        if(written < 0) return false;
        if(written == 0)
        {
            errno = EIO;
            return false;
        }
        ptr += written;
        count -= written;
    }
    return true;
}

bool preparePatternFile(const char *path, size_t size)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, AM_FILE_MODE);
    if(fd < 0) return false;

    unsigned char buffer[IO_CHUNK];
    for(unsigned int i = 0; i < sizeof(buffer); i++)
        buffer[i] = static_cast<unsigned char>(i & 0xff);

    size_t remaining = size;
    while(remaining > 0)
    {
        size_t chunk = std::min<size_t>(sizeof(buffer), remaining);
        if(!writeFull(fd, buffer, chunk))
        {
            close(fd);
            return false;
        }
        remaining -= chunk;
    }

    return close(fd) == 0;
}

void bestEffortRemove(const char *path)
{
    if(unlink(path) < 0 && errno != ENOENT)
        iprintf("Warning: could not remove %s (%d)\n", path, errno);
}

void failScenario(const char *scenario, const char *reason, int err = 0)
{
    char buf[256];
    if(err > 0)
        snprintf(buf, sizeof(buf), "%s: %s (errno=%d)", scenario, reason, err);
    else
        snprintf(buf, sizeof(buf), "%s: %s", scenario, reason);
    fail(buf);
}

void ensureMounted()
{
    if(waitSentinel(AM_TIMEOUT_MS)) return;
    fail("busy extraction: card not mounted before scenario");
}

void waitForReinsertion()
{
    waitForAck("Reinsert the SD card now.");
    if(!waitSentinel(AM_TIMEOUT_MS))
        fail("busy extraction: card did not remount after reinsertion");
}

void markProgress(WorkerState& state)
{
    state.progressCounter.fetch_add(1);
    state.lastProgressNs.store(getTime());
}

void markIoError(WorkerState& state, int err)
{
    state.lastError.store(err > 0 ? err : EIO);
    state.sawIoError.store(true);
    state.finished.store(true);
}

void runReadLoop(WorkerState& state, const char *path)
{
    unsigned char buffer[IO_CHUNK];
    int fd = open(path, O_RDONLY, 0);
    if(fd < 0)
    {
        markIoError(state, errno);
        return;
    }

    for(;;)
    {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if(n > 0)
        {
            markProgress(state);
            continue;
        }
        if(n == 0)
        {
            if(lseek(fd, 0, SEEK_SET) < 0)
            {
                int err = errno;
                close(fd);
                markIoError(state, err);
                return;
            }
            continue;
        }
        {
            int err = errno;
            close(fd);
            markIoError(state, err);
            return;
        }
    }
}

void runWriteLoop(WorkerState& state, const char *path)
{
    unsigned char buffer[IO_CHUNK];
    for(unsigned int i = 0; i < sizeof(buffer); i++)
        buffer[i] = static_cast<unsigned char>(0x30 + (i % 40));

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_SYNC, AM_FILE_MODE);
    if(fd < 0)
    {
        markIoError(state, errno);
        return;
    }

    size_t bytesWrittenSinceReset = 0;
    for(;;)
    {
        if(!writeFull(fd, buffer, sizeof(buffer)))
        {
            int err = errno;
            close(fd);
            markIoError(state, err);
            return;
        }
        markProgress(state);
        bytesWrittenSinceReset += sizeof(buffer);
        if(bytesWrittenSinceReset < PRECREATE_SIZE) continue;

        close(fd);
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, AM_FILE_MODE);
        if(fd < 0)
        {
            markIoError(state, errno);
            return;
        }
        bytesWrittenSinceReset = 0;
    }
}

void *ioWorker(void *arg)
{
    WorkerContext *ctx = reinterpret_cast<WorkerContext*>(arg);
    WorkerState& state = *ctx->state;

    state.lastProgressNs.store(getTime());

    switch(ctx->mode)
    {
        case WorkerMode::SequentialRead:
            runReadLoop(state, ctx->path);
            return nullptr;

        case WorkerMode::SyncWrite:
            runWriteLoop(state, ctx->path);
            return nullptr;
    }

    markIoError(state, EINVAL);
    return nullptr;
}

void waitWarmup(const char *scenario, WorkerState& state)
{
    // Do not ask for card removal until the worker has already started
    // issuing file I/O. Otherwise a failure could come from setup, not from
    // extraction while the storage path is active.
    const long long deadline = getTime() + WARMUP_TIMEOUT_MS * 1000000LL;
    while(getTime() < deadline)
    {
        if(state.sawIoError.load())
            failScenario(scenario, "worker reported I/O error before removal",
                         state.lastError.load());

        if(state.finished.load())
            failScenario(scenario, "worker finished before removal");

        if(state.progressCounter.load() >= WARMUP_PROGRESS)
            return;

        const unsigned int progress = state.progressCounter.load();
        const long long lastNs = state.lastProgressNs.load();
        if(progress > 0 && lastNs != 0 &&
           getTime() - lastNs > STALL_TIMEOUT_MS * 1000000LL)
        {
            failScenario(scenario, "worker stalled before removal");
        }

        Thread::sleep(POLL_MS);
    }
    failScenario(scenario, "worker did not reach warmup state");
}

void waitCompletionAfterRemoval(const char *scenario, WorkerState& state)
{
    // After extraction, a regular I/O error is acceptable. A worker that
    // stops making progress and never terminates indicates a broken path.
    const long long deadline = getTime() + SCENARIO_TIMEOUT_MS * 1000000LL;
    while(getTime() < deadline)
    {
        if(state.finished.load()) return;

        const long long lastNs = state.lastProgressNs.load();
        if(lastNs != 0 &&
           getTime() - lastNs > STALL_TIMEOUT_MS * 1000000LL)
        {
            failScenario(scenario, "worker stalled after removal");
        }
        Thread::sleep(POLL_MS);
    }
    failScenario(scenario, "worker did not finish after removal");
}

void joinWorker(const char *scenario, Thread *thread)
{
    if(thread == nullptr || thread->join() == false)
        failScenario(scenario, "could not join worker thread");
}

void prepareScenario(const Scenario& scenario)
{
    if(scenario.precreateFile)
    {
        if(!preparePatternFile(scenario.path, PRECREATE_SIZE))
            failScenario(scenario.name, "could not prepare read scenario file");
        return;
    }

    bestEffortRemove(scenario.path);
}

void runScenario(const Scenario& scenario)
{
    testName(scenario.name);
    ensureMounted();
    prepareScenario(scenario);

    // The worker keeps the filesystem busy while the main test thread only
    // coordinates the manual extraction steps and watches for stalls.
    WorkerState state;
    WorkerContext context = {&state, scenario.mode, scenario.path};
    Thread *thread = Thread::create(ioWorker, WORKER_STACK, DEFAULT_PRIORITY,
                                    &context, Thread::JOINABLE);
    if(thread == nullptr)
        failScenario(scenario.name, "could not create worker thread");

    waitWarmup(scenario.name, state);
    waitForAck("Remove the SD card now.");
    waitCompletionAfterRemoval(scenario.name, state);
    joinWorker(scenario.name, thread);
    pass();

    if(scenario.requireReinsertionAfter)
        waitForReinsertion();
}

} // namespace busy

void hwBusyExtraction()
{
    if(!checkAvailHeap(estThreadHeapUsage(busy::WORKER_STACK) * 2))
        return;

    static const busy::Scenario scenarios[] = {
        {"[hw] [5.1] busy extraction read loop",
         busy::WorkerMode::SequentialRead,
         busy::READ_FILE,
         true,
         true},
        {"[hw] [5.2] busy extraction sync write loop",
         busy::WorkerMode::SyncWrite,
         busy::WRITE_FILE,
         false,
         false}
    };

    // Run two complementary scenarios:
    // - a read loop on an existing file
    // - a synchronous write loop on a separate file
    // Both should end with a regular I/O failure after extraction, never with
    // a deadlock or a silently stuck worker thread.
    for(const auto& scenario : scenarios)
        busy::runScenario(scenario);
}

} // namespace

void test_automounter()
{
    logicTest1();
    logicTest2();
    logicTest3();
    logicTest4();
    logicTest5();

    #ifndef WITH_AUTOMOUNTER
    iprintf("Automounter hardware tests skipped, WITH_AUTOMOUNTER is disabled\n");
    return;
    #else
    if(!askYesNo("Run interactive hardware automounter tests now?"))
    {
        iprintf("Interactive hardware automounter tests skipped by user\n");
        return;
    }

    if(askYesNo("Was the board booted with the SD card already inserted?"))
        hwBootWithCard();
    else
        hwInsertCard();

    if(!isMounted())
    {
        waitForAck("Insert the SD card to continue.");
        if(!waitSentinel(AM_TIMEOUT_MS))
            fail("could not mount card for removal/reinsert tests");
    }

    hwRemoveCard();
    hwReinsertCard();

    bool ranBusyExtraction=false;
    if(askYesNo("Run busy extraction hardware test now?"))
    {
        hwBusyExtraction();
        ranBusyExtraction=true;
    }
    else
        iprintf("Busy extraction hardware test skipped by user\n");

    if(ranBusyExtraction)
        iprintf("All automounter tests passed.\n");
    else
        iprintf("All selected automounter tests passed.\n");
    #endif
}

#else

void test_automounter()
{
    iprintf("Automounter tests skipped, filesystem support is disabled\n");
}

#endif
