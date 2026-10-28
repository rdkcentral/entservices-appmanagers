#include <atomic>
#include <list>
#include <map>
#include <sstream>
#include <string>

#define private public
#include "VictimSelectorImplementation.h"
#undef private

#include "common/L0Expect.hpp"

namespace {

using namespace Thunder;

class FakeAppManager : public Exchange::IAppManager {
public:
    explicit FakeAppManager(const std::list<Exchange::IAppManager::LoadedAppInfo>& loadedApps)
        : mLoadedApps(loadedApps)
    {
    }

    uint32_t AddRef() const override { return ++mRefCount; }
    uint32_t Release() const override
    {
        const uint32_t references = --mRefCount;
        if (0 == references) {
            delete this;
        }
        return references;
    }
    void* QueryInterface(const uint32_t) override { return nullptr; }
    Core::hresult Register(INotification*) override { return Core::ERROR_NONE; }
    Core::hresult Unregister(INotification*) override { return Core::ERROR_NONE; }
    Core::hresult GetInstalledApps(string&) override { return Core::ERROR_NONE; }
    Core::hresult IsInstalled(const string&, bool&) override { return Core::ERROR_NONE; }
    Core::hresult GetLoadedApps(ILoadedAppInfoIterator*& apps) override
    {
        ++getLoadedAppsCalls;
        apps = Core::Service<RPC::IteratorType<ILoadedAppInfoIterator>>::Create<ILoadedAppInfoIterator>(mLoadedApps);
        return Core::ERROR_NONE;
    }
    Core::hresult LaunchApp(const string&, const string&, const string&) override { return Core::ERROR_NONE; }
    Core::hresult PreloadApp(const string&, const string&, const string&, string&) override { return Core::ERROR_NONE; }
    Core::hresult CloseApp(const string&) override { return Core::ERROR_NONE; }
    Core::hresult TerminateApp(const string& appId) override { ++terminateCalls; terminatedAppId = appId; return Core::ERROR_NONE; }
    Core::hresult StartSystemApp(const string&) override { return Core::ERROR_NONE; }
    Core::hresult StopSystemApp(const string&) override { return Core::ERROR_NONE; }
    Core::hresult KillApp(const string& appId) override { ++killCalls; killedAppId = appId; return Core::ERROR_NONE; }
    Core::hresult SendIntent(const string&, const string&) override { return Core::ERROR_NONE; }
    Core::hresult ClearAppData(const string&) override { return Core::ERROR_NONE; }
    Core::hresult ClearAllAppData() override { return Core::ERROR_NONE; }
    Core::hresult GetAppMetadata(const string&, const string&, string&) override { return Core::ERROR_NONE; }
    Core::hresult GetAppProperty(const string& appId, const string&, string& value) override
    {
        value = priorities[appId];
        return Core::ERROR_NONE;
    }
    Core::hresult SetAppProperty(const string&, const string&, const string&) override { return Core::ERROR_NONE; }
    Core::hresult GetMaxRunningApps(int32_t&) const override { return Core::ERROR_NONE; }
    Core::hresult GetMaxHibernatedApps(int32_t&) const override { return Core::ERROR_NONE; }
    Core::hresult GetMaxHibernatedFlashUsage(int32_t&) const override { return Core::ERROR_NONE; }
    Core::hresult GetMaxInactiveRamUsage(int32_t&) const override { return Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> mRefCount { 1 };
    std::list<LoadedAppInfo> mLoadedApps;
    std::map<string, string> priorities;
    uint32_t getLoadedAppsCalls { 0 };
    uint32_t terminateCalls { 0 };
    uint32_t killCalls { 0 };
    string terminatedAppId;
    string killedAppId;
};

class FakeRuntimeManager : public Exchange::IRuntimeManager {
public:
    uint32_t AddRef() const override { return ++mRefCount; }
    uint32_t Release() const override
    {
        const uint32_t references = --mRefCount;
        if (0 == references) {
            delete this;
        }
        return references;
    }
    void* QueryInterface(const uint32_t) override { return nullptr; }
    Core::hresult Register(INotification*) override { return Core::ERROR_NONE; }
    Core::hresult Unregister(INotification*) override { return Core::ERROR_NONE; }
    Core::hresult Run(const string&, const string&, const uint32_t, const uint32_t, IValueIterator* const&, IStringIterator* const&, IStringIterator* const&, const string&) override { return Core::ERROR_NONE; }
    Core::hresult Hibernate(const string&) override { return Core::ERROR_NONE; }
    Core::hresult Wake(const string&, const RuntimeState) override { return Core::ERROR_NONE; }
    Core::hresult Suspend(const string&) override { return Core::ERROR_NONE; }
    Core::hresult Resume(const string&) override { return Core::ERROR_NONE; }
    Core::hresult Terminate(const string&) override { return Core::ERROR_NONE; }
    Core::hresult Kill(const string&) override { return Core::ERROR_NONE; }
    Core::hresult GetInfo(const string& appInstanceId, string& info) override
    {
        ++getInfoCalls;
        const std::map<string, string>::const_iterator entry = runtimeInfo.find(appInstanceId);
        if (runtimeInfo.end() == entry) {
            return Core::ERROR_GENERAL;
        }
        info = entry->second;
        return Core::ERROR_NONE;
    }
    Core::hresult Annotate(const string&, const string&, const string&) override { return Core::ERROR_NONE; }
    Core::hresult Mount() override { return Core::ERROR_NONE; }
    Core::hresult Unmount() override { return Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> mRefCount { 1 };
    std::map<string, string> runtimeInfo;
    uint32_t getInfoCalls { 0 };
};

class FakeEvictionNotification : public Exchange::IVictimSelector::INotification {
public:
    uint32_t AddRef() const override { return ++mRefCount; }
    uint32_t Release() const override
    {
        const uint32_t references = --mRefCount;
        if (0 == references) {
            delete this;
        }
        return references;
    }
    void* QueryInterface(const uint32_t) override { return nullptr; }
    void OnEvictComplete(const bool evicted, const Exchange::IVictimSelector::EvictErrorReason errorCode) override
    {
        ++completionCalls;
        lastEvicted = evicted;
        lastError = errorCode;
    }

    mutable std::atomic<uint32_t> mRefCount { 1 };
    uint32_t completionCalls { 0 };
    bool lastEvicted { true };
    Exchange::IVictimSelector::EvictErrorReason lastError { Exchange::IVictimSelector::EVICT_ERROR_NONE };
};

Exchange::IAppManager::LoadedAppInfo LoadedApp(const string& appId, const Exchange::IAppManager::AppLifecycleState state)
{
    Exchange::IAppManager::LoadedAppInfo app;
    app.appId = appId;
    app.appInstanceId = appId + ".instance";
    app.lifecycleState = state;
    return app;
}

Plugin::VictimSelectorImplementation* CreateSelector(FakeAppManager*& appManager)
{
    Plugin::VictimSelectorImplementation* selector = Core::Service<Plugin::VictimSelectorImplementation>::Create<Plugin::VictimSelectorImplementation>();
    selector->mAppManager = appManager;
    selector->mRuntimeManager = new FakeRuntimeManager();
    return selector;
}

} // namespace

uint32_t Test_VS_NonRamEvictionIsRejected()
{
    L0Test::TestResult result;
    FakeAppManager* appManager = new FakeAppManager(std::list<Thunder::Exchange::IAppManager::LoadedAppInfo>());
    Thunder::Plugin::VictimSelectorImplementation* selector = CreateSelector(appManager);

    const Thunder::Core::hresult status = selector->Evict(
        Thunder::Exchange::IVictimSelector::EVICTION_REASON_GPU,
        Thunder::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    L0Test::ExpectEqU32(result, status, Thunder::Core::ERROR_UNAVAILABLE, "Non-RAM evictions are rejected");
    L0Test::ExpectEqU32(result, appManager->getLoadedAppsCalls, 0, "Rejected eviction does not load app list");

    selector->Release();
    return result.failures;
}

uint32_t Test_VS_EvictionWithoutRuntimeManagerIsRejected()
{
    L0Test::TestResult result;
    std::list<Thunder::Exchange::IAppManager::LoadedAppInfo> apps;
    apps.push_back(LoadedApp("candidate", Thunder::Exchange::IAppManager::APP_STATE_SUSPENDED));
    FakeAppManager* appManager = new FakeAppManager(apps);
    Thunder::Plugin::VictimSelectorImplementation* selector = Thunder::Core::Service<Thunder::Plugin::VictimSelectorImplementation>::Create<Thunder::Plugin::VictimSelectorImplementation>();
    selector->mAppManager = appManager;

    const Thunder::Core::hresult status = selector->Evict(
        Thunder::Exchange::IVictimSelector::EVICTION_REASON_RAM,
        Thunder::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    L0Test::ExpectEqU32(result, status, Thunder::Core::ERROR_UNAVAILABLE, "Eviction without RuntimeManager is rejected");
    L0Test::ExpectEqU32(result, appManager->getLoadedAppsCalls, 0, "Unavailable RuntimeManager does not load app list");

    selector->Release();
    return result.failures;
}

uint32_t Test_VS_SoftEvictionSelectsPausedCandidate()
{
    L0Test::TestResult result;
    std::list<Thunder::Exchange::IAppManager::LoadedAppInfo> apps;
    apps.push_back(LoadedApp("protected", Thunder::Exchange::IAppManager::APP_STATE_PAUSED));
    apps.push_back(LoadedApp("paused", Thunder::Exchange::IAppManager::APP_STATE_PAUSED));
    apps.push_back(LoadedApp("suspended", Thunder::Exchange::IAppManager::APP_STATE_SUSPENDED));
    FakeAppManager* appManager = new FakeAppManager(apps);
    appManager->priorities["protected"] = "0";
    appManager->priorities["paused"] = "2";
    appManager->priorities["suspended"] = "2";
    Thunder::Plugin::VictimSelectorImplementation* selector = CreateSelector(appManager);

    const Thunder::Core::hresult status = selector->Evict(
        Thunder::Exchange::IVictimSelector::EVICTION_REASON_RAM,
        Thunder::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    L0Test::ExpectEqU32(result, status, Thunder::Core::ERROR_NONE, "RAM eviction starts successfully");
    L0Test::ExpectEqStr(result, appManager->terminatedAppId, "paused", "Single eligible paused app is selected before suspended app");
    L0Test::ExpectTrue(result, appManager->killedAppId.empty(), "Soft eviction does not kill the selected paused app");
    L0Test::ExpectEqU32(result, static_cast<FakeRuntimeManager*>(selector->mRuntimeManager)->getInfoCalls, 1,
        "Runtime stats are read only for the selected state bucket");

    selector->Release();
    return result.failures;
}

uint32_t Test_VS_SelectsHighestPriorityCandidate()
{
    L0Test::TestResult result;
    std::list<Thunder::Exchange::IAppManager::LoadedAppInfo> apps;
    apps.push_back(LoadedApp("low-priority", Thunder::Exchange::IAppManager::APP_STATE_SUSPENDED));
    apps.push_back(LoadedApp("high-priority", Thunder::Exchange::IAppManager::APP_STATE_SUSPENDED));
    FakeAppManager* appManager = new FakeAppManager(apps);
    appManager->priorities["low-priority"] = "1";
    appManager->priorities["high-priority"] = "2";
    Thunder::Plugin::VictimSelectorImplementation* selector = CreateSelector(appManager);

    const Thunder::Core::hresult status = selector->Evict(
        Thunder::Exchange::IVictimSelector::EVICTION_REASON_RAM,
        Thunder::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    L0Test::ExpectEqU32(result, status, Thunder::Core::ERROR_NONE, "RAM eviction starts successfully");
    L0Test::ExpectEqStr(result, appManager->terminatedAppId, "high-priority", "Higher eviction priority is selected first");

    selector->Release();
    return result.failures;
}

uint32_t Test_VS_SelectsLargestMemoryCandidateAtSamePriority()
{
    L0Test::TestResult result;
    std::list<Thunder::Exchange::IAppManager::LoadedAppInfo> apps;
    apps.push_back(LoadedApp("smaller", Thunder::Exchange::IAppManager::APP_STATE_SUSPENDED));
    apps.push_back(LoadedApp("larger", Thunder::Exchange::IAppManager::APP_STATE_SUSPENDED));
    FakeAppManager* appManager = new FakeAppManager(apps);
    appManager->priorities["smaller"] = "2";
    appManager->priorities["larger"] = "2";
    Thunder::Plugin::VictimSelectorImplementation* selector = CreateSelector(appManager);
    FakeRuntimeManager* runtimeManager = static_cast<FakeRuntimeManager*>(selector->mRuntimeManager);
    runtimeManager->runtimeInfo["smaller.instance"] = "{\"memory\":{\"user\":{\"usage\":100}}}";
    runtimeManager->runtimeInfo["larger.instance"] = "{\"memory\":{\"user\":{\"usage\":200}}}";

    const Thunder::Core::hresult status = selector->Evict(
        Thunder::Exchange::IVictimSelector::EVICTION_REASON_RAM,
        Thunder::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    L0Test::ExpectEqU32(result, status, Thunder::Core::ERROR_NONE, "RAM eviction starts successfully");
    L0Test::ExpectEqStr(result, appManager->terminatedAppId, "larger", "Largest memory user is selected at the same priority");

    selector->Release();
    return result.failures;
}

uint32_t Test_VS_NoCandidateNotifiesCaller()
{
    L0Test::TestResult result;
    FakeAppManager* appManager = new FakeAppManager(std::list<Thunder::Exchange::IAppManager::LoadedAppInfo>());
    Thunder::Plugin::VictimSelectorImplementation* selector = CreateSelector(appManager);
    FakeEvictionNotification* notification = new FakeEvictionNotification();
    selector->Register(notification);

    const Thunder::Core::hresult status = selector->Evict(
        Thunder::Exchange::IVictimSelector::EVICTION_REASON_RAM,
        Thunder::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    L0Test::ExpectEqU32(result, status, Thunder::Core::ERROR_NONE, "No-candidate eviction completes successfully");
    L0Test::ExpectEqU32(result, notification->completionCalls, 1, "No-candidate eviction notifies caller");
    L0Test::ExpectTrue(result, !notification->lastEvicted, "No-candidate notification reports no eviction");
    L0Test::ExpectEqU32(result, notification->lastError, Thunder::Exchange::IVictimSelector::EVICT_ERROR_NO_CANDIDATE_FOUND, "No-candidate notification reports the reason");

    selector->Unregister(notification);
    notification->Release();
    selector->Release();
    return result.failures;
}

uint32_t Test_VS_HardEvictionEscalatesPendingSoftEviction()
{
    L0Test::TestResult result;
    std::list<Thunder::Exchange::IAppManager::LoadedAppInfo> apps;
    apps.push_back(LoadedApp("candidate", Thunder::Exchange::IAppManager::APP_STATE_SUSPENDED));
    FakeAppManager* appManager = new FakeAppManager(apps);
    appManager->priorities["candidate"] = "2";
    Thunder::Plugin::VictimSelectorImplementation* selector = CreateSelector(appManager);

    const Thunder::Core::hresult softStatus = selector->Evict(
        Thunder::Exchange::IVictimSelector::EVICTION_REASON_RAM,
        Thunder::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    const Thunder::Core::hresult hardStatus = selector->Evict(
        Thunder::Exchange::IVictimSelector::EVICTION_REASON_RAM,
        Thunder::Exchange::IVictimSelector::EVICTION_TYPE_HARD);
    L0Test::ExpectEqU32(result, softStatus, Thunder::Core::ERROR_NONE, "Soft eviction starts successfully");
    L0Test::ExpectEqU32(result, hardStatus, Thunder::Core::ERROR_NONE, "Hard request escalates pending soft eviction");
    L0Test::ExpectEqU32(result, appManager->terminateCalls, 1, "Soft eviction terminates selected app once");
    L0Test::ExpectEqU32(result, appManager->killCalls, 1, "Hard escalation kills the pending app once");
    L0Test::ExpectEqStr(result, appManager->killedAppId, "candidate", "Hard escalation targets the pending app");

    selector->Release();
    return result.failures;
}