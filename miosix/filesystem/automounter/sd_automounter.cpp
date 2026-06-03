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

#include "sd_automounter.h"

#ifdef WITH_FILESYSTEM

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include "filesystem/fat32/fat32.h"
#include "filesystem/file_access.h"
#include "filesystem/file.h"
#include "filesystem/ioctl.h"
#include "filesystem/stringpart.h"
#include "filesystem/littlefs/lfs_miosix.h"

#if AUTOMOUNTER_DEBUG_LOG
#define AUTOMOUNTER_LOG(fmt, ...) iprintf("[Automounter] " fmt, ##__VA_ARGS__)
#else
#define AUTOMOUNTER_LOG(fmt, ...) do { } while(0)
#endif

namespace miosix
{

    namespace
    {
        const int automounterWorkerStack = 2048;
        const int automounterWorkerPriority = 1;

        #if AUTOMOUNTER_DEBUG_LOG
        const char *errnoName(int error)
        {
            if (error < 0)
                error = -error;

            switch (error)
            {
                case 0: return "0";
                case EACCES: return "EACCES";
                case EBUSY: return "EBUSY";
                case EEXIST: return "EEXIST";
                case EINVAL: return "EINVAL";
                case ENODEV: return "ENODEV";
                case ENOENT: return "ENOENT";
                case ENOMEM: return "ENOMEM";
                case ENFILE: return "ENFILE";
                case ENOTTY: return "ENOTTY";
                case EROFS: return "EROFS";
                default: return "UNKNOWN";
            }
        }
        #endif

        // The GPIO pin used for timing measurements of the SD automounter can be
        // optionally defined in board_settings.h
        #if defined(SD_AUTOMOUNTER_TIMING_GPIO)
        void initTimingPin()
        {
            static bool initialized = false;
            if (initialized)
                return;

            sdAutomounterTimingPin::mode(Mode::OUTPUT);
            sdAutomounterTimingPin::low();
            initialized = true;
        }

        class TimingLogger
        {
        public:
            explicit TimingLogger(const char *event)
                : event(event), startNs(getTime()), finished(false)
            {
                initTimingPin();
                sdAutomounterTimingPin::high();
            }

            void finish(const char *outcome)
            {
                if (finished)
                    return;

                const long long elapsedNs = getTime() - startNs;
                sdAutomounterTimingPin::low();
                iprintf("[Automounter] [Time Log] %s %s: %lld us\n", event, outcome, elapsedNs/1000);
                finished = true;
            }

            ~TimingLogger()
            {
                finish("aborted");
            }

        private:
            const char *event;
            long long startNs;
            bool finished;
        };
        #else

        class TimingLogger
        {
        public:
            explicit TimingLogger(const char *) {}
            void finish(const char *) {}
        };
        #endif
    }

    SdAutomounter &SdAutomounter::instance()
    {
        static SdAutomounter inst;
        return inst;
    }

    SdAutomounter::SdAutomounter()
        : storage(),
          detect(nullptr),
          pollMs(SD_AUTOMOUNTER_POLL_MS),
          reinitBeforeMount(SD_AUTOMOUNTER_REINIT_BEFORE_MOUNT_DEFAULT),
          enabled(false),
          configured(false),
          worker(nullptr),
          pollingState(false),
          sdMounted(false)
          #if WITH_SD_CD_PIN==0
          , sdioReinitCountdown(0)
          #endif

    {
    }

    void SdAutomounter::configure(intrusive_ref_ptr<Device> storage)
    {
        #if WITH_SD_CD_PIN
        configure(storage, &sdCardPresentByCd,
                  SD_AUTOMOUNTER_POLL_MS,
                  SD_AUTOMOUNTER_REINIT_BEFORE_MOUNT_WITH_CD);
        AUTOMOUNTER_LOG("Mode: hardware CD\n");
        #else
        // In SDIO probing mode, the probe already performs successful readBlock()
        // calls, so reinit before mount would disrupt the working state.
        configure(storage, &sdioProbeStub,
                  SD_AUTOMOUNTER_POLL_MS,
                  SD_AUTOMOUNTER_REINIT_BEFORE_MOUNT_WITH_SDIO_PROBE);
        AUTOMOUNTER_LOG("Mode: SDIO software probing\n");
        #endif
    }

    void SdAutomounter::configure(intrusive_ref_ptr<Device> storage, CardProbeFunction detect,
                                  int pollMs, bool reinitBeforeMount)
    {
        assert(!configured.load());
        assert(detect != nullptr);

        Lock<Mutex> l(mtx);

        this->storage = storage;
        this->detect = detect;
        this->pollMs = pollMs > 0 ? pollMs : SD_AUTOMOUNTER_POLL_MS;
        this->reinitBeforeMount = reinitBeforeMount;

        // Init the integration counter so it starts saturated in the current
        // direction, avoiding a spurious transition at the first poll cycle.
        pollingState = SdAutomounterPollingState<>(probeCardPresence());

        // Force a fresh edge evaluation when the worker wakes up.
        // This allows mounting an already-inserted card at boot.
        sdMounted = false;

        AUTOMOUNTER_LOG("Configured: poll: %dms\tdebounce: %d\tstate: %s\n",
                    this->pollMs,
                    SD_AUTOMOUNTER_DEBOUNCE_SAMPLES,
                    pollingState.isPresent() ? "present" : "absent");

        if (worker == nullptr)
        {
            worker = Thread::create(threadTrampoline,
                                    automounterWorkerStack,
                                    automounterWorkerPriority,
                                    this,
                                    Thread::JOINABLE);
            if (worker == nullptr)
            {
                AUTOMOUNTER_LOG("Failed to create worker thread\n");
                return;
            }
            AUTOMOUNTER_LOG("Worker thread created\n");
        }

        configured.store(true);
        cv.signal();
    }

    void SdAutomounter::enable()
    {
        if (!configured.load() || !storage)
        {
            AUTOMOUNTER_LOG("Enable requested before a valid configure()\n");
            return;
        }

        #ifdef WITH_DEVFS
        {
            intrusive_ref_ptr<DevFs> devFs = FilesystemManager::instance().getDevFs();
            if(devFs)
            {
                if(devFs->addDevice(SD_AUTOMOUNTER_BLOCK_DEVICE_NAME, storage)==false)
                    AUTOMOUNTER_LOG("DevFs: " SD_AUTOMOUNTER_BLOCK_DEVICE_NAME " already present\n");
            }
            else
            {
                AUTOMOUNTER_LOG("DevFs unavailable, " SD_AUTOMOUNTER_BLOCK_DEVICE_NAME " not created\n");
            }
        }
        #endif

        enabled.store(true);
        AUTOMOUNTER_LOG("Enabled\n");

        Lock<Mutex> l(mtx);
        cv.signal();
    }

    void SdAutomounter::disable()
    {
        enabled.store(false);
        AUTOMOUNTER_LOG("Disabled\n");

        Lock<Mutex> l(mtx);
        cv.signal();
    }

    void SdAutomounter::stop()
    {
        Thread *localWorker = worker;
        if (localWorker == nullptr)
            return;

        enabled.store(false);
        AUTOMOUNTER_LOG("Stopping worker thread\n");
        {
            Lock<Mutex> l(mtx);
            cv.signal();
        }

        localWorker->terminate();
        localWorker->join();
        worker = nullptr;
        AUTOMOUNTER_LOG("Worker thread stopped\n");
    }

    void *SdAutomounter::threadTrampoline(void *automounterInstance)
    {
        reinterpret_cast<SdAutomounter *>(automounterInstance)->run();
        return nullptr;
    }

    bool SdAutomounter::probeCardPresence() const
    {
        return detect();
    }

    #if WITH_SD_CD_PIN==0
    bool SdAutomounter::sdioProbePresence()
    {
        static unsigned char buf[512];

        if (sdioReinitCountdown > 0)
            sdioReinitCountdown--;

        ssize_t r = storage->readBlock(buf, sizeof(buf), 0);
        if (r == 512)
            return true;

        // Reinit only after the backoff expires to avoid reinitializing on every
        // poll cycle when no card is present.
        if (sdioReinitCountdown == 0)
        {
            storage->ioctl(IOCTL_REINIT, nullptr);
            sdioReinitCountdown = SD_AUTOMOUNTER_SDIO_REINIT_BACKOFF_POLLS;
        }

        return false;
    }

    bool SdAutomounter::sdioProbeStub()
    {
        return instance().sdioProbePresence();
    }
    #endif

    bool SdAutomounter::ensureSdMountpoint()
    {
        std::string root("/");
        ResolvedPath resolved = FilesystemManager::instance().resolvePath(root, false);
        if (resolved.result < 0 || !resolved.fs)
        {
            AUTOMOUNTER_LOG("Cannot resolve root filesystem (%s)\n", errnoName(resolved.result));
            return false;
        }

        StringPart sd("sd");
        int result = resolved.fs->mkdir(sd, 0755);
        if (result != 0 && result != -EEXIST)
            AUTOMOUNTER_LOG("Cannot create /sd mountpoint (%s)\n", errnoName(result));
        return result == 0 || result == -EEXIST;
    }

    int SdAutomounter::openDisk(intrusive_ref_ptr<FileBase>& disk)
    {
        if (!storage)
        {
            AUTOMOUNTER_LOG("No storage device configured\n");
            return -ENODEV;
        }
        int result=storage->open(disk, intrusive_ref_ptr<FilesystemBase>(), O_RDWR, 0);
        if(result<0)
            AUTOMOUNTER_LOG("Cannot open storage device (%s)\n", errnoName(result));
        return result;
    }

    bool SdAutomounter::tryMountFat32(intrusive_ref_ptr<FileBase>& disk)
    {
        #ifdef WITH_FATFS
        intrusive_ref_ptr<Fat32Fs> fs(new Fat32Fs(disk));
        if (fs->mountFailed())
            return false;

        int result = FilesystemManager::instance().kmount("/sd", fs);
        if (result == -EBUSY)
            AUTOMOUNTER_LOG("/sd is already mounted (EBUSY)\n");
        else if (result < 0)
            AUTOMOUNTER_LOG("FAT32 mount failed (%s)\n", errnoName(result));
        return result == 0 || result == -EBUSY;
        #else
        (void)disk;
        return false;
        #endif
    }

    bool SdAutomounter::tryMountLittleFs(intrusive_ref_ptr<FileBase>& disk)
    {
        #ifdef WITH_LITTLEFS
        intrusive_ref_ptr<LittleFS> fs(new LittleFS(disk));
        if (fs->mountFailed())
            return false;

        int result = FilesystemManager::instance().kmount("/sd", fs);
        if (result == -EBUSY)
            AUTOMOUNTER_LOG("/sd is already mounted (EBUSY)\n");
        else if (result < 0)
            AUTOMOUNTER_LOG("LittleFS mount failed (%s)\n", errnoName(result));
        return result == 0 || result == -EBUSY;
        #else
        (void)disk;
        return false;
        #endif
    }

    bool SdAutomounter::mountSd()
    {
        if (Thread::testTerminate()) return false;
        if (sdMounted)
        {
            AUTOMOUNTER_LOG("Card already mounted, skipping mount\n");
            return true;
        }

        if (!ensureSdMountpoint()) return false;

        if (reinitBeforeMount && storage)
        {
            // Before probing a filesystem, bring the card back to a known
            // transfer state and let the driver recalibrate its final bus
            // width and clock.
            //
            // Raw presence probing alone is not enough to
            // guarantee that the subsequent mount sees consistent data.

            int reinitResult = storage->ioctl(IOCTL_REINIT, nullptr);
            if (reinitResult < 0)
                AUTOMOUNTER_LOG("Storage reinit failed (%s)\n",
                            errnoName(reinitResult));
        }

        intrusive_ref_ptr<FileBase> disk;
        if (openDisk(disk) < 0) return false;

        // Try all enabled filesystems in the same order used at boot.
        if (tryMountFat32(disk))
        {
            sdMounted = true;
            AUTOMOUNTER_LOG("Mounted /sd using FAT32\n");
            return true;
        }

        if (tryMountLittleFs(disk))
        {
            sdMounted = true;
            AUTOMOUNTER_LOG("Mounted /sd using LittleFS\n");
            return true;
        }

        AUTOMOUNTER_LOG("No supported filesystem detected on card\n");
        return false;
    }

    bool SdAutomounter::unmountSd()
    {
        // Clearing before the actual umount is safe: mount and unmount only
        // run on the single worker thread, so no insertion edge can race here.
        sdMounted = false;

        FilesystemManager& fsm = FilesystemManager::instance();
        const int retries = SD_AUTOMOUNTER_UNMOUNT_RETRY_COUNT > 0
            ? SD_AUTOMOUNTER_UNMOUNT_RETRY_COUNT
            : 1;
        for (int i = 0; i < retries; i++)
        {
            int result = fsm.umount("/sd", false);
            if (result == 0 || result == -EINVAL)
            {
                AUTOMOUNTER_LOG("Unmounted /sd\n");
                return true;
            }
            if (result != -EBUSY)
            {
                AUTOMOUNTER_LOG("Unmount failed (%s)\n", errnoName(result));
                break;
            }
            AUTOMOUNTER_LOG("Unmount busy, retrying (%d/%d)\n", i + 1, retries);
            Thread::sleep(SD_AUTOMOUNTER_UNMOUNT_RETRY_DELAY_MS);
        }

        // Forced umount is a last resort used when handles are still open.
        int forced=fsm.umount("/sd", true);
        if (forced == 0 || forced == -EINVAL)
        {
            AUTOMOUNTER_LOG("Forced unmount completed\n");
            return true;
        }
        else
        {
            AUTOMOUNTER_LOG("Forced unmount failed (%s)\n", errnoName(forced));
            return false;
        }
    }

    void SdAutomounter::run()
    {
        #if AUTOMOUNTER_DEBUG_LOG
        bool lastRawForLog = pollingState.isPresent();
        #endif

        while (!Thread::testTerminate())
        {
            {
                Lock<Mutex> l(mtx);
                while (!Thread::testTerminate() &&
                       (!configured.load() || !enabled.load()))
                {
                    cv.wait(mtx);
                }
            }
            if (Thread::testTerminate())
                break;

            bool raw = probeCardPresence();

            #if AUTOMOUNTER_DEBUG_LOG
            if (raw != lastRawForLog)
            {
                AUTOMOUNTER_LOG("Detected raw status change: now %s\n",
                            raw ? "present" : "absent");
                lastRawForLog = raw;
            }
            #endif

            SdAutomounterEdge edge = pollingState.advance(raw);

            // Edge detection on the debounced state:
            if (edge != SdAutomounterEdge::None)
            {
                if (edge == SdAutomounterEdge::Inserted)
                {
                    TimingLogger timing("inserted");
                    AUTOMOUNTER_LOG("Stable insertion detected\n");
                    //Retry mount a few times. Bus-level CRC errors can
                    //cause intermittent read failures, but retrying
                    //usually succeeds.
                    const int retries = SD_AUTOMOUNTER_MOUNT_RETRY_COUNT > 0
                        ? SD_AUTOMOUNTER_MOUNT_RETRY_COUNT
                        : 1;
                    bool mounted = false;
                    for (int attempt = 0; attempt < retries; attempt++)
                    {
                        if (mountSd())
                        {
                            mounted = true;
                            break;
                        }
                        AUTOMOUNTER_LOG("Mount attempt %d/%d failed, retrying\n",
                                    attempt + 1, retries);
                        if (Thread::testTerminate()) break;
                        Thread::sleep(SD_AUTOMOUNTER_MOUNT_RETRY_DELAY_MS);
                    }

                    timing.finish(mounted ? "success"
                                          : (Thread::testTerminate() ? "aborted"
                                                                     : "failure"));
                }
                else
                {
                    AUTOMOUNTER_LOG("Removal detected stably\n");
                    TimingLogger timing("removed");

                    timing.finish(unmountSd() ? "success" : "failure");
                }
            }

            if (Thread::testTerminate()) break;

            Thread::sleep(pollMs);
        }
    }

} // namespace miosix

#endif // WITH_FILESYSTEM
