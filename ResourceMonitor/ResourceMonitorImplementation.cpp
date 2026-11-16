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


#include "ResourceMonitorImplementation.h"
#include "ResourceMonitorConfig.h"
#include "UtilsLogging.h"
#include "UtilsMemorySize.h"
#include <tracing/Logging.h>

#include <stdlib.h>
#include <errno.h>
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <dirent.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <algorithm>
#include <sstream>

namespace WPEFramework {
    namespace Plugin {
    SERVICE_REGISTRATION(ResourceMonitorImplementation, 1, 0, 0)
    ResourceMonitorImplementation* ResourceMonitorImplementation::_instance = nullptr;
        ResourceMonitorImplementation::ResourceMonitorImplementation()
            : _peakSystemMemUsed(0)
            , _peakGpuMemUsed(0)
            , _peakSwapUsed(0)
            , _peakFlashSpaceUsed(0)
            , _reactiveMonitorRunning(false)
            , _warningStartTime(0)
            , _warningPrinted(false)
            , _psiThresholdExceededSince(0)
            , _lastPsiActionTime(0)
            , _reconcileAppSystemMemoryLimitKb(0)
            , _reconcileInProgress(false)
            , _workerRunning(true)
            , _evictionInProgress(false)
            , _initialized(false)
            , _reconcileAppId("")
            , _reconcileEvictionRetryPending(false)
            , _evictionOwner(EvictionOwner::NONE)
            , _pendingEvictionType(Exchange::IVictimSelector::EVICTION_TYPE_SOFT)
            , _victimSelector(nullptr)
            , _victimSelectorNotification(this)
            , _lifecycleConfigProvider()
            , _appManager(nullptr)
            , _appManagerNotification(this)
            , _runtimeManager(nullptr)
            , _managerStopping(false)
            , _managerNotification(this)
            , _appTimersMutex()
            , _nextTimerGeneration(0)
            , _timerStopping(false)
            , _timer(Core::ProxyType<LifecycleTimer>::Create(this))
            , _timerHandler(1024 * 64, _T("ResourceLifecycleTimer"))
            , _adminLock()
            , _rservice(nullptr)
            {
                LOGINFO("ResourceMonitorImplementation constructor invoked");
                _instance = this;
            }

        ResourceMonitorImplementation::~ResourceMonitorImplementation() 
        {
            LOGINFO("ResourceMonitorImplementation Destructor");
            {
                std::lock_guard<std::mutex> lock(_managerMutex);
                _managerStopping = true;
            }
            if (_rservice != nullptr) { _rservice->Unregister(&_managerNotification); }
            {
                std::lock_guard<std::mutex> lock(_appTimersMutex);
                _timerStopping = true;
            }
            _timerHandler.Revoke(*_timer);
            {
                std::lock_guard<std::mutex> lock(_reactiveMonitorMutex);
                _reactiveMonitorRunning = false;
            }

            _reactiveMonitorCv.notify_all();
            if (_reactiveMonitorThread.joinable()) {
                _reactiveMonitorThread.join();
            }
            {
                std::lock_guard<std::mutex> lock(_reconcileMutex);
                _workerRunning.store(false);
                _reconcileConditionVariable.notify_all();
            }

            if (_reconcileThread.joinable()) {
                _reconcileThread.join();
            }
            if (_victimSelector != nullptr) {
                _victimSelector->Unregister(&_victimSelectorNotification);
                _victimSelector->Release();
                _victimSelector = nullptr;
                LOGINFO("VictimSelector notification unregistered");
            }
            ManagerStateChanged("org.rdk.AppManager", false);
            ManagerStateChanged("org.rdk.RuntimeManager", false);
			std::list<Exchange::IResourceMonitor::INotification*> notifications;
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                notifications.swap(_notifications);
            }
            for (auto* notification : notifications) {
                notification->Release();
            }
            _lifecycleConfigProvider.reset();
            if (_rservice != nullptr) {
                _rservice->Release();
                _rservice = nullptr;
            }
            if (_instance == this) {
                _instance = nullptr;
            }
        }

        ResourceMonitorImplementation::AppManagerReference ResourceMonitorImplementation::AcquireAppManager()
        {
            std::lock_guard<std::mutex> lock(_managerMutex);
            auto* manager = _managerStopping ? nullptr : _appManager;
            if (manager != nullptr) { manager->AddRef(); }
            return AppManagerReference(manager);
        }

        ResourceMonitorImplementation::RuntimeManagerReference ResourceMonitorImplementation::AcquireRuntimeManager()
        {
            std::lock_guard<std::mutex> lock(_managerMutex);
            auto* manager = _managerStopping ? nullptr : _runtimeManager;
            if (manager != nullptr) { manager->AddRef(); }
            return RuntimeManagerReference(manager);
        }

        void ResourceMonitorImplementation::ManagerStateChanged(const string& callsign, bool active)
        {
            const bool appManager = callsign == "org.rdk.AppManager";
            if (!appManager && callsign != "org.rdk.RuntimeManager") { return; }
            std::lock_guard<std::mutex> update(_managerUpdateMutex);
            if (active) {
                {
                    std::lock_guard<std::mutex> lock(_managerMutex);
                    if (_managerStopping || (appManager ? _appManager != nullptr : _runtimeManager != nullptr)) { return; }
                }
                if (_rservice == nullptr) { return; }
                if (appManager) {
                    AppManagerReference candidate(_rservice->QueryInterfaceByCallsign<Exchange::IAppManager>(callsign));
                    if (!candidate || candidate->Register(&_appManagerNotification) != Core::ERROR_NONE) { return; }
                    {
                        std::lock_guard<std::mutex> lock(_managerMutex);
                        if (!_managerStopping) { _appManager = candidate.release(); }
                    }
                    if (candidate) {
                        candidate->Unregister(&_appManagerNotification);
                    } else {
                        auto manager = AcquireAppManager();
                        Exchange::IAppManager::ILoadedAppInfoIterator* loaded = nullptr;
                        const auto result = manager ? manager->GetLoadedApps(loaded) : Core::ERROR_UNAVAILABLE;
                        std::unique_ptr<Exchange::IAppManager::ILoadedAppInfoIterator,
                            ReleaseInterface<Exchange::IAppManager::ILoadedAppInfoIterator>> apps(loaded);
                        if (result == Core::ERROR_NONE && apps) {
                            Exchange::IAppManager::LoadedAppInfo info;
                            while (apps->Next(info)) {
                                OnAppLifecycleStateChanged(info.appId, info.appInstanceId, info.lifecycleState,
                                    Exchange::IAppManager::APP_STATE_UNKNOWN, Exchange::IAppManager::APP_ERROR_NONE);
                            }
                        }
                    }
                } else {
                    RuntimeManagerReference candidate(_rservice->QueryInterfaceByCallsign<Exchange::IRuntimeManager>(callsign));
                    std::lock_guard<std::mutex> lock(_managerMutex);
                    if (!_managerStopping) { _runtimeManager = candidate.release(); }
                }
            } else {
                AppManagerReference previousApp;
                RuntimeManagerReference previousRuntime;
                {
                    std::lock_guard<std::mutex> lock(_managerMutex);
                    if (appManager) {
                        previousApp.reset(_appManager);
                        _appManager = nullptr;
                    } else {
                        previousRuntime.reset(_runtimeManager);
                        _runtimeManager = nullptr;
                    }
                }
                if (previousApp) { previousApp->Unregister(&_appManagerNotification); }
            }
        }

        Core::hresult ResourceMonitorImplementation::Configure(PluginHost::IShell* service)
        {
            SYSLOG(Logging::Startup, (_T("VictimSelector Integration entry")));
            Core::hresult result = Core::ERROR_BAD_REQUEST;
            if (nullptr == service) {
                SYSLOG(Logging::Startup, (_T("service is not valid")));
                LOGERR("VictimSelector configuration failed: service is null");
            } else {
                _rservice = service;
                _rservice->AddRef();
                if (!_lifecycleConfigProvider) {
                    _lifecycleConfigProvider.reset(new PackageLifecycleConfigProvider(
                        std::unique_ptr<IPackageLifecycleSource>(new PackageManagerLifecycleSource(_rservice))));
                }
                _victimSelector = _rservice->QueryInterfaceByCallsign<WPEFramework::Exchange::IVictimSelector>("org.rdk.VictimSelector");
                if (_victimSelector == nullptr) {
                    LOGERR("VictimSelector Interface is unavailable. ResourceMonitor will continue without Evict support");
                    result = Core::ERROR_NONE;
                } else {
                    const auto registerResult = _victimSelector->Register(&_victimSelectorNotification);
                    if (registerResult != Core::ERROR_NONE) {
                        LOGERR("VictimSelector notification registration failed. Evict support disabled");
                        _victimSelector->Release();
                        _victimSelector = nullptr;
                        result = Core::ERROR_GENERAL;
                    } else {
                        LOGINFO("VictimSelector notification registered");
                        result = Core::ERROR_NONE;
                    }
                }
                // Subscribe to AppManager lifecycle events for lifecycle enforcement
                _rservice->Register(&_managerNotification);
                ManagerStateChanged("org.rdk.AppManager", true);
                // Continue without lifecycle enforcement - this is not a fatal error
                // Query RuntimeManager for container info (GPU/memory stats)
                ManagerStateChanged("org.rdk.RuntimeManager", true);
            }
            if (!_reactiveMonitorRunning) {
                _reactiveMonitorRunning = true;
               _reactiveMonitorThread = std::thread(&ResourceMonitorImplementation::ReactiveMonitorLoop, this);
            }

            if (!_reconcileThread.joinable()) {
                _reconcileThread = std::thread(&ResourceMonitorImplementation::ReconcileWorker, this);
            }
            _initialized.store(true);
            SYSLOG(Logging::Startup, (_T("VictimSelector exit: result=%d"), result));
            return result;
        }

        Core::hresult ResourceMonitorImplementation::SetConfig(const string& config)
        {
            if (_initialized.load()) {
                LOGERR("SetConfig is not allowed after initialization");
                return Core::ERROR_ILLEGAL_STATE;
            }
            ResourceMonitorConfigJSON configJSON;
            if (!config.empty()) {
                if (!configJSON.FromString(config)) {
                    LOGERR("ResourceMonitorImplementation: invalid configuration");
                    return Core::ERROR_BAD_REQUEST;
                }
            }
            _config.Load(configJSON);
            LOGINFO("ResourceMonitorImplementation configuration loaded");
            return Core::ERROR_NONE;

        }

        Core::hresult ResourceMonitorImplementation::GetAppLifecycleCapabilities(const string& appId,
            AppLifecycleCapabilities& capabilities, const string& version)
        {
            if (!_lifecycleConfigProvider) {
                capabilities = AppLifecycleCapabilities();
                return appId.empty() ? Core::ERROR_INVALID_PARAMETER : Core::ERROR_NONE;
            }
            return _lifecycleConfigProvider->GetLifecycleCapabilities(appId, capabilities, version);
        }

        Core::hresult ResourceMonitorImplementation::IsAppLifecycleStateSupported(const string& appId,
            const AppLifecycleState state, bool& supported, const string& version)
        {
            AppLifecycleCapabilities capabilities;
            const Core::hresult result = GetAppLifecycleCapabilities(appId, capabilities, version);
            supported = (result == Core::ERROR_NONE) && capabilities.IsSupported(state);
            return result;
        }

        Core::hresult ResourceMonitorImplementation::GetAppSuspendedStateConfig(const string& appId,
            SuspendedStateConfig& config, const string& version)
        {
            AppLifecycleCapabilities capabilities;
            const Core::hresult result = GetAppLifecycleCapabilities(appId, capabilities, version);
            config = capabilities.suspended;
            return result;
        }

        Core::hresult ResourceMonitorImplementation::Register(WPEFramework::Exchange::IResourceMonitor::INotification* notification)
        {
            if (notification == nullptr) {
                return Core::ERROR_BAD_REQUEST;
            }
            _adminLock.Lock();
            if (std::find(_notifications.begin(), _notifications.end(), notification) == _notifications.end()) {
                notification->AddRef();
                _notifications.push_back(notification);
                LOGINFO("ResourceMonitor: notification registered");
            }
            _adminLock.Unlock();
            return Core::ERROR_NONE;
        }

        Core::hresult ResourceMonitorImplementation::Unregister(WPEFramework::Exchange::IResourceMonitor::INotification* notification)
        {
            if (notification == nullptr) {
                return Core::ERROR_BAD_REQUEST;
            }
            _adminLock.Lock();
            auto index = std::find(_notifications.begin(), _notifications.end(), notification);
            if (index != _notifications.end()) {
                (*index)->Release();
                _notifications.erase(index);

                LOGINFO("ResourceMonitor: notification unregistered");
                _adminLock.Unlock();
                return Core::ERROR_NONE;
            }
            _adminLock.Unlock();
            return Core::ERROR_NOT_EXIST;
        }

        void ResourceMonitorImplementation::OnReconciliationComplete(const string& appId, const bool targetRamAchieved)
        {
            LOGINFO("ResourceMonitor: OnReconciliationComplete(appId=%s result=%s)", appId.c_str(), targetRamAchieved ? _T("true") : _T("false"));
            std::list<WPEFramework::Exchange::IResourceMonitor::INotification* > notifications;
            _adminLock.Lock();
            notifications = _notifications;
            for (auto notification : notifications) {
                notification->AddRef();
            }
            _adminLock.Unlock();
            for (auto notification : notifications) {
                notification->OnReconciliationComplete(appId, targetRamAchieved);
                notification->Release();
            }
        }

        Core::hresult ResourceMonitorImplementation::GetMemInfo(uint64_t& memAvailable, uint64_t& swapFree, uint64_t& swapTotal)
        {
            memAvailable = 0;
            swapFree = 0;
            swapTotal = 0;
            std::string key = "";
            uint64_t value = 0;
            std::string unit = "";

            LOGINFO("Entering GetMemInfo()");
            std::ifstream file("/proc/meminfo");

            if (!file.is_open()) {
                LOGERR("Unable to open /proc/meminfo");
                return Core::ERROR_GENERAL;
            }

            while (file >> key >> value >> unit) {
                if (key == "MemAvailable:") {
                    memAvailable = value;
                }
                else if (key == "SwapFree:") {
                    swapFree = value;
                }
                else if (key == "SwapTotal:") {
                    swapTotal = value;
                }
            }

            LOGINFO("MemAvailable = %llu KiB",static_cast<unsigned long long>(memAvailable));
            LOGINFO("SwapFree = %llu KiB",static_cast<unsigned long long>(swapFree));
            LOGINFO("SwapTotal=%llu KiB",static_cast<unsigned long long>(swapTotal));
            return Core::ERROR_NONE;
        }

        Core::hresult ResourceMonitorImplementation::GetPsiMetrics(const string& metric, double& value, uint64_t& total)
        {
            PsiMetric m = _config.GetPsiMetric();
            const char* group = "";
            const char* avg = "";
            std::string line = "";
            std::string token = "";

            if (!metric.empty()) {
                if (metric == "some_avg10")
                    m = PsiMetric::SOME_AVG10;
                else if (metric == "some_avg60")
                    m = PsiMetric::SOME_AVG60;
                else if (metric == "some_avg300")
                    m = PsiMetric::SOME_AVG300;
                else if (metric == "full_avg10")
                    m = PsiMetric::FULL_AVG10;
                else if (metric == "full_avg60")
                    m = PsiMetric::FULL_AVG60;
                else if (metric == "full_avg300")
                    m = PsiMetric::FULL_AVG300;
                else
                    return Core::ERROR_BAD_REQUEST;
            }

            LOGINFO("Entering GetPsiMetrics()");
            value = 0.0;
            total = 0;
            if (!_config.PsiCheckingEnabled())
                return Core::ERROR_UNAVAILABLE;

            switch (m) {
                case PsiMetric::SOME_AVG10:
                    group = "some";
                    avg = "avg10=";
                    break;

                case PsiMetric::SOME_AVG60:
                    group = "some";
                    avg = "avg60=";
                    break;

                case PsiMetric::SOME_AVG300:
                    group = "some";
                    avg = "avg300=";
                    break;

                case PsiMetric::FULL_AVG10:
                    group = "full";
                    avg = "avg10=";
                    break;

                case PsiMetric::FULL_AVG60:
                    group = "full";
                    avg = "avg60=";
                    break;

                case PsiMetric::FULL_AVG300:
                    group = "full";
                    avg = "avg300=";
                    break;

                default:
                    return Core::ERROR_BAD_REQUEST;
            }

            std::ifstream file(_config.PsiPath());

            if (!file.is_open())
                return Core::ERROR_GENERAL;

            while (std::getline(file, line)) {
                std::stringstream ss(line);
                ss >> token;
                if (token != group)
                    continue;
                while (ss >> token) {
                    if (token.rfind(avg, 0) == 0) {
                        try {
                            value = std::stod(token.substr(std::string(avg).length()));
                            continue;
                        }
                        catch (...) {
                            return Core::ERROR_GENERAL;
                        }
                    }
                    if (token.rfind("total=", 0) == 0) {
                        try {
                            total = std::stoull(token.substr(std::string("total=").length()));
                        }
                        catch (...) {
                            return Core::ERROR_GENERAL;
                        }
                    }
                }
                return Core::ERROR_NONE;
            }
            return Core::ERROR_UNKNOWN_KEY;
        }

        Core::hresult ResourceMonitorImplementation::CalculateDirectorySize(const std::string& path, uint64_t& totalSize)
        {
            totalSize = 0;
            DIR* dir = opendir(path.c_str());
            if (!dir) {
                LOGERR("Failed to open directory %s", path.c_str());
                return Core::ERROR_GENERAL;
            }
            struct dirent* entry = nullptr;
            while ((entry = readdir(dir)) != nullptr) {
                if ((strcmp(entry->d_name, ".") == 0) ||
                    (strcmp(entry->d_name, "..") == 0)) {
                    continue;
                }
                std::string fullPath = path + "/" + entry->d_name;
                struct stat st {};
                if (lstat(fullPath.c_str(), &st) != 0) {
                    LOGERR("lstat failed for %s", fullPath.c_str());
                    closedir(dir);
                    return Core::ERROR_GENERAL;
                }
                if (S_ISDIR(st.st_mode)) {
                    uint64_t childSize = 0;
                    Core::hresult result = CalculateDirectorySize(fullPath, childSize);
                    if (result != Core::ERROR_NONE) {
                        closedir(dir);
                        return result;
                    }
                    totalSize += childSize;
                }
                else {
                    totalSize += static_cast<uint64_t>(st.st_size);
                }
            }
            closedir(dir);
            return Core::ERROR_NONE;
        }

        Core::hresult ResourceMonitorImplementation::GetFlashSpace(uint64_t& total, uint64_t& used, uint64_t& available)
        {
        const std::string& mountPoint = _config.FlashSpaceForHibernatedAppsMountPoint();
        const uint64_t configuredSizeMB = _config.FlashSpaceForHibernatedAppsSize();
        const uint64_t configuredSizeBytes = configuredSizeMB * 1024ULL * 1024ULL;
        uint64_t directoryUsedBytes = 0;
        uint64_t quotaRemaining = 0;
        uint64_t filesystemAvailable = 0;

        LOGINFO("Entering GetFlashSpace()");
    
        // Ensure directory exists
        struct stat statBuffer {};
        if (stat(mountPoint.c_str(), &statBuffer) != 0) {
            if (errno == ENOENT) {
                if (mkdir(mountPoint.c_str(), 0744) != 0 && errno != EEXIST) {
                    LOGERR("Failed to create hibernated apps path=%s", mountPoint.c_str());
                    return Core::ERROR_GENERAL;
                }
                LOGINFO("Created hibernated apps path=%s", mountPoint.c_str());
            } else {
                LOGERR("Failed to access hibernated apps path=%s", mountPoint.c_str());
                return Core::ERROR_GENERAL;
            }
        }
    
        // Query filesystem space
        struct statvfs vfs {};
        if (statvfs(mountPoint.c_str(), &vfs) != 0) {
            LOGERR("statvfs failed for %s, errno=%d", mountPoint.c_str(), errno);
            return Core::ERROR_GENERAL;
        }

        Core::hresult result = CalculateDirectorySize(mountPoint, directoryUsedBytes);
        if (result != Core::ERROR_NONE) {
            LOGERR("Directory size calculation failed");
            return result;
        }

        total = configuredSizeBytes;
        used = directoryUsedBytes;
        quotaRemaining = (used >= total) ? 0 : (total - used);
        filesystemAvailable = static_cast<uint64_t>(vfs.f_bavail) * static_cast<uint64_t>(vfs.f_frsize);
        available = std::min(quotaRemaining, filesystemAvailable);
    
        LOGINFO("Flash mount point=%s Total=%llu Used=%llu Available=%llu",
            mountPoint.c_str(),
            static_cast<unsigned long long>(total),
            static_cast<unsigned long long>(used),
            static_cast<unsigned long long>(available));

        return Core::ERROR_NONE;
        }


        Core::hresult ResourceMonitorImplementation::GetSwapUsed(uint64_t& swapUsed, uint64_t& memUsedTotal)
        {
            uint64_t origDataSize = 0;
            uint64_t comprDataSize = 0;
            swapUsed = 0;
            memUsedTotal = 0;
            LOGINFO("Entering GetSwapUsed()");

            // Read orig_data_size from mm_stat
            std::ifstream mmStat("/sys/block/zram0/mm_stat");

            if (!mmStat.is_open()) {
                LOGERR("Failed to open /sys/block/zram0/mm_stat");
                return Core::ERROR_GENERAL;
            }
            mmStat >> origDataSize >> comprDataSize >> memUsedTotal;
            if (mmStat.fail()) {
                LOGERR("Failed to parse /sys/block/zram0/mm_stat");
                return Core::ERROR_GENERAL;
            }

            swapUsed = origDataSize;
            LOGINFO("SwapUsed = %llu bytes, MemUsedTotal = %llu bytes",
                static_cast<unsigned long long>(swapUsed),
                static_cast<unsigned long long>(memUsedTotal));
            return Core::ERROR_NONE;
        }

        Core::hresult ResourceMonitorImplementation::GetStats(WPEFramework::Exchange::ResourceMonitorStats& stats)
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            uint64_t memAvailable = 0;
            uint64_t swapFree = 0;
            uint64_t swapTotal = 0;
            uint64_t memTotal = 0;
            std::string key = "";
            uint64_t value = 0;
            std::string unit = "";
            uint64_t swapUsed = 0;
            uint64_t memUsedTotal = 0;
            uint64_t flashTotal = 0;
            uint64_t flashUsed = 0;
            uint64_t flashAvailable = 0;
            Core::hresult result;

            LOGINFO("Entering GetStats");
            memset(&stats, 0, sizeof(stats));
            result = GetMemInfo(memAvailable, swapFree, swapTotal);
            if (result != Core::ERROR_NONE) {
                LOGERR("GetStats: GetMemInfo failed");
                return result;
            }

            std::ifstream mem("/proc/meminfo");
            if (!mem.is_open()) {
                LOGERR("GetStats: Failed to open /proc/meminfo");
                return Core::ERROR_GENERAL;
            }
            while (mem >> key >> value >> unit) {
                if (key == "MemTotal:")
                    memTotal = value;
            }
            if (memTotal == 0) {
                LOGERR("GetStats: Failed to read MemTotal");
                return Core::ERROR_GENERAL;
            }

            stats.systemMemTotal = memTotal;
            stats.systemMemAvailable = memAvailable;
            stats.systemMemUsed = memTotal - memAvailable;

            if (stats.systemMemUsed > _peakSystemMemUsed)
            {
                _peakSystemMemUsed = stats.systemMemUsed;
            }
            stats.peakSystemMemUsed = _peakSystemMemUsed;

            // GPU (TO BE IMPLEMENTED)

            stats.gpuMemTotal = 0;
            stats.gpuMemUsed = 0;
            stats.gpuMemAvailable = 0;
            stats.peakGpuMemUsed = _peakGpuMemUsed;

            result = GetSwapUsed(swapUsed, memUsedTotal);
            if (result != Core::ERROR_NONE) {
                LOGERR("GetStats: GetSwapUsed failed");
                return result;
            }

            stats.swapTotal = swapTotal * 1024ULL;
            stats.swapFree = swapFree * 1024ULL;
            stats.swapUsed = swapUsed;

            if (swapUsed > _peakSwapUsed)
            {
                _peakSwapUsed = swapUsed;
            }
            stats.peakSwapUsed = _peakSwapUsed;

            result = GetFlashSpace(flashTotal, flashUsed, flashAvailable);
            if (result != Core::ERROR_NONE) {
                LOGERR("GetStats: GetFlashSpace failed");
                return result;
            }

            stats.flashSpaceTotal = flashTotal;
            stats.flashSpaceUsed = flashUsed;
            stats.flashSpaceAvailable = flashAvailable;

            if (flashUsed > _peakFlashSpaceUsed)
            {
                _peakFlashSpaceUsed = flashUsed;
            }
            stats.peakFlashSpaceUsed = _peakFlashSpaceUsed;
            LOGINFO("Leaving GetStats");
            return Core::ERROR_NONE;
        }

        uint32_t ResourceMonitorImplementation::CalculateSwapPercentage(uint64_t swapFree, uint64_t swapTotal) const
        {
            if (swapTotal == 0) {
                return 0;
            }
            return static_cast<uint32_t>((swapTotal - swapFree) * 100ULL / swapTotal);
        }

        void ResourceMonitorImplementation::ReactiveMonitorLoop()
        {
            uint64_t memAvailable = 0;
            uint64_t swapFree = 0;
            uint64_t swapTotal, total = 0;
            double psiFullAvg60 = 0.0;
            LOGINFO("Reactive monitor started");

            while (_reactiveMonitorRunning.load()) {
                bool memInfoValid = (GetMemInfo(memAvailable, swapFree, swapTotal) == Core::ERROR_NONE);

                if (_config.PsiCheckingEnabled()) {
                    const uint32_t psiResult = GetPsiMetrics("", psiFullAvg60, total);
                    LOGINFO("GOT PSI metrics!");
                    if (psiResult != Core::ERROR_NONE) {
                        LOGERR("Reactive monitor: GetPsiMetrics failed");
                    }
                }

                EvaluateResourceThresholds(memInfoValid, memAvailable, swapFree, swapTotal, psiFullAvg60);
                // Wait until next poll
                std::unique_lock<std::mutex> lock(_reactiveMonitorMutex);
                _reactiveMonitorCv.wait_for(
                    lock,
                    std::chrono::milliseconds(_config.PollingInterval()),
                    [this] {
                    return !_reactiveMonitorRunning.load();
                    });
            }
            LOGINFO("Reactive monitor stopped");
        }

        void ResourceMonitorImplementation::EvaluateResourceThresholds(bool memInfoValid, uint64_t memAvailable, uint64_t swapFree, uint64_t swapTotal, double psiFullAvg60)
        {
            bool warningCondition = false;
            uint32_t swapPercentage = 0;
            uint64_t now = 0, swapUsed = 0, memUsedTotal = 0 , swapTotalBytes = 0;
            bool hardEvictionRequired = false;
            bool softEvictionRequired = false;
            std::string evictionReason;
            std::string evictionAction;
            bool psiEvictionCandidate = false;

            LOGINFO("Low_memory_warning_enabled=%u", _config.LowMemoryWarningsEnabled());
            if (_config.LowMemoryWarningsEnabled()) {
                LOGINFO("INFO memAvailable=%llu", static_cast<unsigned long long>(memAvailable));
                if (memInfoValid) {
                    if (memAvailable <= _config.HardThresholdMinMem()) {
                        LOGWARN("Hard MEMORY threshold exceeded");
                        hardEvictionRequired = true;
                        evictionReason = "HardMemoryThresholdReached";
                        evictionAction = "Kill";
                    }
                    else if (memAvailable <= _config.SoftThresholdMinMem()) {
                        LOGWARN("Soft MEMORY threshold exceeded");
                        if (!hardEvictionRequired) {
                            softEvictionRequired = true;
                            evictionReason = "SoftMemoryThresholdReached";
                            evictionAction = "terminate";
                        }
                    }
                    else if (memAvailable <= _config.WarningThresholdMinMem()) {
                        warningCondition = true;
                    }
                }

                if (_config.ZramCheckingEnabled()) {
                    GetSwapUsed(swapUsed, memUsedTotal);
                    swapTotalBytes = swapTotal * 1024ULL;
                    if (swapTotalBytes > 0) {
                        double percent = (static_cast<double>(swapUsed) * 100.0) / static_cast<double>(swapTotalBytes);
                        swapPercentage = static_cast<uint32_t>(percent + 0.5); // round to nearest
                    }

                    LOGINFO("SWAP percentage: %.2f", (static_cast<double>(swapUsed) * 100.0) / static_cast<double>(swapTotalBytes));
                    if (swapPercentage >= _config.HardThresholdMaxSwapPercentage()) {
                        LOGWARN("Hard SWAP threshold exceeded");
                        hardEvictionRequired = true;
                        evictionReason = "HardSwapThresholdReached";
                        evictionAction = "Kill";
                    }
                    else if (swapPercentage >= _config.SoftThresholdMaxSwapPercentage()) {
                        LOGWARN("Soft SWAP threshold exceeded");
                        if (!hardEvictionRequired && !softEvictionRequired) {
                            softEvictionRequired = true;
                            evictionReason = "SoftSwapThresholdReached";
                            evictionAction = "terminate";
                        }
                    }
                    else if (swapPercentage >= _config.WarningThresholdMaxSwapPercentage()) {
                        warningCondition = true;
                    }
                }

                if (_config.PsiCheckingEnabled()) {
                    LOGINFO("psiFullAvg60=%.2f", psiFullAvg60);
                    const uint64_t nowMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
                    if (psiFullAvg60 >= _config.HardThresholdMaxPsi()) {
                        if (_psiThresholdExceededSince == 0) {
                            _psiThresholdExceededSince = nowMs;
                        }
                        if ((nowMs - _psiThresholdExceededSince) >= _config.PsiExcessDuration()) {
                             if (_lastPsiActionTime == 0 || (nowMs - _lastPsiActionTime) >= _config.PsiPostActionDelay()) {
                                LOGWARN("Hard PSI threshold exceeded");
                                hardEvictionRequired = true;
                                evictionReason = "HardPsiThresholdExceeded";
                                evictionAction = "Kill";
                                psiEvictionCandidate = true;
                            }
                        }
                    }
                    else if (psiFullAvg60 >= _config.SoftThresholdMaxPsi()) {
                        if (_psiThresholdExceededSince == 0) {
                            _psiThresholdExceededSince = nowMs;
                        }
                        if ((nowMs - _psiThresholdExceededSince) >= _config.PsiExcessDuration()) {
                            if (_lastPsiActionTime == 0 || (nowMs - _lastPsiActionTime) >= _config.PsiPostActionDelay()) {
                                LOGWARN("Soft PSI threshold exceeded");
                                if (!hardEvictionRequired) {
                                    softEvictionRequired = true;
                                    evictionReason = "SoftPsiThresholdExceeded";
                                    evictionAction = "terminate";
                                    psiEvictionCandidate = true;
                                }
                            }
                        }
                    }
                    else {
                        _psiThresholdExceededSince = 0;
                        if (psiFullAvg60 >= _config.WarningThresholdMaxPsi()) {
                            warningCondition = true;
                        }
                    }
                }

                if (hardEvictionRequired) {
                    const Core::hresult result = Evict(evictionReason, evictionAction, EvictionOwner::REACTIVE);
                    if (result == Core::ERROR_NONE && psiEvictionCandidate) {
                        _lastPsiActionTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
                    }
                }
                else if (softEvictionRequired) {
                    const Core::hresult result = Evict(evictionReason, evictionAction, EvictionOwner::REACTIVE);
                    if (result == Core::ERROR_NONE && psiEvictionCandidate) {
                        _lastPsiActionTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
                    }
                }

                if (!warningCondition) {
                    _warningStartTime = 0;
                    _warningPrinted = false;
                }
                else {
                    now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
                    if (_warningStartTime == 0) {
                        _warningStartTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
                    }

                    if (!_warningPrinted && ((now - _warningStartTime) >= _config.MinPostWarningDelay())) {
                        LOGWARN("warning threshold exceeded for %u seconds", _config.MinPostWarningDelay());
                        _warningPrinted = true;
                    }
                }
            }
        }

        void ResourceMonitorImplementation::ProcessReconcileRequest(const ReconcileRequest& request)
        {
            uint64_t memAvailable = 0;
            uint64_t swapFree = 0;
            uint64_t swapTotal = 0;
            uint64_t remainingMemory = 0;

            LOGINFO("Processing Reconcile: appId=%s ramTargetMB=%llu allowTerminate=%s", request.appId.c_str(), static_cast<unsigned long long>(request.appMemoryLimitKb / 1024ULL), request.allowTerminate ? _T("true") : _T("false"));

            const Core::hresult result = GetMemInfo(memAvailable, swapFree, swapTotal);

            if (result != Core::ERROR_NONE) {
                LOGERR("Reconcile: GetMemInfo failed");
                CompleteReconcile(false);
                return;
            }

            remainingMemory = (memAvailable > request.appMemoryLimitKb) ? (memAvailable - request.appMemoryLimitKb) : 0;
            LOGINFO("Reconcile: memAvailable=%llu appMemoryLimit=%llu remaining=%llu",
                static_cast<unsigned long long>(memAvailable),
                static_cast<unsigned long long>(request.appMemoryLimitKb),
                static_cast<unsigned long long>(remainingMemory));

            // Enough memory available.
            if (remainingMemory > _config.HardThresholdMinMem()) {
                LOGINFO("Reconcile: sufficient memory");
                CompleteReconcile(true);
                return;
            }

            //Not enough memory and allowTerminate=true.
            if (!request.allowTerminate) {
                LOGWARN("Reconcile: insufficient memory and termination is not allowed");
                CompleteReconcile(false);
                return;
            }

            // Not enough memory and allowTerminate=true -> VictimSelector evict.
            LOGINFO("Reconcile: insufficient memory, requesting VictimSelector eviction");
            const Core::hresult evictionResult = Evict("systemMemoryLow", "terminate", EvictionOwner::RECONCILE);
            if (evictionResult != Core::ERROR_NONE) {
                LOGERR("Reconcile: VictimSelector eviction request failed: %u", evictionResult);
                CompleteReconcile(false);
                return;
            }
        }

        Core::hresult ResourceMonitorImplementation::Reconcile(const string& appId, const uint32_t ramTargetMB, const bool allowTerminate) {
            ReconcileRequest request;
            request.appId = appId;
            request.appMemoryLimitKb = static_cast<uint64_t>(ramTargetMB) * 1024ULL;
            request.allowTerminate = allowTerminate;

            if (appId.empty()) {
                return Core::ERROR_BAD_REQUEST;
            }
            LOGINFO("Entering Reconcile(): appId=%s ramTargetMB=%u allowTerminate=%s", appId.c_str(), ramTargetMB, allowTerminate ? _T("true") : _T("false"));
            size_t queueSize = 0;
            {
                std::lock_guard<std::mutex> lock(_reconcileMutex);
                _reconcileRequests.push(request);
                queueSize = _reconcileRequests.size();
                _reconcileConditionVariable.notify_one();
            }
            LOGINFO("Queued reconcile request for appId=%s, queueSize=%zu", appId.c_str(), queueSize);
            return Core::ERROR_NONE;
        }

        void ResourceMonitorImplementation::ReconcileWorker()
        {
            LOGINFO("Reconcile Worker!");
            while (_workerRunning.load()) {
                ReconcileRequest request;
                {
                    std::unique_lock<std::mutex> lock(_reconcileMutex);
                    _reconcileConditionVariable.wait(
                        lock,
                        [this] {
                            return !_workerRunning ||
                            (!_reconcileInProgress &&
                            !_reconcileRequests.empty()) ||
                            (_reconcileInProgress &&
                            _reconcileEvictionRetryPending);
                        });

                    if (!_workerRunning) {
                        break;
                    }
                    if (_reconcileInProgress && _reconcileEvictionRetryPending) {
                        // Continue the same reconciliation request.
                        request.appId = _reconcileAppId;
                        request.appMemoryLimitKb = _reconcileAppSystemMemoryLimitKb;
                        request.allowTerminate = true;
                        _reconcileEvictionRetryPending = false;
                        LOGINFO("Processing eviction retry for appId=%s", request.appId.c_str());
                    }
                    else {
                        request = _reconcileRequests.front();
                        LOGINFO("POP appId=%s queueSizeBeforePop=%zu", request.appId.c_str(), _reconcileRequests.size());
                        _reconcileRequests.pop();
                        _reconcileInProgress = true;
                        _reconcileAppId = request.appId;
                        _reconcileAppSystemMemoryLimitKb = request.appMemoryLimitKb;
                    }
                }
                ProcessReconcileRequest(request);
            }
        }
        
        void ResourceMonitorImplementation::CompleteReconcile(const bool targetRamAchieved)
        {
            string completedAppId;
            {
                std::lock_guard<std::mutex> lock(_reconcileMutex);
                if (!_reconcileInProgress) {
                    LOGWARN("Ignoring reconciliation completion: no active request");
                    return;
                }
                completedAppId = _reconcileAppId;
                _reconcileAppId.clear();
                _reconcileAppSystemMemoryLimitKb = 0;
                _reconcileInProgress = false;
                _reconcileEvictionRetryPending = false;
            }
            OnReconciliationComplete(completedAppId, targetRamAchieved);
            _reconcileConditionVariable.notify_one();
        }

        Core::hresult ResourceMonitorImplementation::Evict(const std::string& reason, const std::string& action, EvictionOwner owner)
        {
            Exchange::IVictimSelector::EvictionReason evictionReason = Exchange::IVictimSelector::EVICTION_REASON_RAM;
            Exchange::IVictimSelector::EvictionType evictionType;
            LOGINFO("VictimSelector Evict is called!");

            // Determine the requested eviction type before checking whether another eviction is already in progress.
            if (action == "Kill") {
                evictionType = Exchange::IVictimSelector::EVICTION_TYPE_HARD;
            }
            else if (action == "terminate") {
                evictionType = Exchange::IVictimSelector::EVICTION_TYPE_SOFT;
            }
            else {
                LOGWARN("Invalid VictimSelector action: %s", action.c_str());
                return Core::ERROR_BAD_REQUEST;
            }

            if (_victimSelector == nullptr) {
                LOGWARN("VictimSelector is not available");
                return Core::ERROR_UNAVAILABLE;
            }

            // An eviction is already active. Allow only SOFT -> HARD escalation.
            if (_evictionInProgress.load()) {
                if (evictionType == Exchange::IVictimSelector::EVICTION_TYPE_HARD) {
                    auto expected = Exchange::IVictimSelector::EVICTION_TYPE_SOFT;
                    if (_pendingEvictionType.compare_exchange_strong(expected, Exchange::IVictimSelector::EVICTION_TYPE_HARD)) {
                        if (!_evictionInProgress.load() || (_evictionOwner.load() == EvictionOwner::NONE)) {
                            expected = Exchange::IVictimSelector::EVICTION_TYPE_HARD;
                            _pendingEvictionType.compare_exchange_strong(expected, Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
                            LOGWARN("Eviction completed before HARD escalation could start");
                            return Core::ERROR_INPROGRESS;
                        }
                        LOGWARN("Upgrading pending SOFT eviction to HARD");
                        const Core::hresult result = _victimSelector->Evict(evictionReason, evictionType);
                        if (result != Core::ERROR_NONE) {
                            // Restore SOFT only if the original eviction is still active and the type has not changed again.
                            expected = Exchange::IVictimSelector::EVICTION_TYPE_HARD;
                            _pendingEvictionType.compare_exchange_strong(expected, Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
                            LOGERR("VictimSelector HARD escalation failed: %u", result);
                        }
                        // Do not change _evictionOwner. The original eviction remains responsible for completing its operation.
                        return result;
                    }
                }
                LOGWARN("Eviction already in progress; requested eviction cannot be started");
                return Core::ERROR_INPROGRESS;
            }

            // Claim the eviction slot. Another thread may have claimed it after the earlier load.
            bool expectedNotInProgress = false;
            if (!_evictionInProgress.compare_exchange_strong(expectedNotInProgress, true)) {
                LOGWARN("Eviction already in progress");
                return Core::ERROR_INPROGRESS;
            }

            _pendingEvictionType.store(evictionType);
            _evictionOwner.store(owner);
            LOGINFO("Calling VictimSelector::Evict: reason=%s action=%s", reason.c_str(), action.c_str());
            const Core::hresult result = _victimSelector->Evict(evictionReason, evictionType);
            if (result != Core::ERROR_NONE) {
                // VictimSelector can invoke OnEvictComplete synchronously and still return an error. Claim the operation only if its callback has not already claimed it.
                const EvictionOwner pendingOwner = _evictionOwner.exchange(EvictionOwner::NONE);
                if (pendingOwner != EvictionOwner::NONE) {
                    _evictionInProgress.store(false);
                    _pendingEvictionType.store(Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
                    LOGERR("VictimSelector::Evict failed: result=%u", result);
                    return result;
                }

                // The callback already claimed completion. Do not let the caller complete the same reconciliation a second time.
                LOGWARN("Eviction callback already handled the failed request");
                return Core::ERROR_NONE;
            }
            return Core::ERROR_NONE;
        }

        void ResourceMonitorImplementation::OnEvictComplete(const bool evicted, const Exchange::IVictimSelector::EvictErrorReason errorCode)
        {
            string appId = "";
            uint64_t appMemoryLimitKb = 0;
            uint64_t memAvailable = 0;
            uint64_t swapFree = 0;
            uint64_t swapTotal, remainingMemory = 0;
            Core::hresult result = Core::ERROR_NONE;

            EvictionOwner owner = _evictionOwner.exchange(EvictionOwner::NONE);
            if (owner == EvictionOwner::NONE) {
                LOGWARN("Ignoring duplicate or already handled eviction callback");
                return;
            }
            _evictionInProgress.store(false);
            _pendingEvictionType.store(Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
            LOGINFO("VictimSelector::OnEvictComplete: evicted=%s errorCode=%u", evicted ? _T("true") : _T("false"), static_cast<uint32_t>(errorCode));
            // No active Reconcile. This eviction was triggered by the reactive resource monitoring path.
            {
                std::lock_guard<std::mutex> lock(_reconcileMutex);
                if (owner != EvictionOwner::RECONCILE) {
                    LOGINFO("Reactive eviction completed");
                    return;
                }
                appId = _reconcileAppId;
                appMemoryLimitKb = _reconcileAppSystemMemoryLimitKb;
            }

            // VictimSelector could not evict a victim.
            if (!evicted) {
                LOGERR("VictimSelector eviction failed for appId=%s errorCode=%u", appId.c_str(), static_cast<uint32_t>(errorCode));
                CompleteReconcile(false);
                return;
            }

            // VictimSelector successfully completed the eviction. Now check whether enough memory was actually recovered.
            result = GetMemInfo(memAvailable, swapFree, swapTotal);
            if (result != Core::ERROR_NONE) {
                LOGERR("Failed to get memory information after eviction");
                CompleteReconcile(false);
                return;
            }

            remainingMemory = (memAvailable > appMemoryLimitKb) ? (memAvailable - appMemoryLimitKb) : 0;
            LOGINFO(
                "After eviction: appId=%s memAvailable=%llu limit=%llu remaining=%llu",
                appId.c_str(),
                static_cast<unsigned long long>(memAvailable),
                static_cast<unsigned long long>(appMemoryLimitKb),
                static_cast<unsigned long long>(remainingMemory));

            // Reconciliation succeeded.
            if (remainingMemory > _config.HardThresholdMinMem()) {
                LOGINFO("Reconcile successful after VictimSelector eviction");
                CompleteReconcile(true);
                return;
            }

            // Still insufficient memory. Schedule the retry on the reconciliation worker. Do not call VictimSelector again from this callback.
            LOGWARN("Still insufficient memory. Scheduling eviction retry");
            {
                std::lock_guard<std::mutex> lock(_reconcileMutex);
                if (!_reconcileInProgress) {
                    LOGERR("Cannot schedule eviction retry: no active reconciliation");
                    return;
                }
                _reconcileEvictionRetryPending = true;
            }
            _reconcileConditionVariable.notify_one();
        }

        // Lifecycle enforcement: AppManager lifecycle state change handler
        void ResourceMonitorImplementation::OnAppLifecycleStateChanged(const string& appId,
            const string& appInstanceId, const Exchange::IAppManager::AppLifecycleState newState,
            const Exchange::IAppManager::AppLifecycleState oldState, const Exchange::IAppManager::AppErrorReason errorReason)
        {
            LOGINFO("App lifecycle state changed: appId=%s appInstanceId=%s oldState=%d newState=%d error=%d",
                appId.c_str(), appInstanceId.c_str(), static_cast<int>(oldState),
                static_cast<int>(newState), static_cast<int>(errorReason));

            if (appInstanceId.empty()) {
                return;
            }
            if (newState == Exchange::IAppManager::APP_STATE_UNLOADED) {
                OnAppUnloaded(appInstanceId);
                return;
            }

            // Handle PAUSED entry
            if (newState == Exchange::IAppManager::APP_STATE_PAUSED) {
                StartLifecycleTimer(appId, appInstanceId, true);
            }
            // Handle PAUSED exit
            else if (oldState == Exchange::IAppManager::APP_STATE_PAUSED) {
                CancelPausedTimer(appInstanceId);
            }

            // Handle SUSPENDED entry
            if (newState == Exchange::IAppManager::APP_STATE_SUSPENDED) {
                StartLifecycleTimer(appId, appInstanceId, false);
            }
            // Handle SUSPENDED exit
            else if (oldState == Exchange::IAppManager::APP_STATE_SUSPENDED) {
                CancelSuspendedTimer(appInstanceId);
            }

            // If app is TERMINATED or HIBERNATED, cancel all timers
            if (newState == Exchange::IAppManager::APP_STATE_HIBERNATED ||
                newState == Exchange::IAppManager::APP_STATE_TERMINATING ||
                (newState == Exchange::IAppManager::APP_STATE_ACTIVE && oldState != Exchange::IAppManager::APP_STATE_ACTIVE)) {
                CancelAllTimers(appInstanceId);
            }
        }

        void ResourceMonitorImplementation::StartLifecycleTimer(const string& appId,
            const string& appInstanceId, bool paused)
        {
            AppLifecycleCapabilities capabilities;
            uint64_t generation;
            bool capture;
            {
                std::lock_guard<std::mutex> lock(_appTimersMutex);
                if (_timerStopping) { return; }
                auto& state = _appTimers[appInstanceId];
                if (state.generation == 0) { state.generation = ++_nextTimerGeneration; }
                generation = state.generation;
                capture = !state.hasSnapshot;
                capabilities = state.lifecycleSnapshot;
            }
            if (capture) {
                GetAppLifecycleCapabilities(appId, capabilities);
            }
            std::lock_guard<std::mutex> lock(_appTimersMutex);
            const auto it = _appTimers.find(appInstanceId);
            if (_timerStopping || it == _appTimers.end() || it->second.generation != generation) {
                return;
            }
            auto& state = it->second;
            if (!state.hasSnapshot && capabilities.status != LifecycleConfigStatus::UNAVAILABLE) {
                state.lifecycleSnapshot = capabilities;
                state.hasSnapshot = true;
            }
            if (paused) {
                StartPausedTimerLocked(appInstanceId, state.lifecycleSnapshot);
            } else {
                StartSuspendedTimerLocked(appInstanceId);
            }
        }

        void ResourceMonitorImplementation::OnAppUnloaded(const string& appInstanceId)
        {
            std::lock_guard<std::mutex> lock(_appTimersMutex);
            _appTimers.erase(appInstanceId);
        }

        // Timer management methods
        void ResourceMonitorImplementation::StartPausedTimer(const string& appInstanceId,
            const AppLifecycleCapabilities& capabilities)
        {
            std::lock_guard<std::mutex> lock(_appTimersMutex);
            StartPausedTimerLocked(appInstanceId, capabilities);
        }

        void ResourceMonitorImplementation::StartPausedTimerLocked(const string& appInstanceId,
            const AppLifecycleCapabilities& capabilities)
        {
            AppTimerState& timerState = _appTimers[appInstanceId];
            
            // Snapshot capabilities
            if (!timerState.hasSnapshot && capabilities.status != LifecycleConfigStatus::UNAVAILABLE) {
                timerState.lifecycleSnapshot = capabilities;
                timerState.hasSnapshot = true;
            }
            
            // Store expected state (the state when timer started)
            timerState.pausedExpectedState = Exchange::IAppManager::APP_STATE_PAUSED;
            
            // Set expiry time
            const uint64_t timeoutMs = static_cast<uint64_t>(_config.PausedToSuspendedTimeout()) * 1000;
            auto now = std::chrono::steady_clock::now();
            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            timerState.pausedTimerExpiryMs = nowMs + timeoutMs;
            
            // Wake up timer worker
            ScheduleNextTimerLocked();
            
            LOGINFO("Started PAUSED timer for appInstanceId=%s, timeout=%u seconds",
                appInstanceId.c_str(), _config.PausedToSuspendedTimeout());
        }

        void ResourceMonitorImplementation::StartSuspendedTimer(const string& appInstanceId)
        {
            std::lock_guard<std::mutex> lock(_appTimersMutex);
            StartSuspendedTimerLocked(appInstanceId);
        }

        void ResourceMonitorImplementation::StartSuspendedTimerLocked(const string& appInstanceId)
        {
            AppTimerState& timerState = _appTimers[appInstanceId];
            
            // Store expected state (the state when timer started)
            timerState.suspendedExpectedState = Exchange::IAppManager::APP_STATE_SUSPENDED;
            
            // Set expiry time (configured GPU grace period)
            const uint64_t timeoutMs = static_cast<uint64_t>(_config.SuspendedGpuGracePeriod()) * 1000;
            auto now = std::chrono::steady_clock::now();
            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            timerState.suspendedTimerExpiryMs = nowMs + timeoutMs;
            
            // Wake up timer worker
            ScheduleNextTimerLocked();
            
            LOGINFO("Started SUSPENDED timer (GPU grace period) for appInstanceId=%s, timeout=%u seconds",
                appInstanceId.c_str(), _config.SuspendedGpuGracePeriod());
        }

        void ResourceMonitorImplementation::CancelPausedTimer(const string& appInstanceId)
        {
            std::lock_guard<std::mutex> lock(_appTimersMutex);
            auto it = _appTimers.find(appInstanceId);
            if (it != _appTimers.end()) {
                if (it->second.pausedTimerExpiryMs != 0) {
                    it->second.pausedTimerExpiryMs = 0;
                    ScheduleNextTimerLocked();
                    LOGINFO("Cancelled PAUSED timer for appInstanceId=%s", appInstanceId.c_str());
                }
            }
        }

        void ResourceMonitorImplementation::CancelSuspendedTimer(const string& appInstanceId)
        {
            std::lock_guard<std::mutex> lock(_appTimersMutex);
            auto it = _appTimers.find(appInstanceId);
            if (it != _appTimers.end()) {
                if (it->second.suspendedTimerExpiryMs != 0) {
                    it->second.suspendedTimerExpiryMs = 0;
                    ScheduleNextTimerLocked();
                    LOGINFO("Cancelled SUSPENDED timer for appInstanceId=%s", appInstanceId.c_str());
                }
            }
        }

        void ResourceMonitorImplementation::CancelAllTimers(const string& appInstanceId)
        {
            std::lock_guard<std::mutex> lock(_appTimersMutex);
            auto it = _appTimers.find(appInstanceId);
            if (it != _appTimers.end()) {
                if (it->second.pausedTimerExpiryMs != 0) {
                    it->second.pausedTimerExpiryMs = 0;
                }
                if (it->second.suspendedTimerExpiryMs != 0) {
                    it->second.suspendedTimerExpiryMs = 0;
                }
                ScheduleNextTimerLocked();
                LOGINFO("Cancelled all timers for appInstanceId=%s", appInstanceId.c_str());
            }
        }

        void ResourceMonitorImplementation::ScheduleNextTimerLocked()
        {
            if (_timerStopping) {
                return;
            }
            uint64_t nextExpiryMs = 0;
            for (const auto& entry : _appTimers) {
                const AppTimerState& timerState = entry.second;
                // Find next expiry
                if (timerState.pausedTimerExpiryMs != 0 &&
                    (nextExpiryMs == 0 || timerState.pausedTimerExpiryMs < nextExpiryMs)) {
                    nextExpiryMs = timerState.pausedTimerExpiryMs;
                }
                if (timerState.suspendedTimerExpiryMs != 0 &&
                    (nextExpiryMs == 0 || timerState.suspendedTimerExpiryMs < nextExpiryMs)) {
                    nextExpiryMs = timerState.suspendedTimerExpiryMs;
                }
            }
            if (nextExpiryMs != 0) {
                const uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                const uint64_t waitMs = nextExpiryMs > nowMs ? nextExpiryMs - nowMs : 0;
                const uint32_t delayMs = static_cast<uint32_t>(std::min<uint64_t>(waitMs, 86400000));
                _timerHandler.Trigger(Core::Time::Now().Add(delayMs).Ticks(), *_timer);
            } else {
                _timerHandler.Revoke(*_timer);
            }
        }

        // Timer worker loop - checks for expired timers and triggers callbacks
        void ResourceMonitorImplementation::OnLifecycleTimerExpired()
        {
            std::vector<std::pair<std::string, bool>> expiredTimers; // pair: appInstanceId, isPaused
            {
                std::lock_guard<std::mutex> lock(_appTimersMutex);
                if (_timerStopping) {
                    return;
                }
                const uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                for (auto& entry : _appTimers) {
                    const std::string& appInstanceId = entry.first;
                    AppTimerState& timerState = entry.second;
                    // Check PAUSED timer
                    if (timerState.pausedTimerExpiryMs != 0 && nowMs >= timerState.pausedTimerExpiryMs) {
                        expiredTimers.emplace_back(appInstanceId, true);
                        timerState.pausedTimerExpiryMs = 0;
                    }
                    // Check SUSPENDED timer
                    if (timerState.suspendedTimerExpiryMs != 0 && nowMs >= timerState.suspendedTimerExpiryMs) {
                        expiredTimers.emplace_back(appInstanceId, false);
                        timerState.suspendedTimerExpiryMs = 0;
                    }
                }
            }
            // Trigger callbacks for expired timers (outside lock)
            for (const auto& timer : expiredTimers) {
                if (timer.second) {
                    OnPausedTimerExpired(timer.first);
                } else {
                    OnSuspendedTimerExpired(timer.first);
                }
            }
            // Wait until next expiry or shutdown
            std::lock_guard<std::mutex> lock(_appTimersMutex);
            ScheduleNextTimerLocked();
        }

        // Timer callbacks
        void ResourceMonitorImplementation::OnPausedTimerExpired(const string& appInstanceId)
        {
            LOGINFO("PAUSED timer expired for appInstanceId=%s", appInstanceId.c_str());
            
            // Verify the app is still in the expected state (PAUSED)
            Exchange::IAppManager::AppLifecycleState expectedState;
            {
                std::lock_guard<std::mutex> lock(_appTimersMutex);
                auto it = _appTimers.find(appInstanceId);
                if (it == _appTimers.end()) {
                    LOGWARN("PAUSED timer expired but appInstanceId=%s not found in timer map, skipping", appInstanceId.c_str());
                    return;
                }
                expectedState = it->second.pausedExpectedState;
            }
            
            const Exchange::IAppManager::AppLifecycleState currentState = GetAppState(appInstanceId);
            if (currentState != expectedState) {
                LOGINFO("PAUSED timer expired but appInstanceId=%s state changed from expected=%d to current=%d, skipping action",
                    appInstanceId.c_str(), static_cast<int>(expectedState), static_cast<int>(currentState));
                return;
            }
            
            // Check if this is a non-app workload
            if (IsNonAppWorkload(appInstanceId)) {
                LOGINFO("Non-app workload detected for appInstanceId=%s, requesting termination", appInstanceId.c_str());
                RequestAppTermination(appInstanceId, "non-app-workload-paused-timeout");
                return;
            }
            
            // Get lifecycle capabilities snapshot
            AppLifecycleCapabilities capabilities;
            bool hasSnapshot = false;
            {
                std::lock_guard<std::mutex> lock(_appTimersMutex);
                auto it = _appTimers.find(appInstanceId);
                if (it != _appTimers.end() && it->second.hasSnapshot) {
                    capabilities = it->second.lifecycleSnapshot;
                    hasSnapshot = true;
                }
            }
            
            // Check if SUSPENDED is supported
            bool supportsSuspended = false;
            if (hasSnapshot && capabilities.status == LifecycleConfigStatus::CONFIGURED) {
                supportsSuspended = (capabilities.supportedStates & static_cast<uint8_t>(AppLifecycleState::SUSPENDED)) != 0;
            }
            
            if (supportsSuspended) {
                LOGINFO("App supports SUSPENDED, requesting SUSPENDED transition for appInstanceId=%s", appInstanceId.c_str());
                RequestSuspendedTransition(appInstanceId);
            } else {
                if (!hasSnapshot) {
                    LOGWARN("No lifecycle capability snapshot for appInstanceId=%s, treating as SUSPENDED not supported", appInstanceId.c_str());
                } else if (capabilities.status == LifecycleConfigStatus::UNAVAILABLE) {
                    LOGINFO("Lifecycle capabilities unavailable for appInstanceId=%s, treating as SUSPENDED not supported", appInstanceId.c_str());
                } else {
                    LOGINFO("App does not support SUSPENDED for appInstanceId=%s, requesting termination", appInstanceId.c_str());
                }
                RequestAppTermination(appInstanceId, "paused-timeout-suspended-not-supported");
            }
        }

        // Helper to parse GPU memory from RuntimeManager GetInfo JSON
        bool ResourceMonitorImplementation::GetGPUMemoryUsage(const string& appInstanceId, uint64_t& gpuMemoryBytes)
        {
            gpuMemoryBytes = 0;
            auto runtimeManager = AcquireRuntimeManager();
            if (!runtimeManager) {
                LOGWARN("RuntimeManager unavailable, cannot query GPU memory for appInstanceId=%s", appInstanceId.c_str());
                return false;
            }
            string infoJson;
            const Core::hresult result = runtimeManager->GetInfo(appInstanceId, infoJson);
            if (result != Core::ERROR_NONE) {
                LOGWARN("Failed to get container info for appInstanceId=%s, error=%d", appInstanceId.c_str(), static_cast<int>(result));
                return false;
            }
            JsonObject info;
            if (!info.FromString(infoJson)) {
                return false;
            }
            if (info.HasLabel("gpu")) {
                if (info["gpu"].Content() != JsonValue::type::OBJECT) { return false; }
                const JsonObject gpu = info["gpu"].Object();
                if (!gpu.HasLabel("memory") || gpu["memory"].Content() != JsonValue::type::OBJECT) { return false; }
                const JsonObject memory = gpu["memory"].Object();
                if (!memory.HasLabel("usage") || memory["usage"].Content() != JsonValue::type::NUMBER) { return false; }
                return Utils::ParseMemorySize(memory["usage"].Value(), gpuMemoryBytes);
            }
            // Parse JSON: {"ramUsage": "150MB","cpuUsage": "20%","gpuMemory": "32MB","uptime": "45s"}
            // Extract gpuMemory value
            if (!info.HasLabel("gpuMemory") || info["gpuMemory"].Content() != JsonValue::type::STRING) {
                return false;
            }
            string gpuMemoryStr = info["gpuMemory"].String();
            // Parse value like "32MB" or "32 MB"
            gpuMemoryStr.erase(std::remove(gpuMemoryStr.begin(), gpuMemoryStr.end(), ' '), gpuMemoryStr.end());
            return Utils::ParseMemorySize(gpuMemoryStr, gpuMemoryBytes);
        }

        void ResourceMonitorImplementation::OnSuspendedTimerExpired(const string& appInstanceId)
        {
            LOGINFO("SUSPENDED timer expired for appInstanceId=%s", appInstanceId.c_str());
            
            // Verify the app is still in the expected state (SUSPENDED)
            Exchange::IAppManager::AppLifecycleState expectedState;
            {
                std::lock_guard<std::mutex> lock(_appTimersMutex);
                auto it = _appTimers.find(appInstanceId);
                if (it == _appTimers.end()) {
                    LOGWARN("SUSPENDED timer expired but appInstanceId=%s not found in timer map, skipping", appInstanceId.c_str());
                    return;
                }
                expectedState = it->second.suspendedExpectedState;
            }
            
            const Exchange::IAppManager::AppLifecycleState currentState = GetAppState(appInstanceId);
            if (currentState != expectedState) {
                LOGINFO("SUSPENDED timer expired but appInstanceId=%s state changed from expected=%d to current=%d, skipping action",
                    appInstanceId.c_str(), static_cast<int>(expectedState), static_cast<int>(currentState));
                return;
            }
            
            // GPU grace period check (RDKEMW-28031 requirement)
            const uint64_t maxGPUAllowedBytes = 0; // AC4/AC5 require released GPU resources
            uint64_t gpuMemoryBytes = 0;
            if (!GetGPUMemoryUsage(appInstanceId, gpuMemoryBytes)) {
                LOGWARN("GPU usage unavailable for appInstanceId=%s; deferring lifecycle action", appInstanceId.c_str());
                return;
            }
            if (gpuMemoryBytes > maxGPUAllowedBytes) {
                LOGWARN("GPU grace period check failed for appInstanceId=%s: GPU=%llu bytes, requesting termination",
                    appInstanceId.c_str(), static_cast<unsigned long long>(gpuMemoryBytes));
                RequestAppTermination(appInstanceId, "suspended-gpu-grace-period-violation");
                return;
            }
            LOGINFO("GPU grace period check passed for appInstanceId=%s: GPU released", appInstanceId.c_str());
            
            // Check app hibernation support via RDKEMW-28030 metadata
            bool appSupportsHibernation = false;
            {
                std::lock_guard<std::mutex> lock(_appTimersMutex);
                auto it = _appTimers.find(appInstanceId);
                if (it != _appTimers.end() && it->second.hasSnapshot) {
                    appSupportsHibernation = (it->second.lifecycleSnapshot.supportedStates & 
                        static_cast<uint8_t>(AppLifecycleState::HIBERNATED)) != 0;
                }
            }
            
            if (appSupportsHibernation) {
                LOGINFO("App supports hibernation for appInstanceId=%s, requesting HIBERNATED transition", appInstanceId.c_str());
                RequestHibernatedTransition(appInstanceId);
            } else {
                LOGINFO("App does not support hibernation for appInstanceId=%s, requesting termination", appInstanceId.c_str());
                RequestAppTermination(appInstanceId, "suspended-hibernation-not-supported");
            }
        }

        // Enforcement helpers
        void ResourceMonitorImplementation::RequestSuspendedTransition(const string& appInstanceId)
        {
            // Map appInstanceId to appId using GetLoadedApps
            auto appManager = AcquireAppManager();
            if (!appManager) {
                LOGERR("AppManager unavailable, cannot request SUSPENDED transition for appInstanceId=%s", appInstanceId.c_str());
                return;
            }
            
            string appId;
            Exchange::IAppManager::ILoadedAppInfoIterator* loadedApps = nullptr;
            const Core::hresult result = appManager->GetLoadedApps(loadedApps);
            
            if (result != Core::ERROR_NONE || loadedApps == nullptr) {
                LOGERR("Failed to get loaded apps for transition, appInstanceId=%s, error=%d", appInstanceId.c_str(), static_cast<int>(result));
                return;
            }
            
            // Iterate to find the appId corresponding to appInstanceId
            bool found = false;
            uint32_t index = 0;
            while (true) {
                Exchange::IAppManager::LoadedAppInfo info;
                const bool hasNext = loadedApps->Next(info);
                if (!hasNext) {
                    break;
                }
                
                if (info.appInstanceId == appInstanceId) {
                    appId = info.appId;
                    found = true;
                    break;
                }
                index++;
            }
            
            loadedApps->Release();
            
            if (!found) {
                LOGERR("Could not find appId for appInstanceId=%s", appInstanceId.c_str());
                return;
            }
            
            // Request SUSPENDED transition via AppManager
            const Core::hresult transitionResult = appManager->SetTargetAppState(appId, Exchange::IAppManager::APP_STATE_SUSPENDED);
            if (transitionResult == Core::ERROR_NONE) {
                LOGINFO("Requested SUSPENDED transition for appId=%s appInstanceId=%s", appId.c_str(), appInstanceId.c_str());
            } else {
                LOGERR("Failed to request SUSPENDED transition for appId=%s appInstanceId=%s, error=%d", 
                    appId.c_str(), appInstanceId.c_str(), static_cast<int>(transitionResult));
            }
        }

        void ResourceMonitorImplementation::RequestHibernatedTransition(const string& appInstanceId)
        {
            // Map appInstanceId to appId using GetLoadedApps
            auto appManager = AcquireAppManager();
            if (!appManager) {
                LOGERR("AppManager unavailable, cannot request HIBERNATED transition for appInstanceId=%s", appInstanceId.c_str());
                return;
            }
            
            string appId;
            Exchange::IAppManager::ILoadedAppInfoIterator* loadedApps = nullptr;
            const Core::hresult result = appManager->GetLoadedApps(loadedApps);
            
            if (result != Core::ERROR_NONE || loadedApps == nullptr) {
                LOGERR("Failed to get loaded apps for transition, appInstanceId=%s, error=%d", appInstanceId.c_str(), static_cast<int>(result));
                return;
            }
            
            // Iterate to find the appId corresponding to appInstanceId
            bool found = false;
            uint32_t index = 0;
            while (true) {
                Exchange::IAppManager::LoadedAppInfo info;
                const bool hasNext = loadedApps->Next(info);
                if (!hasNext) {
                    break;
                }
                
                if (info.appInstanceId == appInstanceId) {
                    appId = info.appId;
                    found = true;
                    break;
                }
                index++;
            }
            
            loadedApps->Release();
            
            if (!found) {
                LOGERR("Could not find appId for appInstanceId=%s", appInstanceId.c_str());
                return;
            }
            
            // Request HIBERNATED transition via AppManager
            const Core::hresult transitionResult = appManager->SetTargetAppState(appId, Exchange::IAppManager::APP_STATE_HIBERNATED);
            if (transitionResult == Core::ERROR_NONE) {
                LOGINFO("Requested HIBERNATED transition for appId=%s appInstanceId=%s", appId.c_str(), appInstanceId.c_str());
            } else {
                LOGERR("Failed to request HIBERNATED transition for appId=%s appInstanceId=%s, error=%d", 
                    appId.c_str(), appInstanceId.c_str(), static_cast<int>(transitionResult));
            }
        }

        void ResourceMonitorImplementation::RequestAppTermination(const string& appInstanceId, const string& reason)
        {
            // Map appInstanceId to appId using GetLoadedApps
            auto appManager = AcquireAppManager();
            if (!appManager) {
                LOGERR("AppManager unavailable, cannot request termination for appInstanceId=%s", appInstanceId.c_str());
                return;
            }
            
            string appId;
            Exchange::IAppManager::ILoadedAppInfoIterator* loadedApps = nullptr;
            const Core::hresult result = appManager->GetLoadedApps(loadedApps);
            
            if (result != Core::ERROR_NONE || loadedApps == nullptr) {
                LOGERR("Failed to get loaded apps for termination, appInstanceId=%s, error=%d", appInstanceId.c_str(), static_cast<int>(result));
                return;
            }
            
            // Iterate to find the appId corresponding to appInstanceId
            bool found = false;
            uint32_t index = 0;
            while (true) {
                Exchange::IAppManager::LoadedAppInfo info;
                const bool hasNext = loadedApps->Next(info);
                if (!hasNext) {
                    break;
                }
                
                if (info.appInstanceId == appInstanceId) {
                    appId = info.appId;
                    found = true;
                    break;
                }
                index++;
            }
            
            loadedApps->Release();
            
            if (!found) {
                LOGERR("Could not find appId for appInstanceId=%s", appInstanceId.c_str());
                return;
            }
            
            // Request termination via AppManager
            const Core::hresult terminateResult = appManager->TerminateApp(appId);
            if (terminateResult == Core::ERROR_NONE) {
                LOGINFO("Requested termination for appId=%s appInstanceId=%s reason=%s", appId.c_str(), appInstanceId.c_str(), reason.c_str());
            } else {
                LOGERR("Failed to request termination for appId=%s appInstanceId=%s reason=%s, error=%d", 
                    appId.c_str(), appInstanceId.c_str(), reason.c_str(), static_cast<int>(terminateResult));
            }
        }

        bool ResourceMonitorImplementation::IsNonAppWorkload(const string& appInstanceId)
        {
            // Query GetLoadedApps to check LoadedAppInfo.type
            auto appManager = AcquireAppManager();
            if (!appManager) {
                LOGWARN("AppManager unavailable, cannot check workload type for appInstanceId=%s", appInstanceId.c_str());
                return false; // Safe default
            }
            
            Exchange::IAppManager::ILoadedAppInfoIterator* loadedApps = nullptr;
            const Core::hresult result = appManager->GetLoadedApps(loadedApps);
            
            if (result != Core::ERROR_NONE || loadedApps == nullptr) {
                LOGWARN("Failed to get loaded apps for workload type check, appInstanceId=%s", appInstanceId.c_str());
                return false;
            }
            
            // Iterate to find the workload type
            bool found = false;
            string workloadType;
            uint32_t index = 0;
            while (true) {
                Exchange::IAppManager::LoadedAppInfo info;
                const bool hasNext = loadedApps->Next(info);
                if (!hasNext) {
                    break;
                }
                
                if (info.appInstanceId == appInstanceId) {
                    workloadType = info.type;
                    found = true;
                    break;
                }
                index++;
            }
            
            loadedApps->Release();
            
            if (!found) {
                LOGWARN("Could not find workload type for appInstanceId=%s", appInstanceId.c_str());
                return false;
            }
            
            // Check if type indicates a non-app workload
            // AppManager reports system workloads as "SYSTEM_APP"; retain the "SYSTEM" alias
            // Unknown workload types follow the application capability checks
            const bool isNonApp = workloadType == "SYSTEM_APP" || workloadType == "SYSTEM";
            if (isNonApp) {
                LOGINFO("Detected non-app workload: appInstanceId=%s type=%s", appInstanceId.c_str(), workloadType.c_str());
            }
            
            return isNonApp;
        }

        Exchange::IAppManager::AppLifecycleState ResourceMonitorImplementation::GetAppState(const string& appInstanceId)
        {
            // Query GetLoadedApps to find the current state of the app
            auto appManager = AcquireAppManager();
            if (!appManager) {
                LOGWARN("AppManager unavailable, cannot get state for appInstanceId=%s", appInstanceId.c_str());
                return Exchange::IAppManager::APP_STATE_UNKNOWN;
            }
            
            Exchange::IAppManager::ILoadedAppInfoIterator* loadedApps = nullptr;
            const Core::hresult result = appManager->GetLoadedApps(loadedApps);
            
            if (result != Core::ERROR_NONE || loadedApps == nullptr) {
                LOGWARN("Failed to get loaded apps for state check, appInstanceId=%s", appInstanceId.c_str());
                return Exchange::IAppManager::APP_STATE_UNKNOWN;
            }
            
            // Iterate to find the current state
            bool found = false;
            Exchange::IAppManager::AppLifecycleState state = Exchange::IAppManager::APP_STATE_UNKNOWN;
            uint32_t index = 0;
            while (true) {
                Exchange::IAppManager::LoadedAppInfo info;
                const bool hasNext = loadedApps->Next(info);
                if (!hasNext) {
                    break;
                }
                
                if (info.appInstanceId == appInstanceId) {
                    state = info.lifecycleState;
                    found = true;
                    break;
                }
                index++;
            }
            
            loadedApps->Release();
            
            if (!found) {
                LOGWARN("Could not find state for appInstanceId=%s", appInstanceId.c_str());
                return Exchange::IAppManager::APP_STATE_UNKNOWN;
            }
            
            return state;
        }
    } // namespace Plugin
} // namespace WPEFramework

