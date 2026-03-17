/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <map>
#include <interfaces/IAppManager.h>

namespace WPEFramework {
namespace Plugin {

    /* Forward declaration to avoid circular includes */
    class AppManagerImplementation;

    /**
     * @brief MemoryMonitor - Background monitor for sequential RAM reclamation.
     *
     * Runs a dedicated thread within AppManagerImplementation responsible for
     * sequential memory reclamation using the Linux MemoryAvailable metric from
     * /proc/meminfo. Handles RAM-only pressure by executing a specific order of
     * operations:
     *   Pre-Phase 1: Suspend PAUSED apps (if suspendable, oldest first)
     *   Pre-Phase 2: Hibernate SUSPENDED apps (if hibernatable & delay elapsed, oldest first)
     *   Phase 1: Terminate PAUSED apps (oldest first, skip preloaded)
     *   Phase 2: Terminate SUSPENDED apps (oldest first, skip preloaded)
     *   Phase 3: Terminate HIBERNATED apps (oldest first, skip preloaded)
     *
     * RAM Target Registry:
     *   - Launch Target: configuration.urn:rdk:config:memory.system (e.g. "256M")
     *   - Preload Target: configuration.urn:rdk:config:application-lifecycle.maxSuspendedSystemMemory (e.g. "16M")
     */
    class MemoryMonitor {
    public:
        explicit MemoryMonitor(AppManagerImplementation* parent);
        ~MemoryMonitor();

        MemoryMonitor(const MemoryMonitor&) = delete;
        MemoryMonitor& operator=(const MemoryMonitor&) = delete;

        /**
         * @brief Start the monitor background thread.
         */
        void Start();

        /**
         * @brief Stop the monitor background thread and join.
         */
        void Stop();

        /**
         * @brief Request a memory reconciliation cycle for the given app (async).
         * @param appId   The app that needs memory cleared for.
         * @param targetRAMKB  The target MemoryAvailable in KB.
         * @param allowTermination If true, termination phases are enabled; if false, only suspend/hibernate.
         */
        void RequestReconciliation(const string& appId, uint32_t targetRAMKB, bool allowTermination = true);

        /**
         * @brief Request a memory reconciliation cycle and block until it completes.
         *        Used by PreloadApp (before scheduling) and by the worker thread
         *        (before actual launch) to ensure memory is available.
         * @param appId   The app that needs memory cleared for.
         * @param targetRAMKB  The target MemoryAvailable in KB.
         * @param allowTermination If true, termination phases are enabled; if false, only suspend/hibernate.
         * @return true if MemoryAvailable >= targetRAMKB after reclamation, false otherwise.
         */
        bool RequestReconciliationAndWait(const string& appId, uint32_t targetRAMKB, bool allowTermination = true);

        /**
         * @brief Called by AppManagerImplementation::handleOnAppLifecycleStateChanged
         *        to notify the monitor that an app changed state, so it can stop
         *        waiting if the expected state transition happened.
         * @param appId    The application that changed state.
         * @param newState The new lifecycle state.
         */
        void OnAppStateChanged(const string& appId, Exchange::IAppManager::AppLifecycleState newState);

    private:
        /**
         * @brief Main loop for the background monitor thread.
         */
        void MonitorLoop();

        /**
         * @brief Execute the sequential reclamation phases for the active app.
         * @param activeId The app ID that triggered the reclamation.
         * @param targetRAMKB  The required MemoryAvailable threshold in KB.
         * @param allowTermination If false, only suspend/hibernate phases run (no kill).
         * @return true if MemoryAvailable >= targetRAMKB after all phases.
         */
        bool ExecuteSequentialReclamation(const string& activeId, uint32_t targetRAMKB, bool allowTermination);

        /**
         * @brief Try to resolve memory pressure by terminating apps in the given state.
         * @param state    The lifecycle state of candidate apps.
         * @param action   The action string: "TERMINATE".
         * @param targetRAMKB  The required MemoryAvailable threshold in KB.
         * @param ignoreId The app ID to not touch (the active foreground app).
         * @param actionsPerformed [out] Incremented for each app that was terminated.
         * @return true if MemoryAvailable >= targetRAMKB after resolution.
         */
        bool Resolve(Exchange::IAppManager::AppLifecycleState state,
                     const string& action,
                     uint32_t targetRAMKB,
                     const string& ignoreId,
                     int& actionsPerformed);

        /**
         * @brief Pre-phase: Move PAUSED apps to SUSPENDED (if the app supports it).
         * @param targetRAMKB  The required MemoryAvailable threshold in KB.
         * @param ignoreId The app ID to not touch.
         * @param actionsPerformed [out] Incremented for each app suspended.
         * @return true if MemoryAvailable >= targetRAMKB.
         */
        bool ResolveSuspend(uint32_t targetRAMKB,
                            const string& ignoreId,
                            int& actionsPerformed);

        /**
         * @brief Pre-phase: Move SUSPENDED apps to HIBERNATED (if supported and
         *        the hibernate delay has elapsed since the app entered SUSPENDED).
         * @param targetRAMKB  The required MemoryAvailable threshold in KB.
         * @param ignoreId The app ID to not touch.
         * @param actionsPerformed [out] Incremented for each app hibernated.
         * @return true if MemoryAvailable >= targetRAMKB.
         */
        bool ResolveHibernate(uint32_t targetRAMKB,
                              const string& ignoreId,
                              int& actionsPerformed);

        /**
         * @brief Wait for a specific app to reach the target state with a timeout.
         * @param appId       The app to wait on.
         * @param targetState The expected lifecycle state.
         * @return true if the app reached the target state before timeout.
         */
        bool WaitForState(const string& appId, Exchange::IAppManager::AppLifecycleState targetState);

        AppManagerImplementation* _parent;
        std::thread _thread;
        std::mutex _mutex;
        std::mutex _stateMutex;
        std::mutex _completionMutex;
        std::mutex _blockingMutex;        /* Serializes concurrent blocking callers */
        std::condition_variable _cv;
        std::condition_variable _stateCv;
        std::condition_variable _completionCv;
        std::atomic<bool> _running;
        bool _pending;
        bool _completed;
        bool _reconciliationSuccess;
        string _activeAppId;
        uint32_t _targetRAMKB;
        bool _allowTermination;
        string _waitingAppId;
        Exchange::IAppManager::AppLifecycleState _waitingTargetState;

        /* Tracks when each app entered SUSPENDED state (for hibernate delay) */
        std::map<string, struct timespec> _suspendedTimestamps;

        static constexpr int STATE_WAIT_TIMEOUT_SECONDS = 10;
    };

} /* namespace Plugin */
} /* namespace WPEFramework */
