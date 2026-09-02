#include <atomic>
#include <list>
#include <map>
#include <string>

#define private public
#include "VictimSelectorImplementation.h"
#undef private

#include "common/L0Expect.hpp"

namespace {

using namespace WPEFramework;

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
    Core::hresult TerminateApp(const string& appId) override { terminatedAppId = appId; return Core::ERROR_NONE; }
    Core::hresult StartSystemApp(const string&) override { return Core::ERROR_NONE; }
    Core::hresult StopSystemApp(const string&) override { return Core::ERROR_NONE; }
    Core::hresult KillApp(const string& appId) override { killedAppId = appId; return Core::ERROR_NONE; }
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
    Core::hresult Run(const string&, const string&, const uint32_t, const uint32_t, IValueIterator* const&, IStringIterator* const&, IStringIterator* const&, const Exchange::RuntimeConfig&) override { return Core::ERROR_NONE; }
    Core::hresult Hibernate(const string&) override { return Core::ERROR_NONE; }
    Core::hresult Wake(const string&, const RuntimeState) override { return Core::ERROR_NONE; }
    Core::hresult Suspend(const string&) override { return Core::ERROR_NONE; }
    Core::hresult Resume(const string&) override { return Core::ERROR_NONE; }
    Core::hresult Terminate(const string&) override { return Core::ERROR_NONE; }
    Core::hresult Kill(const string&) override { return Core::ERROR_NONE; }
    Core::hresult GetInfo(const string&, string&) override { return Core::ERROR_GENERAL; }
    Core::hresult Annotate(const string&, const string&, const string&) override { return Core::ERROR_NONE; }
    Core::hresult Mount() override { return Core::ERROR_NONE; }
    Core::hresult Unmount() override { return Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> mRefCount { 1 };
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
    FakeAppManager* appManager = new FakeAppManager(std::list<WPEFramework::Exchange::IAppManager::LoadedAppInfo>());
    WPEFramework::Plugin::VictimSelectorImplementation* selector = CreateSelector(appManager);

    const WPEFramework::Core::hresult status = selector->Evict(
        WPEFramework::Exchange::IVictimSelector::EVICTION_REASON_GPU,
        WPEFramework::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    L0Test::ExpectEqU32(result, status, WPEFramework::Core::ERROR_UNAVAILABLE, "Non-RAM evictions are rejected");
    L0Test::ExpectEqU32(result, appManager->getLoadedAppsCalls, 0, "Rejected eviction does not load app list");

    selector->Release();
    return result.failures;
}

uint32_t Test_VS_SoftEvictionSelectsPausedCandidate()
{
    L0Test::TestResult result;
    std::list<WPEFramework::Exchange::IAppManager::LoadedAppInfo> apps;
    apps.push_back(LoadedApp("protected", WPEFramework::Exchange::IAppManager::APP_STATE_PAUSED));
    apps.push_back(LoadedApp("paused", WPEFramework::Exchange::IAppManager::APP_STATE_PAUSED));
    apps.push_back(LoadedApp("suspended", WPEFramework::Exchange::IAppManager::APP_STATE_SUSPENDED));
    FakeAppManager* appManager = new FakeAppManager(apps);
    appManager->priorities["protected"] = "0";
    appManager->priorities["paused"] = "2";
    appManager->priorities["suspended"] = "2";
    WPEFramework::Plugin::VictimSelectorImplementation* selector = CreateSelector(appManager);

    const WPEFramework::Core::hresult status = selector->Evict(
        WPEFramework::Exchange::IVictimSelector::EVICTION_REASON_RAM,
        WPEFramework::Exchange::IVictimSelector::EVICTION_TYPE_SOFT);
    L0Test::ExpectEqU32(result, status, WPEFramework::Core::ERROR_NONE, "RAM eviction starts successfully");
    L0Test::ExpectEqStr(result, appManager->terminatedAppId, "paused", "Single eligible paused app is selected before suspended app");
    L0Test::ExpectTrue(result, appManager->killedAppId.empty(), "Soft eviction does not kill the selected paused app");

    selector->Release();
    return result.failures;
}