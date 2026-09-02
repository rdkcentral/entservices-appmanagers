#include "VictimSelectorImplementation.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

namespace WPEFramework {
namespace Plugin {

namespace {
constexpr const char* APP_PRIORITY_PROPERTY = "priority";
/* Apps without a stored priority are treated as ordinary eviction candidates. */
constexpr uint32_t DEFAULT_APP_PRIORITY = 2;
}

SERVICE_REGISTRATION(VictimSelectorImplementation, 1, 0);

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
    , mAppManagerRegistered(false)
    , mNotification(nullptr)
    , mPendingEvictionType(EVICTION_TYPE_SOFT)
    , mEvictionInProgress(false) {
}

VictimSelectorImplementation::~VictimSelectorImplementation() {
    releaseAppManager();
    std::lock_guard<std::mutex> guard(mLock);
    if (nullptr != mNotification) {
        mNotification->Release();
        mNotification = nullptr;
    }
}

Core::hresult VictimSelectorImplementation::Register(Exchange::IVictimSelector::INotification* notification) {
    if (nullptr == notification) {
        return Core::ERROR_BAD_REQUEST;
    }

    std::lock_guard<std::mutex> guard(mLock);
    if (nullptr != mNotification) {
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

Core::hresult VictimSelectorImplementation::Configure(PluginHost::IShell* service) {
    SYSLOG(Logging::Startup, (_T("VictimSelectorImplementation::Configure entry")));

    Core::hresult result = Core::ERROR_BAD_REQUEST;
    if (nullptr == service) {
        SYSLOG(Logging::Startup, (_T("VictimSelectorImplementation::Configure: service is not valid")));
    } else {
        mService = service;
        mService->AddRef();
        mAppManager = mService->QueryInterfaceByCallsign<Exchange::IAppManager>("org.rdk.AppManager");
        mRuntimeManager = mService->QueryInterfaceByCallsign<Exchange::IRuntimeManager>("org.rdk.RuntimeManager");
        if ((nullptr == mAppManager) || (nullptr == mRuntimeManager)) {
            result = Core::ERROR_UNAVAILABLE;
        } else {
            result = mAppManager->Register(&mAppManagerNotification);
            mAppManagerRegistered = (Core::ERROR_NONE == result);
        }

        if (Core::ERROR_NONE != result) {
            releaseAppManager();
        }
    }

    SYSLOG(Logging::Startup, (_T("VictimSelectorImplementation::Configure exit: result=%d"), result));
    return result;
}

void VictimSelectorImplementation::releaseAppManager() {
    {
        std::lock_guard<std::mutex> guard(mLock);
        mPendingAppId.clear();
        mPendingEvictionType = EVICTION_TYPE_SOFT;
        mEvictionInProgress = false;
    }
    if (nullptr != mAppManager) {
        if (mAppManagerRegistered) {
            mAppManager->Unregister(&mAppManagerNotification);
            mAppManagerRegistered = false;
        }
        mAppManager->Release();
        mAppManager = nullptr;
    }
    if (nullptr != mRuntimeManager) {
        mRuntimeManager->Release();
        mRuntimeManager = nullptr;
    }
    if (nullptr != mService) {
        mService->Release();
        mService = nullptr;
    }
}

/* Priority is owned by the platform integration, which stores it per app via
 * IAppManager::SetAppProperty("priority", "<0|1|2>"). 0 means never evict. */
uint32_t VictimSelectorImplementation::getAppPriority(const std::string& appId) const {
    if (nullptr == mAppManager) {
        return DEFAULT_APP_PRIORITY;
    }

    std::string value;
    if ((Core::ERROR_NONE != mAppManager->GetAppProperty(appId, APP_PRIORITY_PROPERTY, value)) || value.empty()) {
        return DEFAULT_APP_PRIORITY;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    if ((value.c_str() == end) || ('\0' != *end) || (std::numeric_limits<uint32_t>::max() < parsed)) {
        return DEFAULT_APP_PRIORITY;
    }
    return static_cast<uint32_t>(parsed);
}

Core::hresult VictimSelectorImplementation::selectVictim(std::string& appId, bool& isHibernated) {
    isHibernated = false;
    if (nullptr == mAppManager) {
        return Core::ERROR_UNAVAILABLE;
    }

    Exchange::IAppManager::ILoadedAppInfoIterator* apps = nullptr;
    const Core::hresult status = mAppManager->GetLoadedApps(apps);
    if ((Core::ERROR_NONE != status) || (nullptr == apps)) {
        return Core::ERROR_GENERAL;
    }

    struct Candidate {
        Exchange::IAppManager::LoadedAppInfo app;
        uint64_t memoryUsage;
        uint32_t priority;
        uint32_t recency;
    };
    std::vector<Candidate> paused;
    std::vector<Candidate> suspended;
    std::vector<Candidate> hibernated;
    Exchange::IAppManager::LoadedAppInfo info;
    /* GetLoadedApps returns most recently active first, so a higher position means staler. */
    uint32_t recency = 0;
    while (apps->Next(info)) {
        const uint32_t position = recency++;
        const uint32_t priority = getAppPriority(info.appId);
        if (0 == priority) {
            continue;
        }
        uint64_t memoryUsage = 0;
        std::string stats;
        if (Core::ERROR_NONE == mRuntimeManager->GetInfo(info.appInstanceId, stats)) {
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
        std::vector<Candidate>* bucket = nullptr;
        if (Exchange::IAppManager::APP_STATE_PAUSED == info.lifecycleState) {
            bucket = &paused;
        } else if (Exchange::IAppManager::APP_STATE_SUSPENDED == info.lifecycleState) {
            bucket = &suspended;
        } else if (Exchange::IAppManager::APP_STATE_HIBERNATED == info.lifecycleState) {
            bucket = &hibernated;
        }
        if (nullptr != bucket) {
            bucket->emplace_back(Candidate{std::move(info), memoryUsage, priority, position});
        }
    }
    apps->Release();

    const std::vector<Candidate>* candidates = nullptr;
    if (1 == paused.size()) {
        candidates = &paused;
    } else if (!suspended.empty()) {
        candidates = &suspended;
    } else if (!hibernated.empty()) {
        candidates = &hibernated;
    }

    if ((nullptr == candidates) || candidates->empty()) {
        return Core::ERROR_NONE;
    }

    const Candidate& victim = *std::min_element(candidates->begin(), candidates->end(),
        [](const Candidate& left, const Candidate& right) {
            if (left.priority != right.priority) {
                return left.priority > right.priority;
            }
            if (left.memoryUsage != right.memoryUsage) {
                return left.memoryUsage > right.memoryUsage;
            }
            if (left.recency != right.recency) {
                return left.recency > right.recency;
            }
            return left.app.appId < right.app.appId;
        });
    appId = victim.app.appId;
    isHibernated = (Exchange::IAppManager::APP_STATE_HIBERNATED == victim.app.lifecycleState);
    return Core::ERROR_NONE;
}

Core::hresult VictimSelectorImplementation::Evict(const EvictionReason reason, const EvictionType type) {
    std::lock_guard<std::mutex> evictGuard(mEvictLock);

    if (EVICTION_REASON_RAM != reason) {
        return Core::ERROR_UNAVAILABLE;
    }
    if (nullptr == mAppManager) {
        return Core::ERROR_UNAVAILABLE;
    }

    std::string pendingAppId;
    {
        std::lock_guard<std::mutex> guard(mLock);
        if (mEvictionInProgress) {
            if ((EVICTION_TYPE_HARD == type) &&
                (EVICTION_TYPE_SOFT == mPendingEvictionType) &&
                !mPendingAppId.empty()) {
                pendingAppId = mPendingAppId;
                mPendingEvictionType = EVICTION_TYPE_HARD;
            } else {
                return Core::ERROR_ILLEGAL_STATE;
            }
        } else {
            mEvictionInProgress = true;
            mPendingEvictionType = type;
        }
    }

    if (!pendingAppId.empty()) {
        const Core::hresult escalationStatus = mAppManager->KillApp(pendingAppId);
        if (Core::ERROR_NONE != escalationStatus) {
            std::lock_guard<std::mutex> guard(mLock);
            if (mEvictionInProgress &&
                (pendingAppId == mPendingAppId) &&
                (EVICTION_TYPE_HARD == mPendingEvictionType)) {
                mPendingEvictionType = EVICTION_TYPE_SOFT;
            }
        }
        return escalationStatus;
    }

    std::string appId;
    bool isHibernated = false;
    const Core::hresult selectionStatus = selectVictim(appId, isHibernated);
    if (Core::ERROR_NONE != selectionStatus) {
        {
            std::lock_guard<std::mutex> guard(mLock);
            mPendingEvictionType = EVICTION_TYPE_SOFT;
            mEvictionInProgress = false;
        }
        complete(false, EVICT_ERROR_TERMINATION_FAILED);
        return selectionStatus;
    }
    if (appId.empty()) {
        {
            std::lock_guard<std::mutex> guard(mLock);
            mPendingEvictionType = EVICTION_TYPE_SOFT;
            mEvictionInProgress = false;
        }
        complete(false, EVICT_ERROR_NO_CANDIDATE_FOUND);
        return Core::ERROR_NONE;
    }

    {
        std::lock_guard<std::mutex> guard(mLock);
        mPendingAppId = appId;
        mPendingEvictionType = ((EVICTION_TYPE_HARD == type) || isHibernated)
            ? EVICTION_TYPE_HARD
            : EVICTION_TYPE_SOFT;
    }

    const Core::hresult status = ((EVICTION_TYPE_HARD == type) || isHibernated)
        ? mAppManager->KillApp(appId)
        : mAppManager->TerminateApp(appId);
    if (Core::ERROR_NONE != status) {
        bool completeEviction = false;
        {
            std::lock_guard<std::mutex> guard(mLock);
            if (mEvictionInProgress && (mPendingAppId == appId)) {
                mPendingAppId.clear();
                mPendingEvictionType = EVICTION_TYPE_SOFT;
                mEvictionInProgress = false;
                completeEviction = true;
            }
        }
        if (completeEviction) {
            complete(false, EVICT_ERROR_TERMINATION_FAILED);
        }
    }
    return status;
}

void VictimSelectorImplementation::onAppLifecycleStateChanged(
    const string& appId, const string&, Exchange::IAppManager::AppLifecycleState newState,
    Exchange::IAppManager::AppLifecycleState, Exchange::IAppManager::AppErrorReason errorReason) {
    bool completeEviction = false;
    {
        std::lock_guard<std::mutex> guard(mLock);
        completeEviction = (mPendingAppId == appId) &&
            ((Exchange::IAppManager::APP_STATE_UNLOADED == newState) || (Exchange::IAppManager::APP_ERROR_NONE != errorReason));
        if (completeEviction) {
            mPendingAppId.clear();
            mPendingEvictionType = EVICTION_TYPE_SOFT;
            mEvictionInProgress = false;
        }
    }
    if (completeEviction) {
        const bool evicted = (Exchange::IAppManager::APP_ERROR_NONE == errorReason) &&
            (Exchange::IAppManager::APP_STATE_UNLOADED == newState);
        complete(evicted, evicted ? EVICT_ERROR_NONE : EVICT_ERROR_TERMINATION_FAILED);
    }
}

void VictimSelectorImplementation::complete(bool evicted, EvictErrorReason errorCode) {
    Exchange::IVictimSelector::INotification* notification = nullptr;
    {
        std::lock_guard<std::mutex> guard(mLock);
        notification = mNotification;
        if (nullptr != notification) {
            notification->AddRef();
        }
    }
    if (nullptr != notification) {
        notification->OnEvictComplete(evicted, errorCode);
        notification->Release();
    }
}

} // namespace Plugin
} // namespace WPEFramework
