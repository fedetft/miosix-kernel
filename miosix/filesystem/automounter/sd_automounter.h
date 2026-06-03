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

#pragma once

#include "miosix.h"
#include "config/sd_automounter_config.h"

#ifdef WITH_FILESYSTEM

#include <algorithm>
#include <atomic>
#include "kernel/sync.h"
#include "filesystem/devfs/devfs.h"

namespace miosix
{

    enum class SdAutomounterEdge { None, Inserted, Removed };

    /**
     * \brief Self-contained polling state for the SD automounter debounce logic.
     *
     * This state machine is intentionally kept independent from the worker
     * thread and from the rest of SdAutomounter so that the debounce logic can
     * be directly tested deterministically.
     *
     * \tparam requiredStableSamples is the number of consecutive stable samples
     * required to accept a state change. It is part of the type, so tests can
     * instantiate the polling state with a compile-time debounce threshold and
     * validate edge generation without runtime configuration.
     *
     * A static_assert prevents invalid values at compile time.
     */
    template<int requiredStableSamples = SD_AUTOMOUNTER_DEBOUNCE_SAMPLES>
    class SdAutomounterPollingState
    {
        static_assert(requiredStableSamples > 0, "debounce threshold must be positive");
    public:
        /**
         * \brief Build the initial polling state.
         *
         * If the card is already present at boot, the counter starts saturated
         * and the first advance(true) emits an Inserted edge so the automounter
         * can mount an already-inserted card.
         */
        explicit SdAutomounterPollingState(bool present)
        {
            stablePresent = present;
            lastPresent = false;
            samplesCount = present ? requiredStableSamples : 0;
        }

        SdAutomounterEdge advance(bool raw)
        {
            if (raw)
                samplesCount = std::min(samplesCount + 1, requiredStableSamples);
            else
                samplesCount = std::max(samplesCount - 1, 0);

            if (samplesCount >= requiredStableSamples)
                stablePresent = true;
            else if (samplesCount <= 0)
                stablePresent = false;

            if (stablePresent == lastPresent)
                return SdAutomounterEdge::None;

            lastPresent = stablePresent;
            return stablePresent ? SdAutomounterEdge::Inserted
                                : SdAutomounterEdge::Removed;
        }

        bool isPresent() const { return stablePresent; }

    private:
        bool stablePresent;
        bool lastPresent;
        int samplesCount;
    };

    #if WITH_SD_CD_PIN
    /// Read hardware card-detect pin and apply board-configured polarity.
    inline bool sdCardPresentByCd()
    {
        bool cd = sdAutomounterCardDetectPin::value() != 0;
        if constexpr(sdAutomounterCdPolarity == SdAutomounterCdPolarity::ActiveLow)
            return !cd;
        else
            return cd;
    }
    #endif

    /**
     * \brief Polling-based SD card automounter.
     *
     * This module periodically checks whether an SD card is present, applies a
     * simple debounce filter, and mounts/unmounts `/sd` on insertion/removal.
     *
     * The BSP must provide a CardProbeFunction function that returns true if the card
     * is present.
     */
    class SdAutomounter
    {
    public:
        typedef bool (*CardProbeFunction)();

        /**
         * \return singleton instance
         */
        static SdAutomounter &instance();

        /**
         * Board-agnostic configure. Selects the probe strategy (hardware CD or
         * SDIO software probing) based on WITH_SD_CD_PIN and registers the block
         * device in DevFs. Call once from BSP before enable().
         *
         * \param storage storage device used to mount the filesystem
         */
        void configure(intrusive_ref_ptr<Device> storage);

        /**
         * Configure with explicit probe function. Use when the default probe
         * strategy is not suitable.
         *
         * \param storage storage device used to mount the filesystem
         * \param detect card detect function (polling); must not be nullptr
         * \param pollMs polling period (ms)
         * \param reinitBeforeMount true to reinitialize the storage device before mounting
         */
        void configure(
                        intrusive_ref_ptr<Device> storage,
                        CardProbeFunction detect,
                        int pollMs = SD_AUTOMOUNTER_POLL_MS,
                        bool reinitBeforeMount = SD_AUTOMOUNTER_REINIT_BEFORE_MOUNT_DEFAULT);

        /**
         * Enable/disable automounter at runtime. enable() also registers the
         * storage device in DevFs (if WITH_DEVFS is enabled).
         */
        void enable();
        void disable();

        /**
         * \brief Stop the worker thread permanently.
         *
         * This method is intended for shutdown and reboot paths. It prevents
         * further polling activity, requests cooperative thread termination,
         * and waits until the worker exits.
         *
         * After the stop operation returns, the automounter is no longer
         * running and must not be restarted.
         */
        void stop();

        bool isEnabled() const { return enabled.load(); }

    private:
        SdAutomounter();
        SdAutomounter(const SdAutomounter &);
        SdAutomounter &operator=(const SdAutomounter &);

        /**
         * \brief Thread entry point for the SD automounter worker.
         *
         * Miosix `Thread::create()` expects a static member function,
         * while `run()` is a non-static member function of the SD
         * automounter that needs to access instance members.
         *
         * This helper converts the generic thread argument back to an
         * SdAutomounter object and starts its main loop.
         *
         * \param arg pointer to the SdAutomounter instance
         * \return nullptr when the worker thread exits
         */
        static void *threadTrampoline(void *arg);

        void run();

        /** \brief This method polls for the presence of the SD card.
         *
         * This is a wrapper around the configured CardProbeFunction and returns its
         * result
         *
         * \return true if the card is present, false otherwise
         */
        bool probeCardPresence() const;

        #if WITH_SD_CD_PIN==0
        /// SDIO software probe with reinit backoff. Uses this->storage directly.
        bool sdioProbePresence();
        /// Static trampoline so sdioProbePresence() fits the CardProbeFunction signature.
        static bool sdioProbeStub();
        #endif

        /**
         * \brief Ensure that the /sd mount point exists.
         *
         * This method resolves the root filesystem and creates the `/sd`
         * directory in it if needed. If the directory already exists, that
         * condition is treated as success.
         *
         * \return true if /sd exists or is created successfully, false otherwise
         */
        bool ensureSdMountpoint();

        bool mountSd();
        bool unmountSd();
        bool tryMountFat32(intrusive_ref_ptr<FileBase>& disk);
        bool tryMountLittleFs(intrusive_ref_ptr<FileBase>& disk);

        /**
         * \brief Open the configured storage device for mounting.
         *
         * This method opens the provided storage device representing the SD card
         * and stores the resulting FileBase object in `disk` on success.
         *
         * \param disk output reference that receives the opened device object
         * \return 0 on success, or a error code on failure
         */
        int openDisk(intrusive_ref_ptr<FileBase>& disk);

        intrusive_ref_ptr<Device> storage;
        CardProbeFunction detect;
        int pollMs;
        bool reinitBeforeMount;

        std::atomic<bool> enabled;
        std::atomic<bool> configured;

        Mutex mtx;
        ConditionVariable cv;

        Thread *worker;

        SdAutomounterPollingState<> pollingState;
        bool sdMounted;
        #if WITH_SD_CD_PIN==0
        int sdioReinitCountdown;
        #endif
    };

} // namespace miosix

#endif // WITH_FILESYSTEM
