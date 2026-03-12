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

#include "MemoryMonitor.h"
#include "AppManagerImplementation.h"
#include "tracing/Logging.h"

namespace WPEFramework {
namespace Plugin {

MemoryMonitor::MemoryMonitor(AppManagerImplementation* parent)
    : _parent(parent)
    , _running(false)
    , _pending(false)
    , _completed(false)
    , _targetRAMKB(0)
    , _waitingTargetState(Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN)
{
    LOGINFO("[MemoryMonitor] Created");
}

MemoryMonitor::~MemoryMonitor()
{
    Stop();
    LOGINFO("[MemoryMonitor] Destroyed");
}

void MemoryMonitor::Start()
{
    if (_running.load())
    {
        LOGWARN("[MemoryMonitor] Already running");
        return;
    }
    _running = true;
    _thread = std::thread(&MemoryMonitor::MonitorLoop, this);
    LOGINFO("[MemoryMonitor] Monitor thread started");
}

void MemoryMonitor::Stop()
{
    if (!_running.load())
    {
        return;
    }
    _running = false;
    _cv.notify_all();
    _stateCv.notify_all();
    _completionCv.notify_all();
    if (_thread.joinable())
    {
        _thread.join();
        LOGINFO("[MemoryMonitor] Monitor thread joined");
    }
}

void MemoryMonitor::RequestReconciliation(const string& appId, uint32_t targetRAMKB)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _activeAppId = appId;
    _targetRAMKB = targetRAMKB;
    _pending = true;
    _completed = false;
    LOGINFO("[MemoryMonitor] Reconciliation requested (async) for appId: %s, target: %u KB",
            appId.c_str(), targetRAMKB);
    _cv.notify_one();
}

void MemoryMonitor::RequestReconciliationAndWait(const string& appId, uint32_t targetRAMKB)
{
    /* Serialize concurrent blocking callers so only one waits at a time */
    std::unique_lock<std::mutex> blockLock(_blockingMutex);

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _activeAppId = appId;
        _targetRAMKB = targetRAMKB;
        _pending = true;
        _completed = false;
        LOGINFO("[MemoryMonitor] Reconciliation requested (blocking) for appId: %s, target: %u KB",
                appId.c_str(), targetRAMKB);
        _cv.notify_one();
    }

    /* Wait until the monitor thread finishes the reclamation cycle */
    std::unique_lock<std::mutex> completionLock(_completionMutex);
    _completionCv.wait(completionLock, [this] { return _completed || !_running.load(); });

    LOGINFO("[MemoryMonitor] Blocking reconciliation completed. Final MemAvailable: %u KB",
            _parent->ReadMemAvailable());
}

void MemoryMonitor::OnAppStateChanged(const string& appId, Exchange::IAppManager::AppLifecycleState newState)
{
    std::lock_guard<std::mutex> lock(_stateMutex);
    if (appId == _waitingAppId && newState == _waitingTargetState)
    {
        LOGINFO("[MemoryMonitor] State change detected for %s -> %d, notifying waiter",
                appId.c_str(), static_cast<int>(newState));
        _stateCv.notify_all();
    }
}

void MemoryMonitor::MonitorLoop()
{
    LOGINFO("[MemoryMonitor] MonitorLoop entered");
    while (_running.load())
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this] { return _pending || !_running.load(); });

        if (!_running.load())
        {
            break;
        }

        _pending = false;
        string activeId = _activeAppId;
        uint32_t target = _targetRAMKB;
        lock.unlock();

        ExecuteSequentialReclamation(activeId, target);

        /* Signal any blocking caller that the cycle is complete */
        {
            std::lock_guard<std::mutex> completionLock(_completionMutex);
            _completed = true;
        }
        _completionCv.notify_all();
    }
    LOGINFO("[MemoryMonitor] MonitorLoop exited");
}

void MemoryMonitor::ExecuteSequentialReclamation(const string& activeId, uint32_t targetRAMKB)
{
    uint32_t current = _parent->ReadMemAvailable();
    LOGINFO("[MemoryMonitor] RAM Reconciliation Started. Active: %s, Target: %u KB, Current Available: %u KB",
            activeId.c_str(), targetRAMKB, current);

    /* Log the current state of all managed apps */
    _parent->LogManagedAppsSnapshot();

    if (current >= targetRAMKB)
    {
        LOGINFO("[MemoryMonitor] Memory already sufficient (%u KB >= %u KB). No action needed.",
                current, targetRAMKB);
        return;
    }

    /* Phase 1: Kill HIBERNATED apps (oldest first) */
    if (Resolve(Exchange::IAppManager::APP_STATE_HIBERNATED, "KILL", targetRAMKB, activeId))
    {
        LOGINFO("[MemoryMonitor] RAM target met after Phase 1 (Kill Hibernated).");
        LOGINFO("[MemoryMonitor] RAM Reconciliation Finished. Final MemAvailable: %u KB", _parent->ReadMemAvailable());
        return;
    }

    /* Phase 2: Hibernate SUSPENDED apps (oldest first) */
    if (Resolve(Exchange::IAppManager::APP_STATE_SUSPENDED, "HIBERNATE", targetRAMKB, activeId))
    {
        LOGINFO("[MemoryMonitor] RAM target met after Phase 2 (Hibernate Suspended).");
        LOGINFO("[MemoryMonitor] RAM Reconciliation Finished. Final MemAvailable: %u KB", _parent->ReadMemAvailable());
        return;
    }

    /* Phase 3: Suspend PAUSED apps (oldest first) */
    Resolve(Exchange::IAppManager::APP_STATE_PAUSED, "SUSPEND", targetRAMKB, activeId);

    LOGINFO("[MemoryMonitor] RAM Reconciliation Finished. Final MemAvailable: %u KB", _parent->ReadMemAvailable());
}

bool MemoryMonitor::Resolve(Exchange::IAppManager::AppLifecycleState state,
                                   const string& action,
                                   uint32_t targetRAMKB,
                                   const string& ignoreId)
{
    std::vector<string> candidates = _parent->GetSortedCandidates(state);

    for (const auto& appId : candidates)
    {
        if (appId == ignoreId)
        {
            continue;
        }

        if (_parent->ReadMemAvailable() >= targetRAMKB)
        {
            return true;
        }

        struct timespec ts = _parent->GetAppStateTime(appId);
        long elapsed = _parent->GetElapsedTimeSeconds(ts);

        LOGINFO("[MemoryMonitor] DECISION: Selecting %s for %s. Reason: Oldest app in state %d. Time in state: %ld seconds.",
                appId.c_str(), action.c_str(), static_cast<int>(state), elapsed);

        Exchange::IAppManager::AppLifecycleState nextState;

        if (action == "KILL")
        {
            /* Preloaded apps must not be terminated — only hibernate is allowed */
            if (_parent->IsPreloadedApp(appId))
            {
                LOGINFO("[MemoryMonitor] Skipping KILL for preloaded app %s (only hibernation allowed for preloaded apps)",
                        appId.c_str());
                continue;
            }
            _parent->KillApp(appId);
            nextState = Exchange::IAppManager::APP_STATE_UNLOADED;
        }
        else if (action == "HIBERNATE")
        {
            _parent->HibernateApp(appId);
            nextState = Exchange::IAppManager::APP_STATE_HIBERNATED;
        }
        else /* SUSPEND */
        {
            _parent->SuspendApp(appId);
            nextState = Exchange::IAppManager::APP_STATE_SUSPENDED;
        }

        /* Wait for the state transition instead of sleeping */
        if (WaitForState(appId, nextState))
        {
            LOGINFO("[MemoryMonitor] Successfully transitioned %s to target state %d.",
                    appId.c_str(), static_cast<int>(nextState));
        }
        else
        {
            LOGWARN("[MemoryMonitor] TIMEOUT (%ds) reached waiting for %s to transition to state %d.",
                    STATE_WAIT_TIMEOUT_SECONDS, appId.c_str(), static_cast<int>(nextState));
        }

        if (_parent->ReadMemAvailable() >= targetRAMKB)
        {
            return true;
        }
    }
    return false;
}

bool MemoryMonitor::WaitForState(const string& appId, Exchange::IAppManager::AppLifecycleState targetState)
{
    std::unique_lock<std::mutex> lock(_stateMutex);
    _waitingAppId = appId;
    _waitingTargetState = targetState;

    LOGINFO("[MemoryMonitor] Waiting up to %ds for %s to reach state %d...",
            STATE_WAIT_TIMEOUT_SECONDS, appId.c_str(), static_cast<int>(targetState));

    bool success = _stateCv.wait_for(lock, std::chrono::seconds(STATE_WAIT_TIMEOUT_SECONDS),
        [this, &appId, targetState] {
            return _parent->GetAppState(appId) == targetState || !_running.load();
        });

    _waitingAppId = "";
    return success;
}

} /* namespace Plugin */
} /* namespace WPEFramework */
