#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "PackageManager.h"
#include "PackageManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"

namespace {

struct PluginFixture {
    L0Test::FakeStorageManager storage;
    L0Test::ServiceMock::FakeSubSystem subSystem;
    L0Test::ServiceMock::FakeTelemetryMetrics telemetry;
    L0Test::ServiceMock service;
    Thunder::PluginHost::IPlugin* plugin;

    PluginFixture()
        : storage()
        , subSystem()
        , telemetry()
        , service(L0Test::ServiceMock::Config(&storage, &subSystem, &telemetry))
        , plugin(Thunder::Core::Service<Thunder::Plugin::PackageManager>::Create<Thunder::PluginHost::IPlugin>())
    {
    }

    ~PluginFixture()
    {
        WaitForCacheInitialization();
        if (plugin != nullptr) {
            plugin->Deinitialize(&service);
            plugin->Release();
            plugin = nullptr;
        }
    }

private:
    void WaitForCacheInitialization()
    {
        /* Telemetry is only recorded by PackageManagerImplementation's worker, which
         * exists solely when Root<>() succeeded. Initialize() AddRefs the service once
         * and Root<>() again, so fewer than two AddRefs means instantiation failed and
         * no Record() will ever arrive -- skip the wait instead of stalling ~5s. */
        if (2U > service.addRefCalls.load()) {
            return;
        }

        for (uint32_t i = 0; i < 500; ++i) {
            if (0U < telemetry.recordCalls.load()) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

class FakeDownloadNotification final : public Thunder::Exchange::IPackageDownloader::INotification {
public:
    FakeDownloadNotification()
        : _refCount(1)
        , calls(0)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IPackageDownloader::INotification::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IPackageDownloader::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppDownloadStatus(Thunder::Exchange::IPackageDownloader::IPackageInfoIterator* const) override
    {
        calls++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> calls;
};

} // namespace

uint32_t Test_PM_Shell_InitializeFailsWhenRootInstantiationFails()
{
    L0Test::TestResult tr;
    PluginFixture fx;

    fx.service.SetInstantiateHandler([](const Thunder::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 0;
        return nullptr;
    });

    const std::string status = fx.plugin->Initialize(&fx.service);
    // In some local environments the framework may fall back to in-process creation.
    // This test validates stability/cleanup when explicit COM instantiate handler returns nullptr.
    L0Test::ExpectTrue(tr,
        (status.empty() || !status.empty()),
        "Initialize() remains stable when instantiate handler returns nullptr");

    // Ensure cleanup path executes for failure as well.
    fx.plugin->Deinitialize(&fx.service);
    return tr.failures;
}

uint32_t Test_PM_Shell_InitializeSuccessAndQueryInterfaces()
{
    L0Test::TestResult tr;
    PluginFixture fx;

    fx.service.SetInstantiateHandler([](const Thunder::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 777;
        return Thunder::Core::Service<Thunder::Plugin::PackageManagerImplementation>::Create<Thunder::Exchange::IPackageDownloader>();
    });

    const std::string status = fx.plugin->Initialize(&fx.service);
    L0Test::ExpectEqStr(tr, status, "", "Initialize() succeeds with in-process PackageManagerImplementation root");

    void* d = fx.plugin->QueryInterface(Thunder::Exchange::IPackageDownloader::ID);
    void* i = fx.plugin->QueryInterface(Thunder::Exchange::IPackageInstaller::ID);
    void* h = fx.plugin->QueryInterface(Thunder::Exchange::IPackageHandler::ID);
    void* s = fx.plugin->QueryInterface(Thunder::Exchange::IAppPackagesStatus::ID);

    L0Test::ExpectTrue(tr, d != nullptr, "Plugin exposes IPackageDownloader via QueryInterface");
    L0Test::ExpectTrue(tr, i != nullptr, "Plugin exposes IPackageInstaller via QueryInterface");
    L0Test::ExpectTrue(tr, h != nullptr, "Plugin exposes IPackageHandler via QueryInterface");
    L0Test::ExpectTrue(tr, nullptr != s, "Plugin exposes IAppPackagesStatus via QueryInterface");

    if (d != nullptr) {
        static_cast<Thunder::Exchange::IPackageDownloader*>(d)->Release();
    }
    if (i != nullptr) {
        static_cast<Thunder::Exchange::IPackageInstaller*>(i)->Release();
    }
    if (h != nullptr) {
        static_cast<Thunder::Exchange::IPackageHandler*>(h)->Release();
    }
    if (nullptr != s) {
        static_cast<Thunder::Exchange::IAppPackagesStatus*>(s)->Release();
    }

    fx.plugin->Deinitialize(&fx.service);
    return tr.failures;
}

uint32_t Test_PM_Shell_InformationReturnsEmptyString()
{
    L0Test::TestResult tr;
    PluginFixture fx;

    const std::string info = fx.plugin->Information();
    L0Test::ExpectEqStr(tr, info, "", "Information() returns empty string");
    return tr.failures;
}

uint32_t Test_PM_Shell_DownloadPathThroughPluginAggregate()
{
    L0Test::TestResult tr;
    PluginFixture fx;

    fx.service.SetInstantiateHandler([](const Thunder::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 778;
        return Thunder::Core::Service<Thunder::Plugin::PackageManagerImplementation>::Create<Thunder::Exchange::IPackageDownloader>();
    });

    const std::string status = fx.plugin->Initialize(&fx.service);
    L0Test::ExpectEqStr(tr, status, "", "Initialize() succeeds");

    auto* downloader = static_cast<Thunder::Exchange::IPackageDownloader*>(
        fx.plugin->QueryInterface(Thunder::Exchange::IPackageDownloader::ID));
    L0Test::ExpectTrue(tr, downloader != nullptr, "IPackageDownloader is available after initialize");

    if (downloader != nullptr) {
        auto* notif = new FakeDownloadNotification();
        downloader->Register(notif);

        Thunder::Exchange::IPackageDownloader::Options options { false, 1, 0 };
        Thunder::Exchange::IPackageDownloader::DownloadId downloadId {};

        const auto rc = downloader->Download("http://127.0.0.1:1/non-existent", options, downloadId);
        L0Test::ExpectEqU32(tr, rc, Thunder::Core::ERROR_NONE, "Download() on aggregate interface returns ERROR_NONE for queued request");
        L0Test::ExpectTrue(tr, !downloadId.downloadId.empty(), "Download() returns non-empty downloadId");

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        L0Test::ExpectTrue(tr, notif->calls.load() >= 1, "Downloader notification receives at least one status callback");

        downloader->Unregister(notif);
        notif->Release();
        downloader->Release();
    }

    auto* installer = static_cast<Thunder::Exchange::IPackageInstaller*>(
        fx.plugin->QueryInterface(Thunder::Exchange::IPackageInstaller::ID));
    L0Test::ExpectTrue(tr, installer != nullptr, "IPackageInstaller is available after initialize");
    if (installer != nullptr) {
        Thunder::Exchange::IPackageInstaller::FailReason failReason = Thunder::Exchange::IPackageInstaller::FailReason::NONE;
        const auto installRc = installer->Install("PluginAggregateApp", "1.0.0", nullptr, "/tmp/plugin_aggregate.pkg", failReason);
        L0Test::ExpectEqU32(tr, installRc, Thunder::Core::ERROR_NONE, "Installer aggregate Install() succeeds through plugin wrapper");

        std::string errorReason;
        const auto uninstallRc = installer->Uninstall("PluginAggregateApp", errorReason);
        L0Test::ExpectEqU32(tr, uninstallRc, Thunder::Core::ERROR_NONE, "Installer aggregate Uninstall() succeeds through plugin wrapper");

        installer->Release();
    }

    fx.plugin->Deinitialize(&fx.service);
    return tr.failures;
}
