#include "VictimSelectorImplementation.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace WPEFramework {
namespace Plugin {

void VictimSelectorImplementation::AppManagerNotification::OnAppLifecycleStateChanged(
    const string& appId, const string& appInstanceId,
    const Exchange::IAppManager::AppLifecycleState newState,
    const Exchange::IAppManager::AppLifecycleState oldState,
    const Exchange::IAppManager::AppErrorReason errorReason) {
    mParent.onAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
}

VictimSelectorImplementation::VictimSelectorImplementation()
    : mService(nullptr)
    , mAppManager(nullptr)
    , mRuntimeManager(nullptr)
    , mConnectionId(0)
    , mAppManagerNotification(*this)
    , mNotification(nullptr) {
}

VictimSelectorImplementation::~VictimSelectorImplementation() {
    releaseAppManager();
    std::lock_guard<std::mutex> guard(mLock);
    if (mNotification != nullptr) {
        mNotification->Release();
        mNotification = nullptr;
    }
}

Core::hresult VictimSelectorImplementation::Register(Exchange::IVictimSelector::INotification* notification) {
    if (notification == nullptr) {
        return Core::ERROR_BAD_REQUEST;
    }

    std::lock_guard<std::mutex> guard(mLock);
    if (mNotification != nullptr) {
        mNotification->Release();
    }
    mNotification = notification;
    mNotification->AddRef();
    return Core::ERROR_NONE;
}

Core::hresult VictimSelectorImplementation::Unregister(Exchange::IVictimSelector::INotification* notification) {
    std::lock_guard<std::mutex> guard(mLock);
    if (mNotification == notification) {
        mNotification->Release();
        mNotification = nullptr;
    }
    return Core::ERROR_NONE;
}

uint32_t VictimSelectorImplementation::Configure(PluginHost::IShell* service) {
    if (service == nullptr) {
        return Core::ERROR_BAD_REQUEST;
    }

    mService = service;
    mService->AddRef();
    mAppManager = mService->QueryInterfaceByCallsign<Exchange::IAppManager>("org.rdk.AppManager");
    mRuntimeManager = mService->QueryInterfaceByCallsign<Exchange::IRuntimeManager>("org.rdk.RuntimeManager");
    if (mAppManager == nullptr || mRuntimeManager == nullptr) {
        releaseAppManager();
        return Core::ERROR_UNAVAILABLE;
    }

    return mAppManager->Register(&mAppManagerNotification);
}

void VictimSelectorImplementation::releaseAppManager() {
    if (mAppManager != nullptr) {
        mAppManager->Unregister(&mAppManagerNotification);
        mAppManager->Release();
        mAppManager = nullptr;
    }
    if (mRuntimeManager != nullptr) {
        mRuntimeManager->Release();
        mRuntimeManager = nullptr;
    }
    if (mService != nullptr) {
        mService->Release();
        mService = nullptr;
    }
}

Core::hresult VictimSelectorImplementation::selectVictim(std::string& appId, bool& isHibernated) {
    isHibernated = false;
    if (mAppManager == nullptr) {
        return Core::ERROR_UNAVAILABLE;
    }

    Exchange::IAppManager::ILoadedAppInfoIterator* apps = nullptr;
    const Core::hresult status = mAppManager->GetLoadedApps(apps);
    if (status != Core::ERROR_NONE || apps == nullptr) {
        return Core::ERROR_GENERAL;
    }

    struct Candidate {
        Exchange::IAppManager::LoadedAppInfo app;
        uint64_t memoryUsage;
    };
    std::vector<Candidate> paused;
    std::vector<Candidate> suspended;
    std::vector<Candidate> hibernated;
    Exchange::IAppManager::LoadedAppInfo info;
    while (apps->Next(info)) {
        if (info.priority == 0) {
            continue;
        }
        uint64_t memoryUsage = 0;
        std::string stats;
        if (mRuntimeManager->GetInfo(info.appInstanceId, stats) == Core::ERROR_NONE) {
            JsonObject statsObject;
            if (statsObject.FromString(stats) && statsObject.HasLabel("memory")) {
                const JsonObject memoryObject = statsObject["memory"].Object();
                if (memoryObject.HasLabel("user")) {
                    const JsonObject userMemoryObject = memoryObject["user"].Object();
                    if (userMemoryObject.HasLabel("usage")) {
                        memoryUsage = static_cast<uint64_t>(userMemoryObject["usage"].Number());
                    }
                }
            }
        }
        Candidate candidate{info, memoryUsage};
        if (info.lifecycleState == Exchange::IAppManager::APP_STATE_PAUSED) {
            paused.push_back(candidate);
        } else if (info.lifecycleState == Exchange::IAppManager::APP_STATE_SUSPENDED) {
            suspended.push_back(candidate);
        } else if (info.lifecycleState == Exchange::IAppManager::APP_STATE_HIBERNATED) {
            hibernated.push_back(candidate);
        }
    }
    apps->Release();

    const std::vector<Candidate>* candidates = nullptr;
    if (paused.size() == 1) {
        candidates = &paused;
    } else if (!suspended.empty()) {
        candidates = &suspended;
    } else if (!hibernated.empty()) {
        candidates = &hibernated;
    }

    if (candidates == nullptr || candidates->empty()) {
        return Core::ERROR_NONE;
    }

    const Candidate& victim = *std::min_element(candidates->begin(), candidates->end(),
        [](const Candidate& left, const Candidate& right) {
            if (left.app.priority != right.app.priority) {
                return left.app.priority > right.app.priority;
            }
            if (left.memoryUsage != right.memoryUsage) {
                return left.memoryUsage > right.memoryUsage;
            }
            if (left.app.lastActiveIndex != right.app.lastActiveIndex) {
                const uint32_t leftIndex = left.app.lastActiveIndex == 0 ? std::numeric_limits<uint32_t>::max() : left.app.lastActiveIndex;
                const uint32_t rightIndex = right.app.lastActiveIndex == 0 ? std::numeric_limits<uint32_t>::max() : right.app.lastActiveIndex;
                return leftIndex < rightIndex;
            }
            return left.app.appId < right.app.appId;
        });
    appId = victim.app.appId;
    isHibernated = victim.app.lifecycleState == Exchange::IAppManager::APP_STATE_HIBERNATED;
    return Core::ERROR_NONE;
}

Core::hresult VictimSelectorImplementation::Evict(const EvictionReason reason, const EvictionType type) {
    if (reason != EVICTION_REASON_RAM) {
        return Core::ERROR_UNAVAILABLE;
    }
    if (mAppManager == nullptr) {
        return Core::ERROR_UNAVAILABLE;
    }

    std::string appId;
    bool isHibernated = false;
    const Core::hresult selectionStatus = selectVictim(appId, isHibernated);
    if (selectionStatus != Core::ERROR_NONE) {
        complete(false, EVICT_ERROR_TERMINATION_FAILED);
        return selectionStatus;
    }
    if (appId.empty()) {
        complete(false, EVICT_ERROR_NO_CANDIDATE_FOUND);
        return Core::ERROR_NONE;
    }

    {
        std::lock_guard<std::mutex> guard(mLock);
        mPendingAppId = appId;
    }

    const Core::hresult status = (type == EVICTION_TYPE_HARD || isHibernated)
        ? mAppManager->KillApp(appId)
        : mAppManager->TerminateApp(appId);
    if (status != Core::ERROR_NONE) {
        {
            std::lock_guard<std::mutex> guard(mLock);
            mPendingAppId.clear();
        }
        complete(false, EVICT_ERROR_TERMINATION_FAILED);
    }
    return status;
}

void VictimSelectorImplementation::onAppLifecycleStateChanged(
    const string& appId, const string&, Exchange::IAppManager::AppLifecycleState newState,
    Exchange::IAppManager::AppLifecycleState, Exchange::IAppManager::AppErrorReason errorReason) {
    bool completeEviction = false;
    {
        std::lock_guard<std::mutex> guard(mLock);
        completeEviction = (appId == mPendingAppId) &&
            (newState == Exchange::IAppManager::APP_STATE_UNLOADED || errorReason != Exchange::IAppManager::APP_ERROR_NONE);
        if (completeEviction) {
            mPendingAppId.clear();
        }
    }
    if (completeEviction) {
        const bool evicted = errorReason == Exchange::IAppManager::APP_ERROR_NONE &&
            newState == Exchange::IAppManager::APP_STATE_UNLOADED;
        complete(evicted, evicted ? EVICT_ERROR_NONE : EVICT_ERROR_TERMINATION_FAILED);
    }
}

void VictimSelectorImplementation::complete(bool evicted, EvictErrorReason errorCode) {
    Exchange::IVictimSelector::INotification* notification = nullptr;
    {
        std::lock_guard<std::mutex> guard(mLock);
        notification = mNotification;
        if (notification != nullptr) {
            notification->AddRef();
        }
    }
    if (notification != nullptr) {
        notification->OnEvictComplete(evicted, errorCode);
        notification->Release();
    }
}

} // namespace Plugin
} // namespace WPEFramework
