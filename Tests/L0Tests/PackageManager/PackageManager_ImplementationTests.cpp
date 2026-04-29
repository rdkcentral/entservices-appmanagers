#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <unistd.h>

#include "PackageManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

using WPEFramework::Core::ERROR_BAD_REQUEST;
using WPEFramework::Core::ERROR_GENERAL;
using WPEFramework::Core::ERROR_INVALID_SIGNATURE;
using WPEFramework::Core::ERROR_NONE;

class FakeDownloaderNotification final : public WPEFramework::Exchange::IPackageDownloader::INotification {
public:
    FakeDownloaderNotification()
        : _refCount(1)
        , downloadStatusCount(0)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPackageDownloader::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageDownloader::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppDownloadStatus(WPEFramework::Exchange::IPackageDownloader::IPackageInfoIterator* const) override
    {
        downloadStatusCount++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> downloadStatusCount;
};

class FakeInstallerNotification final : public WPEFramework::Exchange::IPackageInstaller::INotification {
public:
    FakeInstallerNotification()
        : _refCount(1)
        , installStatusCount(0)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPackageInstaller::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppInstallationStatus(const std::string&) override
    {
        installStatusCount++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> installStatusCount;
};

struct ImplFixture {
    L0Test::FakeStorageManager storage;
    L0Test::ServiceMock::FakeSubSystem subSystem;
    L0Test::ServiceMock service;
    WPEFramework::Plugin::PackageManagerImplementation* impl;

    ImplFixture()
        : storage()
        , subSystem()
        , service(L0Test::ServiceMock::Config(&storage, &subSystem, nullptr))
        , impl(WPEFramework::Core::Service<WPEFramework::Plugin::PackageManagerImplementation>::Create<
            WPEFramework::Plugin::PackageManagerImplementation>())
    {
    }

    ~ImplFixture()
    {
        if (impl != nullptr) {
            impl->Deinitialize(&service);
            impl->Release();
            impl = nullptr;
        }
    }

    uint32_t Initialize()
    {
        const uint32_t rc = impl->Initialize(&service);
        if (rc == ERROR_NONE) {
            // PackageManager initializes cache asynchronously; wait until marker is published.
            for (uint32_t i = 0; i < 250; ++i) {
                if (0 == access(PACKAGE_MANAGER_MARKER_FILE, F_OK)) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        return rc;
    }
};

} // namespace

uint32_t Test_PM_Impl_InitializeAndDeinitialize()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    const auto result = fx.Initialize();
    L0Test::ExpectEqU32(tr, result, ERROR_NONE, "Initialize() returns ERROR_NONE with valid service and storage manager");

    return tr.failures;
}

uint32_t Test_PM_Impl_DownloaderRegisterUnregister()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    auto* notif = new FakeDownloaderNotification();
    L0Test::ExpectEqU32(tr, fx.impl->Register(notif), ERROR_NONE, "Downloader Register() returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, fx.impl->Unregister(notif), ERROR_NONE, "Downloader Unregister() returns ERROR_NONE");
    notif->Release();

    return tr.failures;
}

uint32_t Test_PM_Impl_InstallerRegisterUnregister()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    auto* notif = new FakeInstallerNotification();
    L0Test::ExpectEqU32(tr, fx.impl->Register(notif), ERROR_NONE, "Installer Register() returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, fx.impl->Unregister(notif), ERROR_NONE, "Installer Unregister() returns ERROR_NONE");
    notif->Release();

    return tr.failures;
}

uint32_t Test_PM_Impl_UnregisterUnknownNotification()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    auto* d = new FakeDownloaderNotification();
    auto* i = new FakeInstallerNotification();
    L0Test::ExpectEqU32(tr, fx.impl->Unregister(d), ERROR_GENERAL, "Downloader Unregister() of unknown notification returns ERROR_GENERAL");
    L0Test::ExpectEqU32(tr, fx.impl->Unregister(i), ERROR_GENERAL, "Installer Unregister() of unknown notification returns ERROR_GENERAL");
    d->Release();
    i->Release();

    return tr.failures;
}

uint32_t Test_PM_Impl_ListPackagesAndPackageStateForDummyData()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    WPEFramework::Exchange::IPackageInstaller::IPackageIterator* packages = nullptr;
    L0Test::ExpectEqU32(tr, fx.impl->ListPackages(packages), ERROR_NONE, "ListPackages() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, packages != nullptr, "ListPackages() returns non-null iterator");

    WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
    L0Test::ExpectEqU32(tr,
                        fx.impl->PackageState("YouTube", "100.1.24", state),
                        ERROR_NONE,
                        "PackageState() for dummy package returns ERROR_NONE");
    L0Test::ExpectTrue(tr,
                       state == WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED,
                       "Dummy package starts in INSTALLED state");

    if (packages != nullptr) {
        packages->Release();
    }

    return tr.failures;
}

uint32_t Test_PM_Impl_ConfigAndGetConfigForPackageEmptyLocator()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    WPEFramework::Exchange::RuntimeConfig config {};
    L0Test::ExpectEqU32(tr,
                        fx.impl->Config("YouTube", "100.1.24", config),
                        ERROR_NONE,
                        "Config() for installed dummy package returns ERROR_NONE");
    L0Test::ExpectEqStr(tr, config.appPath, "/opt/YouTube", "Config() returns expected appPath from dummy metadata");

    std::string id;
    std::string version;
    WPEFramework::Exchange::RuntimeConfig cfg {};
    L0Test::ExpectEqU32(tr,
                        fx.impl->GetConfigForPackage("", id, version, cfg),
                        ERROR_INVALID_SIGNATURE,
                        "GetConfigForPackage() with empty locator returns ERROR_INVALID_SIGNATURE");

    return tr.failures;
}

uint32_t Test_PM_Impl_PauseResumeCancelProgressRateLimitWithoutActiveDownload()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    WPEFramework::Exchange::IPackageDownloader::ProgressInfo progress {};
    L0Test::ExpectEqU32(tr, fx.impl->Pause("1"), ERROR_GENERAL, "Pause() with no active download returns ERROR_GENERAL");
    L0Test::ExpectEqU32(tr, fx.impl->Resume("1"), ERROR_GENERAL, "Resume() with no active download returns ERROR_GENERAL");
    L0Test::ExpectEqU32(tr, fx.impl->Cancel("1"), ERROR_GENERAL, "Cancel() with no active download returns ERROR_GENERAL");
    L0Test::ExpectEqU32(tr, fx.impl->Progress("1", progress), ERROR_GENERAL, "Progress() with no active download returns ERROR_GENERAL");
    L0Test::ExpectEqU32(tr, fx.impl->RateLimit("1", 1024), ERROR_GENERAL, "RateLimit() with no active download returns ERROR_GENERAL");

    return tr.failures;
}

uint32_t Test_PM_Impl_DeleteFilePaths()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    const std::string existing = "/tmp/packagemanager_l0_delete_me.bin";
    FILE* f = std::fopen(existing.c_str(), "wb");
    if (f != nullptr) {
        std::fwrite("ok", 1, 2, f);
        std::fclose(f);
    }

    L0Test::ExpectEqU32(tr, fx.impl->Delete(existing), ERROR_NONE, "Delete() on existing file returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, fx.impl->Delete(existing), ERROR_GENERAL, "Delete() on missing file returns ERROR_GENERAL");

    return tr.failures;
}

uint32_t Test_PM_Impl_InstallAndUninstallFlowWithNotifications()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    auto* notif = new FakeInstallerNotification();
    L0Test::ExpectEqU32(tr, fx.impl->Register(notif), ERROR_NONE, "Installer Register() returns ERROR_NONE");

    WPEFramework::Exchange::IPackageInstaller::FailReason failReason = WPEFramework::Exchange::IPackageInstaller::FailReason::NONE;
    const auto installResult = fx.impl->Install("SampleApp", "1.0.0", nullptr, "/tmp/fake_pkg.ipk", failReason);
    L0Test::ExpectEqU32(tr, installResult, ERROR_NONE, "Install() returns ERROR_NONE with dummy package impl");
    L0Test::ExpectTrue(tr, notif->installStatusCount.load() >= 2, "Install() emits installation notifications");

    std::string errorReason;
    L0Test::ExpectEqU32(tr, fx.impl->Uninstall("SampleApp", errorReason), ERROR_NONE, "Uninstall() returns ERROR_NONE for installed package");

    L0Test::ExpectEqU32(tr, fx.impl->Unregister(notif), ERROR_NONE, "Installer Unregister() returns ERROR_NONE");
    notif->Release();

    return tr.failures;
}

uint32_t Test_PM_Impl_LockUnlockAndGetLockedInfo()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    uint32_t lockId = 0;
    std::string unpackedPath;
    WPEFramework::Exchange::RuntimeConfig runtimeConfig {};
    WPEFramework::Exchange::IPackageHandler::ILockIterator* appMetadata = nullptr;

    L0Test::ExpectEqU32(tr,
                        fx.impl->Lock("YouTube", "100.1.24", WPEFramework::Exchange::IPackageHandler::LockReason::LAUNCH, lockId, unpackedPath, runtimeConfig, appMetadata),
                        ERROR_NONE,
                        "Lock() for dummy package returns ERROR_NONE");
    L0Test::ExpectTrue(tr, lockId > 0, "Lock() returns non-zero lockId");

    bool locked = false;
    std::string gatewayMetadataPath;
    L0Test::ExpectEqU32(tr,
                        fx.impl->GetLockedInfo("YouTube", "100.1.24", unpackedPath, runtimeConfig, gatewayMetadataPath, locked),
                        ERROR_NONE,
                        "GetLockedInfo() returns ERROR_NONE for known package");
    L0Test::ExpectTrue(tr, locked, "GetLockedInfo() reports package as locked");

    if (appMetadata != nullptr) {
        appMetadata->Release();
    }

    L0Test::ExpectEqU32(tr,
                        fx.impl->Unlock("YouTube", "100.1.24"),
                        ERROR_NONE,
                        "Unlock() for previously locked package returns ERROR_NONE");

    L0Test::ExpectEqU32(tr,
                        fx.impl->Lock("UnknownApp", "0.0.1", WPEFramework::Exchange::IPackageHandler::LockReason::LAUNCH, lockId, unpackedPath, runtimeConfig, appMetadata),
                        ERROR_BAD_REQUEST,
                        "Lock() for unknown package returns ERROR_BAD_REQUEST");

    return tr.failures;
}

uint32_t Test_PM_Impl_DownloadAndStorageInfoPaths()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    WPEFramework::Exchange::IPackageDownloader::Options options { false, 1, 0 };
    WPEFramework::Exchange::IPackageDownloader::DownloadId downloadId {};
    L0Test::ExpectEqU32(tr,
                        fx.impl->Download("http://127.0.0.1:1/not-found", options, downloadId),
                        ERROR_NONE,
                        "Download() returns ERROR_NONE when INTERNET subsystem is active");
    L0Test::ExpectTrue(tr, !downloadId.downloadId.empty(), "Download() returns non-empty downloadId");

    uint32_t quotaKB = 0;
    uint32_t usedKB = 0;
    L0Test::ExpectEqU32(tr,
                        fx.impl->GetStorageInformation(quotaKB, usedKB),
                        ERROR_NONE,
                        "GetStorageInformation() returns ERROR_NONE");

    return tr.failures;
}

uint32_t Test_PM_Impl_InstallInputValidationAndUnknownPaths()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    WPEFramework::Exchange::IPackageInstaller::FailReason failReason = WPEFramework::Exchange::IPackageInstaller::FailReason::NONE;
    L0Test::ExpectEqU32(tr,
                        fx.impl->Install("AppX", "1.0", nullptr, "", failReason),
                        ERROR_INVALID_SIGNATURE,
                        "Install() with empty fileLocator returns ERROR_INVALID_SIGNATURE");

    std::string reason;
    L0Test::ExpectEqU32(tr,
                        fx.impl->Uninstall("NoSuchApp", reason),
                        ERROR_BAD_REQUEST,
                        "Uninstall() for unknown app returns ERROR_BAD_REQUEST");

    WPEFramework::Exchange::RuntimeConfig cfg {};
    L0Test::ExpectEqU32(tr,
                        fx.impl->Config("NoSuchApp", "0", cfg),
                        ERROR_BAD_REQUEST,
                        "Config() for unknown package returns ERROR_BAD_REQUEST");

    WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
    L0Test::ExpectEqU32(tr,
                        fx.impl->PackageState("NoSuchApp", "0", state),
                        ERROR_BAD_REQUEST,
                        "PackageState() for unknown package returns ERROR_BAD_REQUEST");

    return tr.failures;
}

uint32_t Test_PM_Impl_GetLockedInfoAndUnlockNegativePaths()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    std::string unpackedPath;
    WPEFramework::Exchange::RuntimeConfig cfg {};
    std::string gatewayMetadataPath;
    bool locked = false;

    L0Test::ExpectEqU32(tr,
                        fx.impl->GetLockedInfo("UnknownApp", "0", unpackedPath, cfg, gatewayMetadataPath, locked),
                        ERROR_BAD_REQUEST,
                        "GetLockedInfo() for unknown package returns ERROR_BAD_REQUEST");

    L0Test::ExpectEqU32(tr,
                        fx.impl->Unlock("YouTube", "100.1.24"),
                        ERROR_GENERAL,
                        "Unlock() without prior lock on known package returns ERROR_GENERAL");

    return tr.failures;
}

uint32_t Test_PM_Impl_InitializeWithNullServiceReturnsError()
{
    L0Test::TestResult tr;
    auto* impl = WPEFramework::Core::Service<WPEFramework::Plugin::PackageManagerImplementation>::Create<
        WPEFramework::Plugin::PackageManagerImplementation>();

    L0Test::ExpectEqU32(tr,
                        impl->Initialize(nullptr),
                        ERROR_GENERAL,
                        "Initialize(nullptr) returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

uint32_t Test_PM_Impl_DownloadWithoutInternetReturnsUnavailable()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    fx.subSystem.internetActive = false;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    WPEFramework::Exchange::IPackageDownloader::Options options { false, 1, 0 };
    WPEFramework::Exchange::IPackageDownloader::DownloadId downloadId {};

    L0Test::ExpectEqU32(tr,
                        fx.impl->Download("http://127.0.0.1:1/offline", options, downloadId),
                        WPEFramework::Core::ERROR_UNAVAILABLE,
                        "Download() returns ERROR_UNAVAILABLE when INTERNET subsystem is inactive");

    return tr.failures;
}

uint32_t Test_PM_Impl_GetConfigForPackageSuccessPath()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    std::string id;
    std::string version;
    WPEFramework::Exchange::RuntimeConfig cfg {};

    L0Test::ExpectEqU32(tr,
                        fx.impl->GetConfigForPackage("/tmp/fake.pkg", id, version, cfg),
                        ERROR_NONE,
                        "GetConfigForPackage() with non-empty locator returns ERROR_NONE in UNIT_TEST path");

    return tr.failures;
}

uint32_t Test_PM_Impl_InstallDifferentVersionBlockedWhileLockedThenProcessedOnUnlock()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    uint32_t lockId = 0;
    std::string unpackedPath;
    WPEFramework::Exchange::RuntimeConfig runtimeConfig {};
    WPEFramework::Exchange::IPackageHandler::ILockIterator* appMetadata = nullptr;

    L0Test::ExpectEqU32(tr,
        fx.impl->Lock("YouTube", "100.1.24", WPEFramework::Exchange::IPackageHandler::LockReason::LAUNCH,
            lockId, unpackedPath, runtimeConfig, appMetadata),
        ERROR_NONE,
        "Lock() existing package before install-block test");

    if (appMetadata != nullptr) {
        appMetadata->Release();
        appMetadata = nullptr;
    }

    WPEFramework::Exchange::IPackageInstaller::FailReason failReason = WPEFramework::Exchange::IPackageInstaller::FailReason::NONE;
    const auto blockedInstall = fx.impl->Install("YouTube", "200.0.0", nullptr, "/tmp/youtube_200.pkg", failReason);
    L0Test::ExpectEqU32(tr,
        blockedInstall,
        ERROR_GENERAL,
        "Install() returns ERROR_GENERAL when newer version is blocked by existing lock");

    WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
    L0Test::ExpectEqU32(tr,
        fx.impl->PackageState("YouTube", "200.0.0", state),
        ERROR_NONE,
        "PackageState() for blocked version is queryable");
    L0Test::ExpectTrue(tr,
        state == WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLATION_BLOCKED,
        "State is INSTALLATION_BLOCKED while old version remains locked");

    L0Test::ExpectEqU32(tr,
        fx.impl->Unlock("YouTube", "100.1.24"),
        ERROR_NONE,
        "Unlock() processes blocked install");

    L0Test::ExpectEqU32(tr,
        fx.impl->PackageState("YouTube", "200.0.0", state),
        ERROR_NONE,
        "PackageState() for new version remains queryable after unlock");
    L0Test::ExpectTrue(tr,
        state == WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED,
        "Blocked install transitions to INSTALLED after unlock");

    return tr.failures;
}

uint32_t Test_PM_Impl_UninstallBlockedWhileLockedThenProcessedOnUnlock()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    uint32_t lockId = 0;
    std::string unpackedPath;
    WPEFramework::Exchange::RuntimeConfig runtimeConfig {};
    WPEFramework::Exchange::IPackageHandler::ILockIterator* appMetadata = nullptr;

    L0Test::ExpectEqU32(tr,
        fx.impl->Lock("YouTube", "100.1.24", WPEFramework::Exchange::IPackageHandler::LockReason::LAUNCH,
            lockId, unpackedPath, runtimeConfig, appMetadata),
        ERROR_NONE,
        "Lock() existing package before uninstall-block test");

    if (appMetadata != nullptr) {
        appMetadata->Release();
        appMetadata = nullptr;
    }

    std::string errorReason;
    const auto blockedUninstall = fx.impl->Uninstall("YouTube", errorReason);
    L0Test::ExpectEqU32(tr,
        blockedUninstall,
        ERROR_GENERAL,
        "Uninstall() returns ERROR_GENERAL while package is locked (blocked uninstall)");

    WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
    L0Test::ExpectEqU32(tr,
        fx.impl->PackageState("YouTube", "100.1.24", state),
        ERROR_NONE,
        "PackageState() remains queryable during blocked uninstall");
    L0Test::ExpectTrue(tr,
        state == WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALL_BLOCKED,
        "State is UNINSTALL_BLOCKED while lock is held");

    L0Test::ExpectEqU32(tr,
        fx.impl->Unlock("YouTube", "100.1.24"),
        ERROR_NONE,
        "Unlock() processes blocked uninstall");

    L0Test::ExpectEqU32(tr,
        fx.impl->PackageState("YouTube", "100.1.24", state),
        ERROR_NONE,
        "PackageState() is queryable after blocked uninstall processing");
    L0Test::ExpectTrue(tr,
        state == WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED,
        "Blocked uninstall transitions to UNINSTALLED after unlock");

    return tr.failures;
}

uint32_t Test_PM_Impl_InstallFailureReasonBranches()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    const std::string versions[] = { "1.0.1", "1.0.2", "1.0.3" };
    const std::string ids[] = { "MismatchApp", "PersistFailApp", "VerifyFailApp" };

    for (size_t idx = 0; idx < 3; ++idx) {
        WPEFramework::Exchange::IPackageInstaller::FailReason failReason = WPEFramework::Exchange::IPackageInstaller::FailReason::NONE;
        const auto rc = fx.impl->Install(ids[idx], versions[idx], nullptr, "/tmp/failure.pkg", failReason);
        L0Test::ExpectEqU32(tr,
            rc,
            ERROR_GENERAL,
            std::string("Install() failure branch returns ERROR_GENERAL for ") + ids[idx]);

        WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
        L0Test::ExpectEqU32(tr,
            fx.impl->PackageState(ids[idx], versions[idx], state),
            ERROR_NONE,
            std::string("PackageState() is queryable after failed install for ") + ids[idx]);
        L0Test::ExpectTrue(tr,
            state == WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALL_FAILURE,
            std::string("Failed install transitions to INSTALL_FAILURE for ") + ids[idx]);
    }

    return tr.failures;
}
