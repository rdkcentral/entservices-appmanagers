/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/


#pragma once

#include "Module.h"
#include <interfaces/IResourceMonitor.h>
#include <interfaces/IVictimSelector.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/IAppManager.h>
#include <interfaces/IRuntimeManager.h>
#include "ResourceMonitorConfig.h"
#include "PackageLifecycleConfigProvider.h"
#include <atomic>
#include <memory>
#include <thread>
#include <list>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <map>

namespace WPEFramework {
namespace Plugin {

    class ResourceMonitorImplementation : public Exchange::IResourceMonitor,
                                          public Exchange::IConfiguration {
    public:
        ResourceMonitorImplementation(const ResourceMonitorImplementation&) = delete;
        ResourceMonitorImplementation& operator=(const ResourceMonitorImplementation&) = delete;

        ResourceMonitorImplementation();
        ~ResourceMonitorImplementation();

        BEGIN_INTERFACE_MAP(ResourceMonitorImplementation)
            INTERFACE_ENTRY(Exchange::IResourceMonitor)
            INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP
        
        Core::hresult GetMemInfo(uint64_t& memAvailable, uint64_t& swapFree, uint64_t& swapTotal) override;
        Core::hresult GetPsiMetrics(const string& metric, double& value, uint64_t& total) override;
        Core::hresult GetSwapUsed(uint64_t& swapUsed, uint64_t& memUsedTotal) override;
        Core::hresult GetFlashSpace(uint64_t& total, uint64_t& used, uint64_t& available) override;
        Core::hresult GetStats(Exchange::ResourceMonitorStats& stats) override;
        Core::hresult Reconcile(const string& appId, const uint32_t ramTargetMB, const bool allowTerminate) override;
        Core::hresult Register(Exchange::IResourceMonitor::INotification* notification) override;
        Core::hresult Unregister(Exchange::IResourceMonitor::INotification* notification) override;

        // Lifecycle capabilities from Package Manager (internal; not exposed over COM-RPC or JSON-RPC).
        // See PackageLifecycleConfigProvider for the usage contract. Do not call while holding
        // _adminLock, _reconcileMutex or _reactiveMonitorMutex.
        Core::hresult GetAppLifecycleCapabilities(const string& appId, AppLifecycleCapabilities& capabilities,
            const string& version = string());
        Core::hresult IsAppLifecycleStateSupported(const string& appId, const AppLifecycleState state, bool& supported,
            const string& version = string());
        Core::hresult GetAppSuspendedStateConfig(const string& appId, SuspendedStateConfig& config,
            const string& version = string());
        static ResourceMonitorImplementation* _instance;

    private:
        class VictimSelectorNotification : public Exchange::IVictimSelector::INotification {
        public:
            explicit VictimSelectorNotification(ResourceMonitorImplementation* parent) : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            BEGIN_INTERFACE_MAP(VictimSelectorNotification)
                INTERFACE_ENTRY(Exchange::IVictimSelector::INotification)
            END_INTERFACE_MAP

            void OnEvictComplete(const bool evicted, const Exchange::IVictimSelector::EvictErrorReason errorCode) override
            {
                _parent.OnEvictComplete(evicted, errorCode);
            }
        private:
            ResourceMonitorImplementation& _parent;
        };

        class AppManagerNotification : public Exchange::IAppManager::INotification {
        public:
            explicit AppManagerNotification(ResourceMonitorImplementation* parent) : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            BEGIN_INTERFACE_MAP(AppManagerNotification)
                INTERFACE_ENTRY(Exchange::IAppManager::INotification)
            END_INTERFACE_MAP

            void OnAppLifecycleStateChanged(const string& appId, const string& appInstanceId,
                const Exchange::IAppManager::AppLifecycleState newState,
                const Exchange::IAppManager::AppLifecycleState oldState,
                const Exchange::IAppManager::AppErrorReason errorReason) override
            {
                _parent.OnAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
            }
        private:
            ResourceMonitorImplementation& _parent;
        };

        enum class EvictionOwner {
            NONE,
            REACTIVE,
            RECONCILE
        };

        // Lifecycle enforcement timer state per app instance
        struct AppTimerState {
            uint64_t pausedTimerExpiryMs;  // Expiry time in milliseconds (0 = inactive)
            uint64_t suspendedTimerExpiryMs;  // Expiry time in milliseconds (0 = inactive)
            AppLifecycleCapabilities lifecycleSnapshot;
            bool hasSnapshot;
            Exchange::IAppManager::AppLifecycleState pausedExpectedState;  // State when PAUSED timer started
            Exchange::IAppManager::AppLifecycleState suspendedExpectedState;  // State when SUSPENDED timer started
            
            AppTimerState()
                : pausedTimerExpiryMs(0)
                , suspendedTimerExpiryMs(0)
                , hasSnapshot(false)
                , pausedExpectedState(Exchange::IAppManager::APP_STATE_UNKNOWN)
                , suspendedExpectedState(Exchange::IAppManager::APP_STATE_UNKNOWN)
            {
            }
        };

        Core::hresult SetConfig(const string& config);
        void ReactiveMonitorLoop();
        void EvaluateResourceThresholds(bool memInfoValid, uint64_t memAvailable, uint64_t swapFree, uint64_t swapTotal, double psiFullAvg60);
        bool IsSoftThresholdExceeded(uint64_t memAvailable, uint64_t swapFree, uint64_t swapTotal, double psiFullAvg60) const;
        bool IsHardThresholdExceeded(uint64_t memAvailable, uint64_t swapFree, uint64_t swapTotal, double psiFullAvg60) const;
        bool IsWarningThresholdExceeded(uint64_t memAvailable, uint64_t swapFree, uint64_t swapTotal, double psiFullAvg60) const;
        uint32_t CalculateSwapPercentage(uint64_t swapFree, uint64_t swapTotal) const;
        void OnReconciliationComplete(const string& appId, const bool targetRamAchieved);
        void OnEvictComplete(const bool evicted, const Exchange::IVictimSelector::EvictErrorReason errorCode);
        void OnAppLifecycleStateChanged(const string& appId, const string& appInstanceId,
            const Exchange::IAppManager::AppLifecycleState newState,
            const Exchange::IAppManager::AppLifecycleState oldState,
            const Exchange::IAppManager::AppErrorReason errorReason);
        Core::hresult Evict(const std::string& reason, const std::string& action, EvictionOwner owner);
        Core::hresult Configure(PluginHost::IShell* service);
        Core::hresult CalculateDirectorySize(const std::string& path, uint64_t& totalSize);

        // Timer management
        void OnPausedTimerExpired(const string& appInstanceId);
        void OnSuspendedTimerExpired(const string& appInstanceId);
        void StartPausedTimer(const string& appInstanceId, const AppLifecycleCapabilities& capabilities);
        void StartSuspendedTimer(const string& appInstanceId);
        void CancelPausedTimer(const string& appInstanceId);
        void CancelSuspendedTimer(const string& appInstanceId);
        void CancelAllTimers(const string& appInstanceId);

        // Enforcement helpers
        void RequestSuspendedTransition(const string& appInstanceId);
        void RequestHibernatedTransition(const string& appInstanceId);
        void RequestAppTermination(const string& appInstanceId, const string& reason);
        bool IsNonAppWorkload(const string& appInstanceId);
        Exchange::IAppManager::AppLifecycleState GetAppState(const string& appInstanceId);
        uint64_t GetGPUMemoryUsage(const string& appInstanceId);

        struct ReconcileRequest {
            string appId;
            uint64_t appMemoryLimitKb;
            bool allowTerminate;
        };
        
        void ProcessReconcileRequest(const ReconcileRequest& request);
        void CompleteReconcile(const bool targetRamAchieved);
        void ReconcileWorker();
        std::list<Exchange::IResourceMonitor::INotification*> _notifications;
        uint64_t _peakSystemMemUsed;
        uint64_t _peakGpuMemUsed;
        uint64_t _peakSwapUsed;
        uint64_t _peakFlashSpaceUsed;

        ResourceMonitorConfig _config;
        std::atomic<bool> _reactiveMonitorRunning;
        std::thread _reactiveMonitorThread;
        std::mutex _reactiveMonitorMutex;
        std::condition_variable _reactiveMonitorCv;

        uint64_t _warningStartTime;
        bool _warningPrinted;
        uint64_t _psiThresholdExceededSince;
        uint64_t _lastPsiActionTime;

        uint64_t _reconcileAppSystemMemoryLimitKb;
        bool _reconcileInProgress;
        std::atomic<bool> _workerRunning;
        std::atomic<bool> _evictionInProgress;
        std::atomic<bool> _initialized;
        std::string _reconcileAppId;
        std::queue<ReconcileRequest> _reconcileRequests;
        std::thread _reconcileThread;
        std::condition_variable _reconcileConditionVariable;
        std::mutex _reconcileMutex;

        std::atomic<EvictionOwner> _evictionOwner;
        Exchange::IVictimSelector* _victimSelector;
        Core::Sink<VictimSelectorNotification> _victimSelectorNotification;
        std::unique_ptr<PackageLifecycleConfigProvider> _lifecycleConfigProvider;

        // AppManager lifecycle event subscription
        Exchange::IAppManager* _appManager;
        Core::Sink<AppManagerNotification> _appManagerNotification;

        // RuntimeManager for querying container info (GPU/memory stats)
        Exchange::IRuntimeManager* _runtimeManager;

        // Per-app timer state for lifecycle enforcement
        std::map<std::string, AppTimerState> _appTimers;
        std::mutex _appTimersMutex;

        // Timer worker for lifecycle enforcement
        std::atomic<bool> _timerWorkerRunning;
        std::thread _timerWorkerThread;
        std::condition_variable _timerWorkerCv;
        std::mutex _timerWorkerMutex;

        void TimerWorkerLoop();

    protected:
        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _rservice;   
    };

} // namespace Plugin
} // namespace WPEFramework
