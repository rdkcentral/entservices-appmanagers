#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include "PackageManagerImplementation.h"
#include "RuntimeConfigPayload.h"
#include "UtilsMemorySize.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

using WPEFramework::Core::ERROR_GENERAL;
using WPEFramework::Core::ERROR_INVALID_PARAMETER;
using WPEFramework::Core::ERROR_INVALID_SIGNATURE;
using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Core::ERROR_NOT_SUPPORTED;

class FakeDownloaderNotification final : public WPEFramework::Exchange::IPackageDownloader::INotification {
public:
    FakeDownloaderNotification()
        : _refCount(1)
        , downloadStatusCount(0)
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
        , lastInstallStatus()
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

    void OnAppInstallationStatus(const std::string& status) override
    {
        lastInstallStatus = status;
        installStatusCount++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> installStatusCount;
    std::string lastInstallStatus;
};

struct ImplFixture {
    L0Test::FakeStorageManager storage;
    L0Test::ServiceMock::FakeSubSystem subSystem;
    L0Test::ServiceMock::FakeTelemetryMetrics telemetry;
    L0Test::ServiceMock service;
    WPEFramework::Plugin::PackageManagerImplementation* impl;

    ImplFixture()
        : storage()
        , subSystem()
        , telemetry()
        , service(L0Test::ServiceMock::Config(&storage, &subSystem, &telemetry))
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
            // The marker is written before telemetry; retain the fixture until the
            // background initialization has completed its final shell access.
            for (uint32_t i = 0; i < 500; ++i) {
                if (0U < telemetry.recordCalls.load()) {
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
    if (packages != nullptr) {
        WPEFramework::Exchange::IPackageInstaller::Package package;
        bool foundPackage = false;
        while (packages->Next(package)) {
            if (package.packageId == "YouTube" && package.version == "100.1.24") {
                foundPackage = true;
                L0Test::ExpectEqStr(tr,
                                    package.digest,
                                    "dummy-md5-youtube-100124",
                                    "ListPackages() returns digest populated from ConfigMetaData.md5Hash");
                break;
            }
        }
        L0Test::ExpectTrue(tr, foundPackage, "ListPackages() contains dummy package entry");
    }

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

    std::string config;
    L0Test::ExpectEqU32(tr,
                        fx.impl->Config("YouTube", "100.1.24", config),
                        ERROR_NONE,
                        "Config() for installed dummy package returns ERROR_NONE");

    WPEFramework::Plugin::Utils::RuntimeConfigPayload payload;
    std::string error;
    bool present = false;
    L0Test::ExpectTrue(tr, payload.Parse(config, error), "Config() returns a valid JSON runtime payload");

    std::string value;
    L0Test::ExpectTrue(tr,
                       payload.GetString("appPath", value, present, error) && present && value == "/opt/YouTube",
                       "Config() payload returns expected appPath from dummy metadata");
    L0Test::ExpectTrue(tr,
                       payload.GetString("capabilities", value, present, error) && present && value == "dial-app,wan-lan,thunder,fkps",
                       "Config() payload returns expected runtime capabilities");

    std::vector<std::string> values;
    L0Test::ExpectTrue(tr,
                       payload.GetStringArray("envVariables", values, present, error) && present && values.empty(),
                       "Config() payload encodes envVariables as a JSON string array");
    L0Test::ExpectTrue(tr,
                       payload.GetStringArray("fkpsFiles", values, present, error) && present &&
                           values == std::vector<std::string>({ "file1", "file2", "file3" }),
                       "Config() payload encodes real fkpsFiles values as a JSON string array");
    L0Test::ExpectTrue(tr,
                       payload.GetStringArray("logLevels", values, present, error) && present && values.empty(),
                       "Config() payload encodes logLevels as a JSON string array");

    std::string id;
    std::string version;
    std::string cfg;
    L0Test::ExpectEqU32(tr,
                        fx.impl->GetConfigForPackage("", id, version, cfg),
                        ERROR_INVALID_SIGNATURE,
                        "GetConfigForPackage() with empty locator returns ERROR_INVALID_SIGNATURE");

    return tr.failures;
}

uint32_t Test_PM_Impl_RuntimeConfigPayloadSerialization()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::Utils::RuntimeConfigPayload canonical;
    canonical.SetString("appPath", "/opt/TestApp");
    canonical.SetStringArray("envVariables", { "HOME=/home/private", "LANG=en_US.UTF-8" });
    canonical.SetStringArray("fkpsFiles", { "/etc/test/one", "/etc/test/two" });
    canonical.SetStringArray("logLevels", { "INFO", "WARN" });
    canonical.SetUnsigned("maxUnsigned", std::numeric_limits<uint64_t>::max());

    std::string serialized;
    std::string error;
    L0Test::ExpectTrue(tr,
                       canonical.Serialize(serialized, error),
                       "RuntimeConfigPayload serializes a canonical opaque runtime payload");

    WPEFramework::Plugin::Utils::RuntimeConfigPayload decoded;
    bool present = false;
    std::vector<std::string> values;
    L0Test::ExpectTrue(tr, decoded.Parse(serialized, error), "Canonical runtime payload parses successfully");
    L0Test::ExpectTrue(tr,
                       decoded.GetStringArray("envVariables", values, present, error) && present &&
                           values == std::vector<std::string>({ "HOME=/home/private", "LANG=en_US.UTF-8" }),
                       "Canonical payload preserves envVariables array values");
    L0Test::ExpectTrue(tr,
                       decoded.GetStringArray("fkpsFiles", values, present, error) && present &&
                           values == std::vector<std::string>({ "/etc/test/one", "/etc/test/two" }),
                       "Canonical payload preserves fkpsFiles array values");
    L0Test::ExpectTrue(tr,
                       decoded.GetStringArray("logLevels", values, present, error) && present &&
                           values == std::vector<std::string>({ "INFO", "WARN" }),
                       "Canonical payload preserves logLevels array values");
    uint64_t unsignedValue = 0;
    L0Test::ExpectTrue(tr,
                       decoded.GetUnsigned("maxUnsigned", unsignedValue, present, error) && present &&
                           unsignedValue == std::numeric_limits<uint64_t>::max(),
                       "Unsigned payload values round trip through the full uint64 range");

    WPEFramework::Plugin::Utils::RuntimeConfigPayload opaque;
    L0Test::ExpectTrue(tr,
                       opaque.Parse("{\"runtimePath\":\"/old/runtime\",\"vendorExtension\":{\"enabled\":true}}", error),
                       "Opaque payload with an unknown property parses successfully");
    opaque.SetString("runtimePath", "/new/runtime");
    L0Test::ExpectTrue(tr, opaque.Serialize(serialized, error), "runtimePath mutation serializes successfully");

    WPEFramework::Core::JSON::VariantContainer object;
    L0Test::ExpectTrue(tr, object.FromString(serialized), "Mutated runtime payload remains valid JSON");
    const WPEFramework::Core::JSON::VariantContainer vendor = object["vendorExtension"].Object();
    L0Test::ExpectTrue(tr,
                       object.HasLabel("vendorExtension") && vendor.HasLabel("enabled") && vendor["enabled"].Boolean(),
                       "runtimePath mutation preserves unknown object properties");
    L0Test::ExpectTrue(tr,
                       object.HasLabel("runtimePath") && object["runtimePath"].String() == "/new/runtime",
                       "runtimePath mutation updates only the requested property");

    WPEFramework::Plugin::Utils::RuntimeConfigPayload environmentPayload;
    L0Test::ExpectTrue(tr,
                       environmentPayload.Parse("{\"envVariables\":[\"FIREBOLT_ENDPOINT=old1\",\"KEEP=value\",\"FIREBOLT_ENDPOINT=old2\"]}", error) &&
                           environmentPayload.UpsertEnvironment("FIREBOLT_ENDPOINT=new", error) &&
                           environmentPayload.GetStringArray("envVariables", values, present, error) && present &&
                           values == std::vector<std::string>({ "FIREBOLT_ENDPOINT=new", "KEEP=value" }),
                       "Environment upsert replaces duplicate definitions and preserves unrelated values");

    WPEFramework::Plugin::Utils::RuntimeConfigPayload malformed;
    L0Test::ExpectTrue(tr,
                       !malformed.Parse("{\"envVariables\":[\"VALID\"]", error),
                       "Malformed runtime payload serialization is rejected");
    L0Test::ExpectTrue(tr, !malformed.Parse("[]", error), "Non-object runtime payload is rejected");

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

    const std::string packageId = "YouTube";
    const std::string version = "100.1.24";
    std::string errorReason;
    L0Test::ExpectEqU32(tr,
                        fx.impl->Uninstall(packageId, errorReason),
                        ERROR_NONE,
                        "Uninstall() succeeds for the package loaded during cache initialization");

    WPEFramework::Exchange::IPackageInstaller::FailReason failReason = WPEFramework::Exchange::IPackageInstaller::FailReason::NONE;
    const auto installResult = fx.impl->Install(packageId, version, nullptr, "/tmp/fake_pkg.ipk", failReason);
    L0Test::ExpectEqU32(tr, installResult, ERROR_NONE, "Reinstall() returns ERROR_NONE with dummy package impl");
    L0Test::ExpectTrue(tr, notif->installStatusCount.load() >= 4, "Uninstall and reinstall emit installation notifications");

    WPEFramework::Exchange::IPackageInstaller::IPackageIterator* packages = nullptr;
    L0Test::ExpectEqU32(tr, fx.impl->ListPackages(packages), ERROR_NONE, "ListPackages() succeeds immediately after reinstall");
    L0Test::ExpectTrue(tr, nullptr != packages, "ListPackages() returns an iterator immediately after reinstall");
    if (nullptr != packages) {
        WPEFramework::Exchange::IPackageInstaller::Package package;
        bool foundPackage = false;
        while (packages->Next(package)) {
            if ((packageId == package.packageId) && (version == package.version)) {
                foundPackage = true;
                L0Test::ExpectEqU32(tr,
                                    package.sizeKb,
                                    31457280U,
                                    "Reinstalled package exposes dataImageSize through ListPackages() without reboot");
                break;
            }
        }
        L0Test::ExpectTrue(tr, foundPackage, "ListPackages() contains the reinstalled package");
        packages->Release();
    }

    L0Test::ExpectEqU32(tr, fx.impl->Unregister(notif), ERROR_NONE, "Installer Unregister() returns ERROR_NONE");
    notif->Release();

    return tr.failures;
}

uint32_t Test_PM_Impl_InstallCreateStorageFailureReportsInstallFailure()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    auto* notif = new FakeInstallerNotification();
    L0Test::ExpectEqU32(tr, fx.impl->Register(notif), ERROR_NONE, "Installer Register() returns ERROR_NONE");

    fx.storage.createStorageResult = ERROR_GENERAL;

    WPEFramework::Exchange::IPackageInstaller::FailReason failReason = WPEFramework::Exchange::IPackageInstaller::FailReason::NONE;
    const std::string packageId = "StorageCreateFailApp";
    const std::string version = "1.2.3";
    const auto installResult = fx.impl->Install(packageId, version, nullptr, "/tmp/storage_create_fail.pkg", failReason);
    L0Test::ExpectEqU32(tr,
                        installResult,
                        ERROR_GENERAL,
                        "Install() returns ERROR_GENERAL when CreateStorage() fails after successful package install");
    L0Test::ExpectTrue(tr, fx.storage.createStorageCalls.load() > 0, "CreateStorage() was attempted");

    WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
    L0Test::ExpectEqU32(tr,
                        fx.impl->PackageState(packageId, version, state),
                        ERROR_NONE,
                        "PackageState() is queryable after CreateStorage() failure");
    L0Test::ExpectTrue(tr,
                       state == WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALL_FAILURE,
                       "CreateStorage() failure reports INSTALL_FAILURE state");

    L0Test::ExpectTrue(tr,
                       notif->installStatusCount.load() >= 2,
                       "Install() emits INSTALLING and final failure notification when CreateStorage() fails");
    L0Test::ExpectTrue(tr,
                       notif->lastInstallStatus.find("\"state\":\"INSTALL_FAILURE\"") != std::string::npos,
                       "Final install notification includes INSTALL_FAILURE state");
    L0Test::ExpectTrue(tr,
                       notif->lastInstallStatus.find("\"failReason\":\"PERSISTENCE_FAILURE\"") != std::string::npos,
                       "Final install notification includes PERSISTENCE_FAILURE failReason");

    L0Test::ExpectEqU32(tr, fx.impl->Unregister(notif), ERROR_NONE, "Installer Unregister() returns ERROR_NONE");
    notif->Release();

    return tr.failures;
}

uint32_t Test_PM_Impl_UninstallDeleteStorageFailureReturnsGeneralAndUninstalledState()
{
    L0Test::TestResult tr;
    ImplFixture fx;

    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    auto* notif = new FakeInstallerNotification();
    L0Test::ExpectEqU32(tr, fx.impl->Register(notif), ERROR_NONE, "Installer Register() returns ERROR_NONE");

    WPEFramework::Exchange::IPackageInstaller::FailReason failReason = WPEFramework::Exchange::IPackageInstaller::FailReason::NONE;
    const std::string packageId = "DeleteStorageFailApp";
    const std::string version = "4.5.6";
    L0Test::ExpectEqU32(tr,
                        fx.impl->Install(packageId, version, nullptr, "/tmp/delete_storage_fail.pkg", failReason),
                        ERROR_NONE,
                        "Install() succeeds before uninstall partial-success path test");

    fx.storage.deleteStorageResult = ERROR_GENERAL;

    std::string errorReason;
    const auto uninstallResult = fx.impl->Uninstall(packageId, errorReason);
    L0Test::ExpectEqU32(tr,
                        uninstallResult,
                        ERROR_NONE,
                        "Uninstall() returns ERROR_NONE when DeleteStorage() fails - package binaries removed successfully, storage cleanup is advisory (telemetry captures persistence failure)");
    L0Test::ExpectTrue(tr, fx.storage.deleteStorageCalls.load() > 0, "DeleteStorage() was attempted");

    WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    L0Test::ExpectEqU32(tr,
                        fx.impl->PackageState(packageId, version, state),
                        ERROR_NONE,
                        "PackageState() is queryable after DeleteStorage() failure");
    L0Test::ExpectTrue(tr,
                       state == WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED,
                       "DeleteStorage() failure keeps package state as UNINSTALLED (partial success semantics)");

    L0Test::ExpectTrue(tr,
                       notif->installStatusCount.load() >= 4,
                       "Install+Uninstall flow emits expected notifications including final UNINSTALLED status");
    L0Test::ExpectTrue(tr,
                       notif->lastInstallStatus.find("\"state\":\"UNINSTALLED\"") != std::string::npos,
                       "Final uninstall notification includes UNINSTALLED state");
    L0Test::ExpectTrue(tr,
                       notif->lastInstallStatus.find("\"failReason\"") == std::string::npos,
                       "Final UNINSTALLED notification does not include failReason field");

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
    std::string runtimeConfig;
    WPEFramework::Exchange::IPackageHandler::ILockIterator* appMetadata = nullptr;

    L0Test::ExpectEqU32(tr,
                        fx.impl->Lock("YouTube", "100.1.24", WPEFramework::Exchange::IPackageHandler::LockReason::LAUNCH, lockId, unpackedPath, runtimeConfig, appMetadata),
                        ERROR_NONE,
                        "Lock() for dummy package returns ERROR_NONE");
    L0Test::ExpectTrue(tr, lockId > 0, "Lock() returns non-zero lockId");

    WPEFramework::Plugin::Utils::RuntimeConfigPayload lockPayload;
    std::string error;
    bool present = false;
    std::vector<std::string> values;
    L0Test::ExpectTrue(tr, lockPayload.Parse(runtimeConfig, error), "Lock() returns a valid JSON runtime payload");
    L0Test::ExpectTrue(tr,
                       lockPayload.GetStringArray("envVariables", values, present, error) && present,
                       "Lock() payload contains envVariables as a JSON string array");
    L0Test::ExpectTrue(tr,
                       lockPayload.GetStringArray("fkpsFiles", values, present, error) && present,
                       "Lock() payload contains fkpsFiles as a JSON string array");
    L0Test::ExpectTrue(tr,
                       lockPayload.GetStringArray("logLevels", values, present, error) && present,
                       "Lock() payload contains logLevels as a JSON string array");
    std::string payloadUnpackedPath;
    L0Test::ExpectTrue(tr,
                       lockPayload.GetString("unpackedPath", payloadUnpackedPath, present, error) && present,
                       "Lock() payload contains unpackedPath");
    L0Test::ExpectEqStr(tr, payloadUnpackedPath, unpackedPath, "Lock() payload contains the returned unpackedPath");

    const std::string lockRuntimeConfig = runtimeConfig;
    bool locked = false;
    std::string gatewayMetadataPath;
    L0Test::ExpectEqU32(tr,
                        fx.impl->GetLockedInfo("YouTube", "100.1.24", unpackedPath, runtimeConfig, gatewayMetadataPath, locked),
                        ERROR_NONE,
                        "GetLockedInfo() returns ERROR_NONE for known package");
    L0Test::ExpectTrue(tr, locked, "GetLockedInfo() reports package as locked");
    L0Test::ExpectEqStr(tr,
                        runtimeConfig,
                        lockRuntimeConfig,
                        "GetLockedInfo() returns the opaque runtime payload created by Lock()");

    if (appMetadata != nullptr) {
        appMetadata->Release();
    }

    L0Test::ExpectEqU32(tr,
                        fx.impl->Unlock("YouTube", "100.1.24"),
                        ERROR_NONE,
                        "Unlock() for previously locked package returns ERROR_NONE");

    L0Test::ExpectEqU32(tr,
                        fx.impl->Lock("UnknownApp", "0.0.1", WPEFramework::Exchange::IPackageHandler::LockReason::LAUNCH, lockId, unpackedPath, runtimeConfig, appMetadata),
                        ERROR_INVALID_PARAMETER,
                        "Lock() for unknown package returns ERROR_INVALID_PARAMETER");

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
                        ERROR_INVALID_PARAMETER,
                        "Uninstall() for unknown app returns ERROR_INVALID_PARAMETER");

    std::string cfg;
    L0Test::ExpectEqU32(tr,
                        fx.impl->Config("NoSuchApp", "0", cfg),
                        ERROR_INVALID_PARAMETER,
                        "Config() for unknown package returns ERROR_INVALID_PARAMETER");

    WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
    L0Test::ExpectEqU32(tr,
                        fx.impl->PackageState("NoSuchApp", "0", state),
                        ERROR_INVALID_PARAMETER,
                        "PackageState() for unknown package returns ERROR_INVALID_PARAMETER");

    return tr.failures;
}

uint32_t Test_PM_Impl_GetLockedInfoAndUnlockNegativePaths()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    std::string unpackedPath;
    std::string cfg;
    std::string gatewayMetadataPath;
    bool locked = false;

    L0Test::ExpectEqU32(tr,
                        fx.impl->GetLockedInfo("UnknownApp", "0", unpackedPath, cfg, gatewayMetadataPath, locked),
                        ERROR_INVALID_PARAMETER,
                        "GetLockedInfo() for unknown package returns ERROR_INVALID_PARAMETER");

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
    std::string cfg;

    L0Test::ExpectEqU32(tr,
                        fx.impl->GetConfigForPackage("/tmp/fake.pkg", id, version, cfg),
                        ERROR_NONE,
                        "GetConfigForPackage() with non-empty locator returns ERROR_NONE in UNIT_TEST path");

    WPEFramework::Plugin::Utils::RuntimeConfigPayload payload;
    std::string error;
    bool present = false;
    std::vector<std::string> values;
    L0Test::ExpectTrue(tr, payload.Parse(cfg, error), "GetConfigForPackage() returns a valid JSON runtime payload");
    L0Test::ExpectTrue(tr,
                       payload.GetStringArray("envVariables", values, present, error) && present,
                       "GetConfigForPackage() payload contains envVariables as a JSON string array");
    L0Test::ExpectTrue(tr,
                       payload.GetStringArray("fkpsFiles", values, present, error) && present,
                       "GetConfigForPackage() payload contains fkpsFiles as a JSON string array");
    L0Test::ExpectTrue(tr,
                       payload.GetStringArray("logLevels", values, present, error) && present,
                       "GetConfigForPackage() payload contains logLevels as a JSON string array");

    return tr.failures;
}

uint32_t Test_PM_Impl_InstallDifferentVersionWhileLocked()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    uint32_t lockId = 0;
    std::string unpackedPath;
    std::string runtimeConfig;
    WPEFramework::Exchange::IPackageHandler::ILockIterator* appMetadata = nullptr;

    L0Test::ExpectEqU32(tr,
        fx.impl->Lock("YouTube", "100.1.24", WPEFramework::Exchange::IPackageHandler::LockReason::LAUNCH,
            lockId, unpackedPath, runtimeConfig, appMetadata),
        ERROR_NONE,
        "Lock() existing package before install");

    if (appMetadata != nullptr) {
        appMetadata->Release();
        appMetadata = nullptr;
    }

    WPEFramework::Exchange::IPackageInstaller::FailReason failReason = WPEFramework::Exchange::IPackageInstaller::FailReason::NONE;
    const auto install = fx.impl->Install("YouTube", "200.0.0", nullptr, "/tmp/youtube_200.pkg", failReason);
    L0Test::ExpectEqU32(tr,
        install,
        ERROR_NONE,
        "Install() succeeds while the old version is locked");

    WPEFramework::Exchange::IPackageInstaller::InstallState state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
    L0Test::ExpectEqU32(tr,
        fx.impl->PackageState("YouTube", "200.0.0", state),
        ERROR_NONE,
        "PackageState() for new version is queryable while old version is locked");
    L0Test::ExpectTrue(tr,
        state == WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED,
        "New version is installed while old version remains locked");

    L0Test::ExpectEqU32(tr,
        fx.impl->PackageState("YouTube", "100.1.24", state),
        ERROR_NONE,
        "PackageState() for old version is queryable while its lock is held");
    L0Test::ExpectTrue(tr,
        state == WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED,
        "Old version is marked uninstalled after the new version installs");

    L0Test::ExpectEqU32(tr,
        fx.impl->Unlock("YouTube", "100.1.24"),
        ERROR_NONE,
        "Unlock() releases the old version without processing a deferred install");

    L0Test::ExpectEqU32(tr,
        fx.impl->PackageState("YouTube", "200.0.0", state),
        ERROR_NONE,
        "PackageState() for new version remains queryable after unlock");
    L0Test::ExpectTrue(tr,
        state == WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED,
        "New version remains installed after unlock");

    return tr.failures;
}

uint32_t Test_PM_Impl_UninstallBlockedWhileLockedThenProcessedOnUnlock()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    uint32_t lockId = 0;
    std::string unpackedPath;
    std::string runtimeConfig;
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
uint32_t Test_PM_Impl_GetConfigListForInstalledPackages()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    std::string config;
    L0Test::ExpectEqU32(tr,
        fx.impl->GetConfigListForInstalledPackages("dial", config),
        ERROR_NONE,
        "GetConfigListForInstalledPackages() returns ERROR_NONE for filter 'dial'");
    L0Test::ExpectTrue(tr, !config.empty(), "GetConfigListForInstalledPackages() config is non-empty on success");
    JsonObject dialConfig;
    const bool parsed = dialConfig.FromString(config);
    L0Test::ExpectTrue(tr, parsed, "DIAL package config is valid JSON");
    if (parsed) {
        L0Test::ExpectEqStr(tr, dialConfig["packageId"].String(), std::string("YouTube"),
            "DIAL package config contains the matching package ID");
        L0Test::ExpectEqStr(tr, dialConfig["version"].String(), std::string("100.1.24"),
            "DIAL package config contains the matching package version");
    }

    config.clear();
    L0Test::ExpectEqU32(tr,
        fx.impl->GetConfigListForInstalledPackages("unsupported-filter", config),
        ERROR_GENERAL,
        "GetConfigListForInstalledPackages() returns ERROR_GENERAL for an unmatched filter");
    L0Test::ExpectTrue(tr, config.empty(), "GetConfigListForInstalledPackages() config remains empty on failure");

    return tr.failures;
}
uint32_t Test_PM_Impl_GetConfigForInstalledPackage()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    std::string config;
    L0Test::ExpectEqU32(tr,
        fx.impl->GetConfigForInstalledPackage("SomePackageId", "1.0.0", config),
        ERROR_GENERAL,
        "GetConfigForInstalledPackage() returns ERROR_GENERAL for non-existent package");

    return tr.failures;
}

uint32_t Test_PM_Impl_GetRunningApplicationsUsingPackageSuccess()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    std::string applicationIds;
    L0Test::ExpectEqU32(tr,
        fx.impl->GetRunningApplicationsUsingPackage("YouTube", applicationIds),
        ERROR_NONE,
        "GetRunningApplicationsUsingPackage() returns ERROR_NONE");
    L0Test::ExpectEqStr(tr,
        applicationIds,
        "[\"YouTube\"]",
        "GetRunningApplicationsUsingPackage() serializes the ids as a JSON array string");

    return tr.failures;
}

uint32_t Test_PM_Impl_GetRunningApplicationsUsingPackageBackendFailure()
{
    L0Test::TestResult tr;
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    std::string applicationIds;
    L0Test::ExpectEqU32(tr,
        fx.impl->GetRunningApplicationsUsingPackage("StatusFailApp", applicationIds),
        ERROR_GENERAL,
        "GetRunningApplicationsUsingPackage() maps a backend failure to ERROR_GENERAL");

    return tr.failures;
}

uint32_t Test_PM_Impl_GetRunningApplicationsUsingPackageCacheUnavailable()
{
    L0Test::TestResult tr;

    // No Initialize(): packageImpl stays null until the cache initialization runs,
    // so CHECK_CACHE rejects the call. Deinitialize is skipped deliberately - with
    // no Initialize there is no downloader thread or service to tear down.
    auto* impl = WPEFramework::Core::Service<WPEFramework::Plugin::PackageManagerImplementation>::Create<
        WPEFramework::Plugin::PackageManagerImplementation>();
    std::string applicationIds;
    L0Test::ExpectEqU32(tr,
        impl->GetRunningApplicationsUsingPackage("YouTube", applicationIds),
        WPEFramework::Core::ERROR_UNAVAILABLE,
        "GetRunningApplicationsUsingPackage() before cache initialization returns ERROR_UNAVAILABLE");
    impl->Release();

    return tr.failures;
}

// ---------------------------------------------------------------------------
// RDKEMW-28030: applicationLifecycle payload member and shared helpers
// ---------------------------------------------------------------------------

namespace {

const char* const kLifecycleBlock =
    "{\"supportedNonActiveStates\":[\"paused\",\"suspended\"],"
    "\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10,"
    "\"vendorFuture\":{\"cpuThreshold\":1}}";

struct LifecycleFixtureDir {
    std::string path;

    LifecycleFixtureDir()
    {
        char tmpl[] = "/tmp/pm_l0_lifecycle_XXXXXX";
        const char* created = mkdtemp(tmpl);
        path = (created != nullptr) ? created : "/tmp";
    }

    ~LifecycleFixtureDir()
    {
        if (path != "/tmp") {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    }

    std::string Write(const std::string& name, const std::string& contents) const
    {
        const std::string file = path + "/" + name;
        std::ofstream out(file);
        out << contents;
        return file;
    }

    // Writes a libpackage-style package list that references the given metadata files.
    std::string WritePackageList(const std::string& name, const std::vector<std::string>& metadataPaths) const
    {
        std::string list = "{\"packages\":[";
        for (size_t i = 0; i < metadataPaths.size(); ++i) {
            list += (i == 0 ? "" : ",");
            list += "{\"pkgMountPath\":\"" + path + "/rootfs" + std::to_string(i) + "\",\"pkgMetaDataPath\":\"" + metadataPaths[i] + "\"}";
        }
        list += "]}";
        return Write(name, list);
    }
};

std::string MetadataJson(const std::string& id, const std::string& packageType, const std::string& configurationMembers)
{
    return "{\"id\":\"" + id + "\",\"version\":\"100.1.24\",\"packageType\":\"" + packageType +
        "\",\"configuration\":{" + configurationMembers + "}}";
}

struct RalfPkgPathEnv {
    explicit RalfPkgPathEnv(const std::string& value) { setenv("PACKAGEMANAGER_TEST_RALF_PKG_PATH", value.c_str(), 1); }
    ~RalfPkgPathEnv() { unsetenv("PACKAGEMANAGER_TEST_RALF_PKG_PATH"); }
};

// Locks the dummy package, returns Lock() and GetLockedInfo() payloads, then unlocks it.
void LockDummyPackage(L0Test::TestResult& tr, std::string& lockPayload, std::string& lockedInfoPayload)
{
    ImplFixture fx;
    L0Test::ExpectEqU32(tr, fx.Initialize(), ERROR_NONE, "Initialize() succeeds");

    uint32_t lockId = 0;
    std::string unpackedPath;
    WPEFramework::Exchange::IPackageHandler::ILockIterator* appMetadata = nullptr;
    L0Test::ExpectEqU32(tr,
                        fx.impl->Lock("YouTube", "100.1.24", WPEFramework::Exchange::IPackageHandler::LockReason::LAUNCH,
                                      lockId, unpackedPath, lockPayload, appMetadata),
                        ERROR_NONE,
                        "Lock() returns ERROR_NONE regardless of lifecycle metadata");
    if (appMetadata != nullptr) {
        appMetadata->Release();
    }

    std::string gatewayMetadataPath;
    bool locked = false;
    L0Test::ExpectEqU32(tr,
                        fx.impl->GetLockedInfo("YouTube", "100.1.24", unpackedPath, lockedInfoPayload, gatewayMetadataPath, locked),
                        ERROR_NONE,
                        "GetLockedInfo() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, locked, "GetLockedInfo() reports the package as locked");
    L0Test::ExpectEqU32(tr, fx.impl->Unlock("YouTube", "100.1.24"), ERROR_NONE, "Unlock() returns ERROR_NONE");
}

bool PayloadMembersEqualExcept(const std::string& lhs, const std::string& rhs, const std::vector<std::string>& ignored)
{
    WPEFramework::Core::JSON::VariantContainer a;
    WPEFramework::Core::JSON::VariantContainer b;
    if (!a.FromString(lhs) || !b.FromString(rhs)) {
        return false;
    }
    auto ignoredLabel = [&ignored](const std::string& label) {
        return std::find(ignored.begin(), ignored.end(), label) != ignored.end();
    };
    for (const WPEFramework::Core::JSON::VariantContainer* side : { &a, &b }) {
        const WPEFramework::Core::JSON::VariantContainer& other = (side == &a) ? b : a;
        auto it = side->Variants();
        while (it.Next()) {
            const std::string label = it.Label();
            if (ignoredLabel(label)) {
                continue;
            }
            if (!other.HasLabel(label.c_str())) {
                return false;
            }
            WPEFramework::Core::JSON::VariantContainer mineWrapper;
            WPEFramework::Core::JSON::VariantContainer theirsWrapper;
            mineWrapper[label.c_str()] = it.Current();
            theirsWrapper[label.c_str()] = other[label.c_str()];
            std::string mine;
            std::string theirs;
            mineWrapper.ToString(mine);
            theirsWrapper.ToString(theirs);
            if (mine != theirs) {
                return false;
            }
        }
    }
    return true;
}

bool PayloadHasLifecycle(const std::string& payloadText, std::string& lifecycle)
{
    WPEFramework::Plugin::Utils::RuntimeConfigPayload payload;
    std::string error;
    bool present = false;
    return payload.Parse(payloadText, error) && payload.GetObject("applicationLifecycle", lifecycle, present, error) && present;
}

} // namespace

uint32_t Test_PM_Impl_RuntimeConfigPayloadObjectAccessors()
{
    L0Test::TestResult tr;
    std::string error;
    std::string serialized;
    bool present = true;

    WPEFramework::Plugin::Utils::RuntimeConfigPayload payload;
    L0Test::ExpectTrue(tr, payload.Parse("{\"appPath\":\"/opt/app\"}", error), "Base payload parses");
    L0Test::ExpectTrue(tr, payload.SetObject("applicationLifecycle", kLifecycleBlock, error), "SetObject() accepts a JSON object");
    L0Test::ExpectTrue(tr, payload.Serialize(serialized, error), "Payload with nested object serializes");

    WPEFramework::Plugin::Utils::RuntimeConfigPayload decoded;
    std::string object;
    L0Test::ExpectTrue(tr, decoded.Parse(serialized, error), "Serialized payload parses");
    L0Test::ExpectTrue(tr, decoded.GetObject("applicationLifecycle", object, present, error) && present,
                       "GetObject() returns the nested object");
    WPEFramework::Core::JSON::VariantContainer lifecycle;
    L0Test::ExpectTrue(tr, lifecycle.FromString(object), "Returned object is valid JSON");
    L0Test::ExpectTrue(tr, lifecycle.HasLabel("vendorFuture") && lifecycle["vendorFuture"].Object().HasLabel("cpuThreshold"),
                       "Nested unknown members survive the round trip");
    L0Test::ExpectEqStr(tr, lifecycle["maxSuspendedSystemMemory"].String(), "16M", "String members survive the round trip");
    std::string appPath;
    L0Test::ExpectTrue(tr, decoded.GetString("appPath", appPath, present, error) && present && appPath == "/opt/app",
                       "Other payload members are preserved");

    std::string before;
    L0Test::ExpectTrue(tr, payload.Serialize(before, error), "Payload serializes before invalid SetObject()");
    L0Test::ExpectTrue(tr, !payload.SetObject("applicationLifecycle", "[1,2]", error), "SetObject() rejects a JSON array");
    L0Test::ExpectTrue(tr, !payload.SetObject("applicationLifecycle", "{not json", error), "SetObject() rejects invalid JSON");
    L0Test::ExpectTrue(tr, !payload.SetObject("applicationLifecycle", "", error), "SetObject() rejects empty input");
    std::string after;
    L0Test::ExpectTrue(tr, payload.Serialize(after, error), "Payload serializes after invalid SetObject()");
    L0Test::ExpectEqStr(tr, after, before, "Rejected SetObject() leaves the payload unchanged");

    L0Test::ExpectTrue(tr, decoded.GetObject("missing", object, present, error) && !present,
                       "GetObject() on a missing key reports present=false");
    L0Test::ExpectTrue(tr, !decoded.GetObject("appPath", object, present, error) && present,
                       "GetObject() on a non-object member fails");

    return tr.failures;
}

uint32_t Test_PM_Impl_ParseMemorySize()
{
    L0Test::TestResult tr;
    uint64_t bytes = 0;

    L0Test::ExpectTrue(tr, Utils::ParseMemorySize("16M", bytes) && bytes == 16777216ULL, "\"16M\" is 16777216 bytes");
    L0Test::ExpectTrue(tr, Utils::ParseMemorySize("100B", bytes) && bytes == 100ULL, "\"100B\" is 100 bytes");
    L0Test::ExpectTrue(tr, Utils::ParseMemorySize("1g", bytes) && bytes == 1073741824ULL, "\"1g\" is 1073741824 bytes");
    L0Test::ExpectTrue(tr, Utils::ParseMemorySize("512KB", bytes) && bytes == 524288ULL, "\"512KB\" is 524288 bytes");
    L0Test::ExpectTrue(tr, Utils::ParseMemorySize("2mb", bytes) && bytes == 2097152ULL, "\"2mb\" is 2097152 bytes");
    L0Test::ExpectTrue(tr, Utils::ParseMemorySize("4096", bytes) && bytes == 4096ULL, "Plain number is bytes");

    bytes = 7;
    L0Test::ExpectTrue(tr, !Utils::ParseMemorySize("16X", bytes), "Unknown suffix is rejected");
    L0Test::ExpectTrue(tr, !Utils::ParseMemorySize("", bytes), "Empty input is rejected");
    L0Test::ExpectTrue(tr, !Utils::ParseMemorySize("M", bytes), "Missing digits are rejected");
    L0Test::ExpectTrue(tr, !Utils::ParseMemorySize("16MBx", bytes), "Trailing characters are rejected");
    L0Test::ExpectTrue(tr, !Utils::ParseMemorySize("16BB", bytes), "Double byte suffix is rejected");
    L0Test::ExpectTrue(tr, !Utils::ParseMemorySize("-1M", bytes), "Negative values are rejected");
    L0Test::ExpectTrue(tr, !Utils::ParseMemorySize("99999999999999999999", bytes), "Overflowing numbers are rejected");
    L0Test::ExpectTrue(tr, !Utils::ParseMemorySize("17179869184G", bytes), "Overflowing multiplication is rejected");
    L0Test::ExpectTrue(tr, bytes == 7, "Rejected input leaves the output unchanged");

    return tr.failures;
}

uint32_t Test_PM_Impl_LockAddsApplicationLifecycle()
{
    L0Test::TestResult tr;

    std::string baselinePayload;
    std::string baselineLockedInfo;
    LockDummyPackage(tr, baselinePayload, baselineLockedInfo);
    std::string lifecycle;
    L0Test::ExpectTrue(tr, !PayloadHasLifecycle(baselinePayload, lifecycle),
                       "Without ralfPkgPath the payload has no applicationLifecycle");

    LifecycleFixtureDir dir;
    const std::string runtimeMeta = dir.Write("runtime.json",
        MetadataJson("com.rdk.runtime", "runtime", "\"urn:rdk:config:application-lifecycle\":{\"supportedNonActiveStates\":[]}"));
    const std::string appMeta = dir.Write("app.json",
        MetadataJson("YouTube", "application",
                     std::string("\"urn:rdk:config:memory\":{\"system\":\"256M\"},\"urn:rdk:config:application-lifecycle\":") + kLifecycleBlock));
    const std::string ralfPkgPath = dir.WritePackageList("full_list.json", { runtimeMeta, appMeta });
    RalfPkgPathEnv env(ralfPkgPath);

    std::string lockPayload;
    std::string lockedInfoPayload;
    LockDummyPackage(tr, lockPayload, lockedInfoPayload);

    L0Test::ExpectTrue(tr, PayloadHasLifecycle(lockPayload, lifecycle), "Lock() payload contains applicationLifecycle");
    WPEFramework::Core::JSON::VariantContainer block;
    L0Test::ExpectTrue(tr, block.FromString(lifecycle), "applicationLifecycle is a JSON object");
    L0Test::ExpectEqStr(tr, block["maxSuspendedSystemMemory"].String(), "16M", "maxSuspendedSystemMemory is copied unchanged");
    L0Test::ExpectEqStr(tr, block["maxTimeToSuspendMemoryState"].Value(), "10", "maxTimeToSuspendMemoryState is copied unchanged");
    L0Test::ExpectTrue(tr, block["supportedNonActiveStates"].Array().Length() == 2,
                       "The application's states are used, not the runtime dependency's");
    L0Test::ExpectTrue(tr, block.HasLabel("vendorFuture") && block["vendorFuture"].Object().HasLabel("cpuThreshold"),
                       "Unknown lifecycle members are preserved");

    std::string lockedLifecycle;
    L0Test::ExpectTrue(tr, PayloadHasLifecycle(lockedInfoPayload, lockedLifecycle) && lockedLifecycle == lifecycle,
                       "GetLockedInfo() returns the same applicationLifecycle");
    L0Test::ExpectTrue(tr, PayloadMembersEqualExcept(lockPayload, baselinePayload, { "applicationLifecycle", "ralfPkgPath" }),
                       "Every other payload member is unchanged");

    return tr.failures;
}

uint32_t Test_PM_Impl_LockWithoutApplicationLifecycle()
{
    L0Test::TestResult tr;
    LifecycleFixtureDir dir;

    struct Case {
        const char* name;
        std::string ralfPkgPath;
    };
    const std::vector<Case> cases = {
        { "metadata without the lifecycle block",
          dir.WritePackageList("noblock_list.json", { dir.Write("noblock.json", MetadataJson("YouTube", "application", "\"urn:rdk:config:memory\":{\"system\":\"256M\"}")) }) },
        { "lifecycle block that is not an object",
          dir.WritePackageList("notobject_list.json", { dir.Write("notobject.json", MetadataJson("YouTube", "application", "\"urn:rdk:config:application-lifecycle\":[\"paused\"]")) }) },
        { "malformed metadata file",
          dir.WritePackageList("malformed_list.json", { dir.Write("malformed.json", "{\"id\":\"YouTube\",\"configuration\":") }) },
        { "metadata whose id does not match",
          dir.WritePackageList("other_list.json", { dir.Write("other.json", MetadataJson("OtherApp", "application", std::string("\"urn:rdk:config:application-lifecycle\":") + kLifecycleBlock)) }) },
        { "metadata file that does not exist",
          dir.WritePackageList("missing_meta_list.json", { dir.path + "/does-not-exist.json" }) },
        { "package list that does not exist",
          dir.path + "/missing_metadata.json" },
        { "malformed package list",
          dir.Write("badlist.json", "{\"packages\":{}}") },
    };

    for (const Case& c : cases) {
        RalfPkgPathEnv env(c.ralfPkgPath);
        std::string lockPayload;
        std::string lockedInfoPayload;
        LockDummyPackage(tr, lockPayload, lockedInfoPayload);

        std::string lifecycle;
        L0Test::ExpectTrue(tr, !PayloadHasLifecycle(lockPayload, lifecycle),
                           std::string("Lock() payload omits applicationLifecycle for ") + c.name);
        L0Test::ExpectTrue(tr, !PayloadHasLifecycle(lockedInfoPayload, lifecycle),
                           std::string("GetLockedInfo() payload omits applicationLifecycle for ") + c.name);
        WPEFramework::Plugin::Utils::RuntimeConfigPayload payload;
        std::string error;
        std::string ralfPkgPath;
        bool present = false;
        L0Test::ExpectTrue(tr, payload.Parse(lockPayload, error) && payload.GetString("ralfPkgPath", ralfPkgPath, present, error) &&
                               present && ralfPkgPath == c.ralfPkgPath,
                           std::string("Payload still reports ralfPkgPath for ") + c.name);
    }

    return tr.failures;
}
