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
#include "AppPolicyConfig.h"
#include "tracing/Logging.h"

namespace WPEFramework {
namespace Plugin {

MemoryMonitor::MemoryMonitor(AppManagerImplementation* parent)
    : _parent(parent)
    , _running(false)
    , _pending(false)
    , _completed(false)
    , _reconciliationSuccess(false)
    , _targetRAMKB(0)
    , _allowTermination(true)
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

void MemoryMonitor::RequestReconciliation(const string& appId, uint32_t targetRAMKB, bool allowTermination)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _activeAppId = appId;
    _targetRAMKB = targetRAMKB;
    _allowTermination = allowTermination;
    _pending = true;
    _completed = false;
    LOGINFO("[MemoryMonitor] Reconciliation requested (async) for appId: %s, target: %u KB, allowTermination: %s",
            appId.c_str(), targetRAMKB, allowTermination ? "true" : "false");
    _cv.notify_one();
}

bool MemoryMonitor::RequestReconciliationAndWait(const string& appId, uint32_t targetRAMKB, bool allowTermination)
{
    /* Serialize concurrent blocking callers so only one waits at a time */
    std::unique_lock<std::mutex> blockLock(_blockingMutex);

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _activeAppId = appId;
        _targetRAMKB = targetRAMKB;
        _allowTermination = allowTermination;
        _pending = true;
        _completed = false;
        _reconciliationSuccess = false;
        LOGINFO("[MemoryMonitor] Reconciliation requested (blocking) for appId: %s, target: %u KB, allowTermination: %s",
                appId.c_str(), targetRAMKB, allowTermination ? "true" : "false");
        _cv.notify_one();
    }

    /* Wait until the monitor thread finishes the reclamation cycle */
    std::unique_lock<std::mutex> completionLock(_completionMutex);
    _completionCv.wait(completionLock, [this] { return _completed || !_running.load(); });

    bool success = _reconciliationSuccess;
    LOGINFO("[MemoryMonitor] Blocking reconciliation completed (success=%s). Final MemAvailable: %u KB",
            success ? "true" : "false", _parent->ReadMemAvailable());
    return success;
}

void MemoryMonitor::OnAppStateChanged(const string& appId, Exchange::IAppManager::AppLifecycleState newState)
{
    std::lock_guard<std::mutex> lock(_stateMutex);

    /* Track when an app enters SUSPENDED for hibernate delay calculation */
    if (newState == Exchange::IAppManager::APP_STATE_SUSPENDED)
    {
        struct timespec now;
        if (timespec_get(&now, TIME_UTC) != 0)
        {
            _suspendedTimestamps[appId] = now;
            LOGINFO("[MemoryMonitor] Recorded SUSPENDED timestamp for %s", appId.c_str());
        }
    }
    else if (newState != Exchange::IAppManager::APP_STATE_SUSPENDED)
    {
        /* App left SUSPENDED — remove its timestamp */
        _suspendedTimestamps.erase(appId);
    }

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
        bool allowTermination = _allowTermination;
        lock.unlock();

        bool success = ExecuteSequentialReclamation(activeId, target, allowTermination);

        /* Signal any blocking caller that the cycle is complete */
        {
            std::lock_guard<std::mutex> completionLock(_completionMutex);
            _reconciliationSuccess = success;
            _completed = true;
        }
        _completionCv.notify_all();
    }
    LOGINFO("[MemoryMonitor] MonitorLoop exited");
}

bool MemoryMonitor::ExecuteSequentialReclamation(const string& activeId, uint32_t targetRAMKB, bool allowTermination)
{
    uint32_t current = _parent->ReadMemAvailable();
    LOGINFO("[MemoryMonitor] RAM Reconciliation Started. Active: %s, Target: %u KB, Current Available: %u KB, AllowTermination: %s",
            activeId.c_str(), targetRAMKB, current, allowTermination ? "true" : "false");

    /* Log the current state of all managed apps */
    _parent->LogManagedAppsSnapshot();

    if (current >= targetRAMKB)
    {
        LOGINFO("[MemoryMonitor] Memory already sufficient (%u KB >= %u KB). No action needed.",
                current, targetRAMKB);
        return true;
    }

    /*
     * Memory reclamation phases (executed in order):
     *
     * Pre-Phase 1: Suspend PAUSED apps (if app supports suspension)
     * Pre-Phase 2: Hibernate SUSPENDED apps (if app supports hibernation AND
     *              hibernate delay has elapsed since entering SUSPENDED)
     *
     * If the above soft phases freed enough memory, we're done.
     * Otherwise proceed with hard termination (only if allowTermination is true):
     *
     * Phase 1: Terminate PAUSED apps (oldest first, skip preloaded)
     * Phase 2: Terminate SUSPENDED apps (oldest first, skip preloaded)
     * Phase 3: Terminate HIBERNATED apps (oldest first, skip preloaded)
     *
     * After each full pass, if any action was taken, repeat.
     * If no action was taken in a full pass, stop.
     */
    int iteration = 0;
    while(true)
    {
        int actionsPerformed = 0;

        LOGINFO("[MemoryMonitor] Reclamation pass %d", iteration + 1);

        /* Pre-Phase 1: Suspend PAUSED apps (if suspendable) */
        if (ResolveSuspend(targetRAMKB, activeId, actionsPerformed))
        {
            LOGINFO("[MemoryMonitor] RAM target met after Pre-Phase 1 (Suspend Paused), pass %d.", iteration + 1);
            LOGINFO("[MemoryMonitor] RAM Reconciliation Finished. Final MemAvailable: %u KB", _parent->ReadMemAvailable());
            return true;
        }

        /* Pre-Phase 2: Hibernate SUSPENDED apps (if hibernatable & delay elapsed) */
        if (ResolveHibernate(targetRAMKB, activeId, actionsPerformed))
        {
            LOGINFO("[MemoryMonitor] RAM target met after Pre-Phase 2 (Hibernate Suspended), pass %d.", iteration + 1);
            LOGINFO("[MemoryMonitor] RAM Reconciliation Finished. Final MemAvailable: %u KB", _parent->ReadMemAvailable());
            return true;
        }

        /* Phase 1: Terminate PAUSED apps (longest-in-state first, skip preloaded) */
        if (allowTermination)
        {
            if (Resolve(Exchange::IAppManager::APP_STATE_PAUSED, "TERMINATE", targetRAMKB, activeId, actionsPerformed))
            {
                LOGINFO("[MemoryMonitor] RAM target met after Phase 1 (Terminate Paused), pass %d.", iteration + 1);
                LOGINFO("[MemoryMonitor] RAM Reconciliation Finished. Final MemAvailable: %u KB", _parent->ReadMemAvailable());
                return true;
            }

            /* Phase 2: Terminate SUSPENDED apps (longest-in-state first, skip preloaded) */
            if (Resolve(Exchange::IAppManager::APP_STATE_SUSPENDED, "TERMINATE", targetRAMKB, activeId, actionsPerformed))
            {
                LOGINFO("[MemoryMonitor] RAM target met after Phase 2 (Terminate Suspended), pass %d.", iteration + 1);
                LOGINFO("[MemoryMonitor] RAM Reconciliation Finished. Final MemAvailable: %u KB", _parent->ReadMemAvailable());
                return true;
            }

            /* Phase 3: Terminate HIBERNATED apps (longest-in-state first, skip preloaded) */
            if (Resolve(Exchange::IAppManager::APP_STATE_HIBERNATED, "TERMINATE", targetRAMKB, activeId, actionsPerformed))
            {
                LOGINFO("[MemoryMonitor] RAM target met after Phase 3 (Terminate Hibernated), pass %d.", iteration + 1);
                LOGINFO("[MemoryMonitor] RAM Reconciliation Finished. Final MemAvailable: %u KB", _parent->ReadMemAvailable());
                return true;
            }
        } /* allowTermination */
        else
        {
            LOGINFO("[MemoryMonitor] Termination phases skipped (allowTermination=false), pass %d.", iteration + 1);
        }

        if (actionsPerformed == 0)
        {
            LOGINFO("[MemoryMonitor] No actions performed in pass %d — nothing more to reclaim.", iteration + 1);
            break;
        }

        LOGINFO("[MemoryMonitor] Pass %d performed %d action(s), target not met yet — repeating phases.",
                iteration + 1, actionsPerformed);
    }

    LOGWARN("[MemoryMonitor] RAM Reconciliation FAILED. Target: %u KB, Final MemAvailable: %u KB",
            targetRAMKB, _parent->ReadMemAvailable());
    return false;
}

bool MemoryMonitor::Resolve(Exchange::IAppManager::AppLifecycleState state,
                                   const string& action,
                                   uint32_t targetRAMKB,
                                   const string& ignoreId,
                                   int& actionsPerformed)
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

        LOGINFO("[MemoryMonitor] DECISION: Selecting %s for %s. Reason: Last in ACTIVE state %ld seconds ago.",
                appId.c_str(), action.c_str(), elapsed);

        if (_parent->IsPreloadedApp(appId))
        {
            LOGINFO("[MemoryMonitor] Skipping TERMINATE for preloaded app %s", appId.c_str());
            continue;
        }
        else if (appId == "com.entos.flutter_ref_app")
        {
            LOGINFO("[MemoryMonitor] Skipping TERMINATE for special app %s (protected from termination)",
                    appId.c_str());
            continue;
        }

        _parent->KillApp(appId);
        Exchange::IAppManager::AppLifecycleState nextState = Exchange::IAppManager::APP_STATE_UNLOADED;

        actionsPerformed++;

        /* Wait for the state transition instead of sleeping */
        if (WaitForState(appId, nextState))
        {
            LOGINFO("[MemoryMonitor] Successfully transitioned %s to target state %d.",
                    appId.c_str(), static_cast<int>(nextState));
            // Give artifical extra time after state change to allow memory to stabilize before checking MemAvailable again
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        else
        {
            LOGWARN("[MemoryMonitor] TIMEOUT (%ds) reached waiting for %s to transition to state %d.",
                    STATE_WAIT_TIMEOUT_SECONDS, appId.c_str(), static_cast<int>(nextState));
        }

        _parent->LogManagedAppsSnapshot();

        if (_parent->ReadMemAvailable() >= targetRAMKB)
        {
            LOGINFO("[MemoryMonitor] Target RAM %u KB reached. Current MemAvailable: %u KB",
                    targetRAMKB, _parent->ReadMemAvailable());
            return true;
        }
        else
        {
            LOGINFO("[MemoryMonitor] Target RAM %u KB not yet reached after transitioning %s. Current MemAvailable: %u KB",
                    targetRAMKB, appId.c_str(), _parent->ReadMemAvailable());
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

bool MemoryMonitor::ResolveSuspend(uint32_t targetRAMKB,
                                   const string& ignoreId,
                                   int& actionsPerformed)
{
    std::vector<string> candidates = _parent->GetSortedCandidates(Exchange::IAppManager::APP_STATE_PAUSED);

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

        if (!AppPolicyConfig::IsAppSuspendable(appId))
        {
            LOGINFO("[MemoryMonitor] Skipping SUSPEND for %s (not suspendable)", appId.c_str());
            continue;
        }

        struct timespec ts = _parent->GetAppStateTime(appId);
        long elapsed = _parent->GetElapsedTimeSeconds(ts);

        LOGINFO("[MemoryMonitor] PRE-PHASE: Suspending PAUSED app %s (last active %ld seconds ago).",
                appId.c_str(), elapsed);

        _parent->SuspendApp(appId);
        actionsPerformed++;

        if (WaitForState(appId, Exchange::IAppManager::APP_STATE_SUSPENDED))
        {
            LOGINFO("[MemoryMonitor] Successfully suspended %s.", appId.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        else
        {
            LOGWARN("[MemoryMonitor] TIMEOUT (%ds) waiting for %s to reach SUSPENDED.",
                    STATE_WAIT_TIMEOUT_SECONDS, appId.c_str());
        }

        _parent->LogManagedAppsSnapshot();

        if (_parent->ReadMemAvailable() >= targetRAMKB)
        {
            LOGINFO("[MemoryMonitor] Target RAM %u KB reached after suspending %s. Current MemAvailable: %u KB",
                    targetRAMKB, appId.c_str(), _parent->ReadMemAvailable());
            return true;
        }
        else
        {
            LOGINFO("[MemoryMonitor] Target RAM %u KB not yet reached after suspending %s. Current MemAvailable: %u KB",
                    targetRAMKB, appId.c_str(), _parent->ReadMemAvailable());
        }
    }
    return false;
}

bool MemoryMonitor::ResolveHibernate(uint32_t targetRAMKB,
                                     const string& ignoreId,
                                     int& actionsPerformed)
{
    /*
     * Loop: on each iteration, scan all SUSPENDED hibernatable candidates.
     *  - If any are eligible (delay elapsed) → hibernate them immediately.
     *  - If none are eligible but some are pending (delay not yet elapsed) →
     *    wait for the nearest one to become eligible, then loop again.
     *  - If no hibernatable candidates exist at all → return false.
     */
    while (_running.load())
    {
        if (_parent->ReadMemAvailable() >= targetRAMKB)
        {
            return true;
        }

        std::vector<string> candidates = _parent->GetSortedCandidates(Exchange::IAppManager::APP_STATE_SUSPENDED);

        bool hibernatedAny = false;
        long shortestWaitSec = -1;    /* -1 = no pending candidates */
        string nearestAppId;

        for (const auto& appId : candidates)
        {
            if (appId == ignoreId)
            {
                continue;
            }

            if (!AppPolicyConfig::IsAppHibernatable(appId))
            {
                LOGINFO("[MemoryMonitor] Skipping HIBERNATE for %s (not hibernatable)", appId.c_str());
                continue;
            }

            /* Calculate how long the app has been SUSPENDED */
            int requiredDelaySec = AppPolicyConfig::GetHibernateDelayTimeSeconds(appId);
            long elapsedInSuspended = 0;
            {
                std::lock_guard<std::mutex> lock(_stateMutex);
                auto it = _suspendedTimestamps.find(appId);
                if (it != _suspendedTimestamps.end())
                {
                    elapsedInSuspended = _parent->GetElapsedTimeSeconds(it->second);
                }
                else
                {
                    /* No recorded timestamp — use the generic state time as fallback */
                    struct timespec ts = _parent->GetAppStateTime(appId);
                    elapsedInSuspended = _parent->GetElapsedTimeSeconds(ts);
                }
            }

            if (elapsedInSuspended < requiredDelaySec)
            {
                /* Not eligible yet — track the one with the shortest remaining wait */
                long remainingSec = requiredDelaySec - elapsedInSuspended;
                LOGINFO("[MemoryMonitor] HIBERNATE pending for %s (suspended %ld sec, need %d sec, remaining %ld sec)",
                        appId.c_str(), elapsedInSuspended, requiredDelaySec, remainingSec);
                if (shortestWaitSec < 0 || remainingSec < shortestWaitSec)
                {
                    shortestWaitSec = remainingSec;
                    nearestAppId = appId;
                }
                continue;
            }

            /* Eligible — hibernate now */
            LOGINFO("[MemoryMonitor] PRE-PHASE: Hibernating SUSPENDED app %s (suspended %ld sec, delay %d sec).",
                    appId.c_str(), elapsedInSuspended, requiredDelaySec);

            _parent->HibernateApp(appId);
            actionsPerformed++;
            hibernatedAny = true;

            if (WaitForState(appId, Exchange::IAppManager::APP_STATE_HIBERNATED))
            {
                LOGINFO("[MemoryMonitor] Successfully hibernated %s.", appId.c_str());
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            else
            {
                LOGWARN("[MemoryMonitor] TIMEOUT (%ds) waiting for %s to reach HIBERNATED.",
                        STATE_WAIT_TIMEOUT_SECONDS, appId.c_str());
            }

            _parent->LogManagedAppsSnapshot();

            if (_parent->ReadMemAvailable() >= targetRAMKB)
            {
                LOGINFO("[MemoryMonitor] Target RAM %u KB reached after hibernating %s. Current MemAvailable: %u KB",
                        targetRAMKB, appId.c_str(), _parent->ReadMemAvailable());
                return true;
            }
            else
            {
                LOGINFO("[MemoryMonitor] Target RAM %u KB not yet reached after hibernating %s. Current MemAvailable: %u KB",
                        targetRAMKB, appId.c_str(), _parent->ReadMemAvailable());
            }
        }

        /* After scanning all candidates for this iteration: */
        if (hibernatedAny)
        {
            /* We hibernated at least one app but target not met.
             * Loop again — there may be more eligible or newly eligible candidates. */
            continue;
        }

        if (shortestWaitSec > 0)
        {
            /* There are hibernatable apps pending but none were eligible yet.
             * Wait for the nearest one to become eligible, then retry. */
            LOGINFO("[MemoryMonitor] Waiting %ld sec for next hibernatable app %s to become eligible.",
                    shortestWaitSec, nearestAppId.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(shortestWaitSec));
            /* Loop again to re-evaluate */
            continue;
        }

        /* No hibernatable candidates at all — nothing more we can do here */
        LOGINFO("[MemoryMonitor] No hibernatable SUSPENDED candidates available.");
        break;
    }
    return false;
}

} /* namespace Plugin */
} /* namespace WPEFramework */
