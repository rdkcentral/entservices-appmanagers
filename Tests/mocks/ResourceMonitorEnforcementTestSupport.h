#pragma once

#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <vector>
#include "ResourceMonitorImplementation.h"
#include "../L0Tests/common/AppManagerL0Mock.hpp"

namespace ResourceMonitorTestSupport {

using namespace WPEFramework;
using Monitor = Plugin::ResourceMonitorImplementation;
using State = Exchange::IAppManager::AppLifecycleState;

class AppManager : public Exchange::IAppManager {
public:
    uint32_t AddRef() const override { return ++references; }
    uint32_t Release() const override
    {
        const auto remaining = --references;
        if (remaining == 0) { delete this; }
        return remaining;
    }
    void* QueryInterface(const uint32_t) override { return nullptr; }
    Core::hresult Register(INotification* sink) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        notification = sink;
        notification->AddRef();
        return Core::ERROR_NONE;
    }
    Core::hresult Unregister(INotification* sink) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (notification == sink) {
            notification->Release();
            notification = nullptr;
        }
        return Core::ERROR_NONE;
    }
    Core::hresult GetLoadedApps(ILoadedAppInfoIterator*& result) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        result = nullptr;
        if (listResult != Core::ERROR_NONE) { return listResult; }
        result = Core::Service<RPC::IteratorType<ILoadedAppInfoIterator>>::Create<ILoadedAppInfoIterator>(apps);
        return Core::ERROR_NONE;
    }
    Core::hresult TerminateApp(const string& appId) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        terminations.push_back(appId);
        return Core::ERROR_NONE;
    }
    void Add(const string& appId, const string& instanceId, const string& type)
    {
        std::lock_guard<std::mutex> lock(mutex);
        LoadedAppInfo app {};
        app.appId = appId;
        app.appInstanceId = instanceId;
        app.type = type;
        app.lifecycleState = APP_STATE_ACTIVE;
        apps.push_back(app);
    }
    void Emit(const string& instanceId, State state)
    {
        string appId;
        State oldState = APP_STATE_UNKNOWN;
        INotification* sink = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex);
            for (auto& app : apps) {
                if (app.appInstanceId == instanceId) {
                    appId = app.appId;
                    oldState = app.lifecycleState;
                    app.lifecycleState = state;
                    break;
                }
            }
            sink = notification;
            if (sink) { sink->AddRef(); }
        }
        if (sink) {
            sink->OnAppLifecycleStateChanged(appId, instanceId, state, oldState, APP_ERROR_NONE);
            sink->Release();
        }
    }
    std::vector<string> Terminations()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return terminations;
    }
    Core::hresult GetInstalledApps(string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult IsInstalled(const string&, bool&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult LaunchApp(const string&, const string&, const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult PreloadApp(const string&, const string&, const string&, string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult CloseApp(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult StartSystemApp(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult StopSystemApp(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult KillApp(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult SendIntent(const string&, const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult ClearAppData(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult ClearAllAppData() override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult GetAppMetadata(const string&, const string&, string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult GetAppProperty(const string&, const string&, string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult SetAppProperty(const string&, const string&, const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult GetMaxRunningApps(int32_t&) const override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult GetMaxHibernatedApps(int32_t&) const override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult GetMaxHibernatedFlashUsage(int32_t&) const override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult GetMaxInactiveRamUsage(int32_t&) const override { return Core::ERROR_UNAVAILABLE; }

    Core::hresult listResult { Core::ERROR_NONE };

private:
    mutable std::atomic<uint32_t> references { 1 };
    std::mutex mutex;
    std::list<LoadedAppInfo> apps;
    INotification* notification { nullptr };
    std::vector<string> terminations;
};

class RuntimeManager : public Exchange::IRuntimeManager {
public:
    uint32_t AddRef() const override { return ++references; }
    uint32_t Release() const override
    {
        const auto remaining = --references;
        if (remaining == 0) { delete this; }
        return remaining;
    }
    void* QueryInterface(const uint32_t) override { return nullptr; }
    Core::hresult GetInfo(const string& id, string& info) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++calls;
        if (result != Core::ERROR_NONE) { return result; }
        const auto entry = data.find(id);
        if (entry == data.end()) { return Core::ERROR_UNAVAILABLE; }
        info = entry->second;
        return Core::ERROR_NONE;
    }
    void SetInfo(const string& id, const string& info)
    {
        std::lock_guard<std::mutex> lock(mutex);
        data[id] = info;
    }
    Core::hresult Register(INotification*) override { return Core::ERROR_NONE; }
    Core::hresult Unregister(INotification*) override { return Core::ERROR_NONE; }
    Core::hresult Run(const string&, const string&, const uint32_t, const uint32_t, IValueIterator* const&, IStringIterator* const&, IStringIterator* const&, const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Hibernate(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Wake(const string&, const RuntimeState) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Suspend(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Resume(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Terminate(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Kill(const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Annotate(const string&, const string&, const string&) override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Mount() override { return Core::ERROR_UNAVAILABLE; }
    Core::hresult Unmount() override { return Core::ERROR_UNAVAILABLE; }

    std::atomic<uint32_t> calls { 0 };
    Core::hresult result { Core::ERROR_NONE };

private:
    mutable std::atomic<uint32_t> references { 1 };
    std::mutex mutex;
    std::map<string, string> data;
};

class PackageSource : public Plugin::IPackageLifecycleSource {
public:
    Core::hresult ListPackages(std::vector<PackageEntry>& entries) override
    {
        for (const auto& item : payloads) { entries.push_back({ item.first, "1.0" }); }
        return Core::ERROR_NONE;
    }
    Core::hresult GetLockedInfo(const string& id, const string&, bool& locked, string& payload) override
    {
        if (onGetLockedInfo) {
            auto callback = std::move(onGetLockedInfo);
            onGetLockedInfo = nullptr;
            callback();
        }
        const auto found = payloads.find(id);
        locked = found != payloads.end();
        if (locked) { payload = found->second; }
        return Core::ERROR_NONE;
    }
    std::map<string, string> payloads;
    std::function<void()> onGetLockedInfo;
};

class Session {
    struct TransitionLog {
        std::mutex mutex;
        std::vector<std::pair<string, Exchange::ILifecycleManager::LifecycleState>> requests;
        std::vector<string> intents;
        Core::hresult result { Core::ERROR_NONE };
    };
public:
    explicit Session(Monitor* configured = nullptr)
        : monitor(configured ? configured : Core::Service<Monitor>::Create<Monitor>())
        , owned(configured == nullptr)
    {
        const auto log = transitionLog;
        lifecycle->setTargetAppStateHandler = [log](const string& id, Exchange::ILifecycleManager::LifecycleState state, const string& intent) {
            std::lock_guard<std::mutex> lock(log->mutex);
            log->requests.emplace_back(id, state);
            log->intents.push_back(intent);
            return log->result;
        };
        if (owned) {
            L0Test::AppManagerServiceMock::Config config;
            config.lifecycleManager = lifecycle;
            service.reset(new L0Test::AppManagerServiceMock(config));
            monitor->_rservice = service.get();
            monitor->_rservice->AddRef();
            monitor->_appManager = new AppManager();
            monitor->_runtimeManager = new RuntimeManager();
            monitor->_appManager->Register(&monitor->_appManagerNotification);
        }
        apps = static_cast<AppManager*>(monitor->_appManager);
        runtime = static_cast<RuntimeManager*>(monitor->_runtimeManager);
        auto source = std::unique_ptr<PackageSource>(new PackageSource());
        packages = source.get();
        monitor->_lifecycleConfigProvider.reset(new Plugin::PackageLifecycleConfigProvider(std::move(source)));
    }
    ~Session()
    {
        if (owned) { monitor->Release(); }
        lifecycle->Release();
    }
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void Add(const string& id, const string& states = "\"paused\",\"suspended\",\"hibernated\"", const string& type = "INTERACTIVE_APP")
    {
        apps->Add(id, id + ".instance", type);
        packages->payloads[id] = "{\"ralfPkgPath\":\"/metadata.json\",\"applicationLifecycle\":{\"supportedNonActiveStates\":[" + states +
            "],\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}}";
    }
    void Emit(const string& id, State state) { apps->Emit(id + ".instance", state); }
    std::vector<std::pair<string, Exchange::ILifecycleManager::LifecycleState>> Transitions()
    {
        std::lock_guard<std::mutex> lock(transitionLog->mutex);
        return transitionLog->requests;
    }
    std::vector<string> Intents()
    {
        std::lock_guard<std::mutex> lock(transitionLog->mutex);
        return transitionLog->intents;
    }
    void SetTransitionResult(Core::hresult result)
    {
        std::lock_guard<std::mutex> lock(transitionLog->mutex);
        transitionLog->result = result;
    }
    uint64_t Remaining(const string& id, bool paused)
    {
        std::lock_guard<std::mutex> lock(monitor->_appTimersMutex);
        const auto& state = monitor->_appTimers.at(id + ".instance");
        const uint64_t expiry = paused ? state.pausedTimerExpiryMs : state.suspendedTimerExpiryMs;
        const uint64_t now = Now();
        return expiry > now ? expiry - now : 0;
    }
    void Expire(const string& id, bool paused)
    {
        {
            std::lock_guard<std::mutex> lock(monitor->_appTimersMutex);
            auto& state = monitor->_appTimers.at(id + ".instance");
            (paused ? state.pausedTimerExpiryMs : state.suspendedTimerExpiryMs) = Now() - 1;
        }
        monitor->OnLifecycleTimerExpired();
    }
    static uint64_t Now()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    Monitor* monitor;
    AppManager* apps;
    RuntimeManager* runtime;
    PackageSource* packages;
    L0Test::MockLifecycleManager* lifecycle { new L0Test::MockLifecycleManager() };

private:
    std::shared_ptr<TransitionLog> transitionLog { new TransitionLog() };
    std::unique_ptr<L0Test::AppManagerServiceMock> service;
    bool owned;
};

}
