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
     *   Phase 1: Kill HIBERNATED apps (oldest first)
     *   Phase 2: Hibernate SUSPENDED apps (oldest first)
     *   Phase 3: Suspend PAUSED apps (oldest first)
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
         */
        void RequestReconciliation(const string& appId, uint32_t targetRAMKB);

        /**
         * @brief Request a memory reconciliation cycle and block until it completes.
         *        Used by PreloadApp (before scheduling) and by the worker thread
         *        (before actual launch) to ensure memory is available.
         * @param appId   The app that needs memory cleared for.
         * @param targetRAMKB  The target MemoryAvailable in KB.
         * @return true if MemoryAvailable >= targetRAMKB after reclamation, false otherwise.
         */
        bool RequestReconciliationAndWait(const string& appId, uint32_t targetRAMKB);

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
         * @return true if MemoryAvailable >= targetRAMKB after all phases.
         */
        bool ExecuteSequentialReclamation(const string& activeId, uint32_t targetRAMKB);

        /**
         * @brief Try to resolve memory pressure by transitioning apps in the given state.
         * @param state    The lifecycle state of candidate apps.
         * @param action   The action string: "KILL", "HIBERNATE", or "SUSPEND".
         * @param targetRAMKB  The required MemoryAvailable threshold in KB.
         * @param ignoreId The app ID to not touch (the active foreground app).
         * @return true if MemoryAvailable >= targetRAMKB after resolution.
         */
        bool Resolve(Exchange::IAppManager::AppLifecycleState state,
                     const string& action,
                     uint32_t targetRAMKB,
                     const string& ignoreId);

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
        string _waitingAppId;
        Exchange::IAppManager::AppLifecycleState _waitingTargetState;

        static constexpr int STATE_WAIT_TIMEOUT_SECONDS = 6;
    };

} /* namespace Plugin */
} /* namespace WPEFramework */
