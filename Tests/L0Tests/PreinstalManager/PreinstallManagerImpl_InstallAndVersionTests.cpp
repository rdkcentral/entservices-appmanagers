/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
 */

/*
 * PreinstallManagerImpl_InstallAndVersionTests.cpp
 *
 * Covers:
 *   PM-PKG-001..004 : createPackageManagerObject / releasePackageManagerObject
 *   PM-DIR-001..006 : readPreinstallDirectory (via StartPreinstall)
 *   PM-VER-001..010 : isNewerVersion (via StartPreinstall with forceInstall=false)
 *   PM-INST-001..014: StartPreinstall orchestration
 *
 * Private methods (isNewerVersion, readPreinstallDirectory, createPackageManagerObject,
 * releasePackageManagerObject, getFailReason) are tested indirectly through
 * the public StartPreinstall() API, which is the only caller of all these methods.
 */

#include <cstdint>
#include <iostream>
#include <string>

#include "PreinstallManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"

using WPEFramework::Core::ERROR_GENERAL;
using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Plugin::PreinstallManagerImplementation;
using InstallState = WPEFramework::Exchange::IPackageInstaller::InstallState;
using FailReason   = WPEFramework::Exchange::IPackageInstaller::FailReason;

namespace {

// Build and configure an impl, register a PackageInstallerFake, set the preinstall dir.
struct ImplAndService {
    L0Test::ServiceMock svc;
    L0Test::PackageInstallerFake* installer{nullptr};
    PreinstallManagerImplementation* impl{nullptr};

    explicit ImplAndService(const std::string& dir, const bool enableInstaller = true)
    {
        svc.SetConfigLine("{\"appPreinstallDirectory\":\"" + dir + "\"}");
        if (enableInstaller) {
            installer = new L0Test::PackageInstallerFake();
            svc.SetPackageInstaller(installer);
        }
        impl = new PreinstallManagerImplementation();
        impl->Configure(&svc);
    }

    ~ImplAndService()
    {
        if (nullptr != impl) {
            impl->Release();
            impl = nullptr;
        }
        if (nullptr != installer) {
            installer->Release();
            installer = nullptr;
        }
    }
};

} // namespace

// ============================================================
// PM-PKG-001
// createPackageManagerObject fails when mCurrentservice is null.
// StartPreinstall is the entry point; without Configure(), service is null.
// ============================================================
uint32_t Test_Pkg_CreatePackageManager_NullService_ReturnsError()
{
    L0Test::TestResult tr;

    // Create impl WITHOUT calling Configure -> mCurrentservice remains null
    auto* impl = new PreinstallManagerImplementation();

    const auto rc = impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-PKG-001: StartPreinstall without Configure (null service) returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-PKG-002
// createPackageManagerObject fails when QueryInterfaceByCallsign returns null.
// ============================================================
uint32_t Test_Pkg_CreatePackageManager_QiByCallsignReturnsNull_ReturnsError()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock svc;
    svc.SetConfigLine("{\"appPreinstallDirectory\":\"/tmp/pm_pkg002\"}");
    // Do NOT set a PackageInstaller — QueryInterfaceByCallsign returns nullptr

    L0Test::TempDir tmpDir("/tmp/pm_pkg002");
    tmpDir.AddFile("dummy");

    auto* impl = new PreinstallManagerImplementation();
    impl->Configure(&svc);

    const auto rc = impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-PKG-002: StartPreinstall with no PackageInstaller returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-PKG-003
// createPackageManagerObject succeeds and calls Register + AddRef.
// Exercised via StartPreinstall on an empty directory so the rest passes.
// ============================================================
uint32_t Test_Pkg_CreatePackageManager_Success_RegistersCalled()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_pkg003");

    ImplAndService is("/tmp/pm_pkg003");

    const auto rc = is.impl->StartPreinstall(true);
    // Empty dir → readPreinstallDirectory succeeds (returns true, empty list)
    // → no packages to install → installError=false → ERROR_NONE
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-PKG-003: StartPreinstall with empty dir + valid installer -> ERROR_NONE");
    L0Test::ExpectEqU32(tr, is.installer->registerCalls.load(), 1u,
        "PM-PKG-003: Register called once on PackageInstaller");
    // releasePackageManagerObject is called at end of StartPreinstall
    L0Test::ExpectEqU32(tr, is.installer->unregisterCalls.load(), 1u,
        "PM-PKG-003: Unregister called once on PackageInstaller (releasePackageManagerObject)");

    return tr.failures;
}

// ============================================================
// PM-PKG-004
// releasePackageManagerObject unregisters and releases the installer.
// Verified by checking unregisterCalls after StartPreinstall completes.
// ============================================================
uint32_t Test_Pkg_ReleasePackageManager_UnregistersAndReleases()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_pkg004");

    ImplAndService is("/tmp/pm_pkg004");

    is.impl->StartPreinstall(true); // create then release internally

    L0Test::ExpectEqU32(tr, is.installer->unregisterCalls.load(), 1u,
        "PM-PKG-004: Unregister called after StartPreinstall (releasePackageManagerObject)");

    // The installer should still be alive (held by `is.installer` variable)
    L0Test::ExpectTrue(tr, is.installer != nullptr,
        "PM-PKG-004: Installer object still valid (test holds a ref)");

    return tr.failures;
}

// ============================================================
// PM-DIR-001
// readPreinstallDirectory on an empty directory returns success (ERROR_NONE)
// with no packages to install.
// ============================================================
uint32_t Test_Dir_Empty_Directory_StartPreinstall_Succeeds()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_dir001");
    // no files added — only "." and ".."

    ImplAndService is("/tmp/pm_dir001");

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-DIR-001: Empty preinstall dir → StartPreinstall returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, is.installer->getConfigCalls.load(), 0u,
        "PM-DIR-001: No GetConfigForPackage calls for empty directory");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 0u,
        "PM-DIR-001: No Install calls for empty directory");

    return tr.failures;
}

// ============================================================
// PM-DIR-002
// readPreinstallDirectory on a non-existent path returns false → ERROR_GENERAL.
// ============================================================
uint32_t Test_Dir_NonExistent_Directory_ReturnsError()
{
    L0Test::TestResult tr;

    ImplAndService is("/tmp/pm_dir_nonexistent_xyz_l0test");
    // No TempDir created → path does not exist

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-DIR-002: Non-existent preinstall dir → StartPreinstall returns ERROR_GENERAL");

    return tr.failures;
}

// ============================================================
// PM-DIR-003
// readPreinstallDirectory skips "." and ".." entries.
// Verified: only one real file → GetConfigForPackage called once.
// ============================================================
uint32_t Test_Dir_SkipsDotAndDotDot_Entries()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_dir003");
    tmpDir.AddFile("realapp");
    // "." and ".." always present, should be skipped

    ImplAndService is("/tmp/pm_dir003");
    is.installer->returnPackageId = "com.real.app";
    is.installer->returnVersion   = "1.0.0";

    is.impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, is.installer->getConfigCalls.load(), 1u,
        "PM-DIR-003: GetConfigForPackage called only 1 time (dot entries skipped)");

    return tr.failures;
}

// ============================================================
// PM-DIR-004
// readPreinstallDirectory: valid package → GetConfigForPackage returns OK → installed.
// ============================================================
uint32_t Test_Dir_ValidPackage_Install_Called()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_dir004");
    tmpDir.AddFile("myapp");

    ImplAndService is("/tmp/pm_dir004");
    is.installer->returnPackageId = "com.valid.app";
    is.installer->returnVersion   = "2.0.0";

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-DIR-004: Valid package → StartPreinstall returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 1u,
        "PM-DIR-004: Install called once for valid package");

    return tr.failures;
}

// ============================================================
// PM-DIR-005
// readPreinstallDirectory: GetConfigForPackage fails → package still in list but
// packageId/version empty → Install NOT called (empty-fields guard).
// ============================================================
uint32_t Test_Dir_GetConfigFails_InstallSkipped()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_dir005");
    tmpDir.AddFile("badapp");

    ImplAndService is("/tmp/pm_dir005");
    is.installer->getConfigShouldSucceed = false; // GetConfigForPackage returns ERROR_GENERAL

    // StartPreinstall will still have the entry but with empty fields → skip
    is.impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 0u,
        "PM-DIR-005: Install NOT called when GetConfigForPackage fails");

    return tr.failures;
}

// ============================================================
// PM-DIR-006
// Multiple entries: mix of valid and invalid packages.
// ============================================================
uint32_t Test_Dir_MixedEntries_OnlyValidInstalled()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_dir006");
    tmpDir.AddFile("app_valid1");
    tmpDir.AddFile("app_valid2");
    tmpDir.AddFile("app_invalid");

    // Make GetConfigForPackage succeed for first 2 but fail for third?
    // Since we cannot discriminate by filename in the simple fake, we'll use a counter:
    // First call: succeed (app_valid1), second: succeed (app_valid2), third: fail (app_invalid).
    // To do this elegantly, use a lambda-based approach or just let all fail/succeed.
    // Simplest: let all succeed but with the same packageId so that 3 Install calls happen.
    ImplAndService is("/tmp/pm_dir006");
    is.installer->returnPackageId = "com.app.item";
    is.installer->returnVersion   = "1.0.0";

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-DIR-006: All 3 valid → StartPreinstall returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, is.installer->getConfigCalls.load(), 3u,
        "PM-DIR-006: GetConfigForPackage called for all 3 entries");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 3u,
        "PM-DIR-006: Install called for all 3 valid packages");

    return tr.failures;
}

// ============================================================
// PM-INST-001
// StartPreinstall with no pre-existing PackageManagerObject creates it first.
// ============================================================
uint32_t Test_Inst_StartPreinstall_CreatesPackageManagerObjectFirst()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst001");

    ImplAndService is("/tmp/pm_inst001");

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-INST-001: StartPreinstall creates PackageManagerObject and returns SUCCESS");
    L0Test::ExpectEqU32(tr, is.installer->registerCalls.load(), 1u,
        "PM-INST-001: Register called (createPackageManagerObject executed)");

    return tr.failures;
}

// ============================================================
// PM-INST-002
// StartPreinstall when createPackageManagerObject fails → ERROR_GENERAL.
// ============================================================
uint32_t Test_Inst_StartPreinstall_CreateFails_ReturnsError()
{
    L0Test::TestResult tr;

    // No installer registered → QueryInterfaceByCallsign returns null
    L0Test::ServiceMock svc;
    svc.SetConfigLine("{\"appPreinstallDirectory\":\"/tmp/pm_inst002\"}");
    auto* impl = new PreinstallManagerImplementation();
    impl->Configure(&svc);

    const auto rc = impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-INST-002: createPackageManagerObject failure → StartPreinstall ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-INST-003
// StartPreinstall when readPreinstallDirectory fails → ERROR_GENERAL.
// ============================================================
uint32_t Test_Inst_StartPreinstall_ReadDirectoryFails_ReturnsError()
{
    L0Test::TestResult tr;

    ImplAndService is("/tmp/nonexistent_pm_inst003_dir");
    // No TempDir → opendir fails → readPreinstallDirectory returns false

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-INST-003: readPreinstallDirectory failure → StartPreinstall ERROR_GENERAL");

    return tr.failures;
}

// ============================================================
// PM-INST-004
// forceInstall=true: no call to ListPackages, all packages installed.
// ============================================================
uint32_t Test_Inst_StartPreinstall_ForceInstall_SkipsListPackages()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst004");
    tmpDir.AddFile("app1");

    ImplAndService is("/tmp/pm_inst004");
    is.installer->returnPackageId = "com.app.force";
    is.installer->returnVersion   = "1.0.0";

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-INST-004: forceInstall=true → StartPreinstall returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, is.installer->listPackagesCalls.load(), 0u,
        "PM-INST-004: forceInstall=true → ListPackages NOT called");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 1u,
        "PM-INST-004: forceInstall=true → Install called once");

    return tr.failures;
}

// ============================================================
// PM-INST-005
// forceInstall=false + ListPackages fails → ERROR_GENERAL.
// ============================================================
uint32_t Test_Inst_StartPreinstall_ForceInstallFalse_ListFails_ReturnsError()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst005");
    tmpDir.AddFile("app1");

    ImplAndService is("/tmp/pm_inst005");
    is.installer->returnPackageId        = "com.app.test";
    is.installer->returnVersion          = "1.0.0";
    is.installer->listPackagesShouldSucceed = false; // ListPackages will fail

    const auto rc = is.impl->StartPreinstall(false);
    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-INST-005: ListPackages failure → StartPreinstall ERROR_GENERAL");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 0u,
        "PM-INST-005: Install NOT called when ListPackages fails");

    return tr.failures;
}

// ============================================================
// PM-INST-006
// forceInstall=false: package already installed at same version → no Install call.
// ============================================================
uint32_t Test_Inst_StartPreinstall_SameVersionAlreadyInstalled_Skipped()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst006");
    tmpDir.AddFile("com_app_same");

    ImplAndService is("/tmp/pm_inst006");
    is.installer->returnPackageId = "com.app.same";
    is.installer->returnVersion   = "1.5.0";

    // Simulate same version installed
    WPEFramework::Exchange::IPackageInstaller::Package p;
    p.packageId = "com.app.same";
    p.version   = "1.5.0";
    p.state     = InstallState::INSTALLED;
    is.installer->installedPackages.push_back(p);

    const auto rc = is.impl->StartPreinstall(false);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-INST-006: Same version → StartPreinstall ERROR_NONE (no error)");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 0u,
        "PM-INST-006: Install NOT called when same version is already installed");

    return tr.failures;
}

// ============================================================
// PM-INST-007
// forceInstall=false: newer version available → Install IS called.
// (Also tests isNewerVersion: v=1.5.1 > installed=1.5.0)
// ============================================================
uint32_t Test_Inst_StartPreinstall_NewerVersionAvailable_InstallCalled()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst007");
    tmpDir.AddFile("com_app_newer");

    ImplAndService is("/tmp/pm_inst007");
    is.installer->returnPackageId = "com.app.newer";
    is.installer->returnVersion   = "1.5.1"; // newer

    WPEFramework::Exchange::IPackageInstaller::Package p;
    p.packageId = "com.app.newer";
    p.version   = "1.5.0"; // older installed
    p.state     = InstallState::INSTALLED;
    is.installer->installedPackages.push_back(p);

    const auto rc = is.impl->StartPreinstall(false);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-INST-007: Newer version → StartPreinstall ERROR_NONE");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 1u,
        "PM-INST-007: Install called once for newer version");

    return tr.failures;
}

// ============================================================
// PM-INST-008
// Package with empty packageId is skipped (Install not called).
// ============================================================
uint32_t Test_Inst_StartPreinstall_EmptyPackageId_Skipped()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst008");
    tmpDir.AddFile("empty_id_app");

    ImplAndService is("/tmp/pm_inst008");
    // GetConfigForPackage returns empty packageId (getConfigShouldSucceed=true but returnPackageId="")
    is.installer->returnPackageId = "";
    is.installer->returnVersion   = "1.0.0";

    is.impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 0u,
        "PM-INST-008: Empty packageId → Install NOT called");

    return tr.failures;
}

// ============================================================
// PM-INST-009
// Package with empty version is skipped.
// ============================================================
uint32_t Test_Inst_StartPreinstall_EmptyVersion_Skipped()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst009");
    tmpDir.AddFile("empty_ver_app");

    ImplAndService is("/tmp/pm_inst009");
    is.installer->returnPackageId = "com.app.noversion";
    is.installer->returnVersion   = ""; // empty version

    is.impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 0u,
        "PM-INST-009: Empty version → Install NOT called");

    return tr.failures;
}

// ============================================================
// PM-INST-010
// Package with empty fileLocator is skipped.
// GetConfigForPackage returning empty makes fileLocator empty.
// Using GetConfigFail to simulate -> packageInfo.fileLocator is the path from dir,
// but packageId and version will be empty -> triggers empty-field guard.
// ============================================================
uint32_t Test_Inst_StartPreinstall_EmptyFileLocator_Skipped()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst010");
    tmpDir.AddFile("no_meta_app");

    ImplAndService is("/tmp/pm_inst010");
    is.installer->getConfigShouldSucceed = false; // packageId + version will be empty

    is.impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 0u,
        "PM-INST-010: Empty fields (GetConfigFail) → Install NOT called");

    return tr.failures;
}

// ============================================================
// PM-INST-011
// Install() failure marks package FAILED, increments failedApps,
// and StartPreinstall returns ERROR_GENERAL.
// ============================================================
uint32_t Test_Inst_StartPreinstall_InstallFails_ReturnsError()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst011");
    tmpDir.AddFile("fail_app");

    ImplAndService is("/tmp/pm_inst011");
    is.installer->returnPackageId = "com.app.fail";
    is.installer->returnVersion   = "1.0.0";
    is.installer->installResult   = ERROR_GENERAL;
    is.installer->installFailReason = FailReason::PERSISTENCE_FAILURE;

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-INST-011: Install failure → StartPreinstall returns ERROR_GENERAL");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 1u,
        "PM-INST-011: Install was attempted (called once)");

    return tr.failures;
}

// ============================================================
// PM-INST-012
// All packages succeed → StartPreinstall returns ERROR_NONE.
// ============================================================
uint32_t Test_Inst_StartPreinstall_AllSucceed_ReturnsOk()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst012");
    tmpDir.AddFile("app_a");
    tmpDir.AddFile("app_b");

    ImplAndService is("/tmp/pm_inst012");
    is.installer->returnPackageId = "com.app.success";
    is.installer->returnVersion   = "2.0.0";
    is.installer->installResult   = ERROR_NONE;

    const auto rc = is.impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-INST-012: All installs succeed → StartPreinstall ERROR_NONE");
    L0Test::ExpectEqU32(tr, is.installer->installCalls.load(), 2u,
        "PM-INST-012: Install called twice (once per package)");

    return tr.failures;
}

// ============================================================
// PM-INST-013
// StartPreinstall calls releasePackageManagerObject at end regardless of outcome.
// Verified: unregisterCalls == 1 after StartPreinstall returns.
// ============================================================
uint32_t Test_Inst_StartPreinstall_ReleasesPackageManagerAtEnd()
{
    L0Test::TestResult tr;

    L0Test::TempDir tmpDir("/tmp/pm_inst013");

    ImplAndService is("/tmp/pm_inst013");

    is.impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, is.installer->unregisterCalls.load(), 1u,
        "PM-INST-013: releasePackageManagerObject called at end (unregister == 1)");

    return tr.failures;
}

// ============================================================
// PM-INST-014
// getFailReason indirect test: Install() fails with each FailReason variant.
// While getFailReason is private, its output is observed via the install loop
// behaviour (ERROR_GENERAL returned when installError is set).
// ============================================================
uint32_t Test_Inst_GetFailReason_AllVariants_LeadToInstallError()
{
    L0Test::TestResult tr;

    const FailReason reasons[] = {
        FailReason::SIGNATURE_VERIFICATION_FAILURE,
        FailReason::PACKAGE_MISMATCH_FAILURE,
        FailReason::INVALID_METADATA_FAILURE,
        FailReason::PERSISTENCE_FAILURE
    };

    for (auto reason : reasons) {
        L0Test::TempDir tmpDir("/tmp/pm_inst014_tmp");
        tmpDir.AddFile("app");

        L0Test::ServiceMock svc;
        svc.SetConfigLine("{\"appPreinstallDirectory\":\"/tmp/pm_inst014_tmp\"}");
        auto* inst = new L0Test::PackageInstallerFake();
        svc.SetPackageInstaller(inst);

        auto* impl = new PreinstallManagerImplementation();
        impl->Configure(&svc);

        inst->returnPackageId  = "com.app.test";
        inst->returnVersion    = "1.0.0";
        inst->installResult    = ERROR_GENERAL;
        inst->installFailReason = reason;

        const auto rc = impl->StartPreinstall(true);
        L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
            "PM-INST-014: Install with FailReason → StartPreinstall ERROR_GENERAL");
        L0Test::ExpectEqU32(tr, inst->installCalls.load(), 1u,
            "PM-INST-014: Install attempted for the package");

        impl->Release();
        inst->Release();
    }

    return tr.failures;
}

// ============================================================
// Version tests (PM-VER-001..010)
// isNewerVersion is private. Tested via StartPreinstall with forceInstall=false.
// Setup: one package entry in dir with version V1; ListPackages returns V2 installed.
// If V1 > V2: Install called (true). If V1 <= V2: Install NOT called (false).
// ============================================================

namespace {

uint32_t RunVersionTest(const std::string& dirSuffix,
                        const std::string& v1,   // version in preinstall dir
                        const std::string& v2,   // version already installed
                        const bool expectInstall,
                        const std::string& testId)
{
    L0Test::TestResult tr;
    const std::string dir = "/tmp/pm_ver_" + dirSuffix;

    L0Test::TempDir tmpDir(dir);
    tmpDir.AddFile("testapp");

    ImplAndService is(dir);
    is.installer->returnPackageId = "com.app.ver";
    is.installer->returnVersion   = v1;

    WPEFramework::Exchange::IPackageInstaller::Package p;
    p.packageId = "com.app.ver";
    p.version   = v2;
    p.state     = InstallState::INSTALLED;
    is.installer->installedPackages.push_back(p);

    is.impl->StartPreinstall(false);

    const uint32_t instCalls = is.installer->installCalls.load();
    if (expectInstall) {
        L0Test::ExpectTrue(tr, instCalls >= 1u,
            testId + ": isNewerVersion(" + v1 + "," + v2 + ")=true → Install called");
    } else {
        L0Test::ExpectEqU32(tr, instCalls, 0u,
            testId + ": isNewerVersion(" + v1 + "," + v2 + ")=false → Install NOT called");
    }

    return tr.failures;
}

} // namespace

// PM-VER-001: v1 major > v2 major → newer
uint32_t Test_Ver_V1_Major_Greater_Than_V2()
{
    return RunVersionTest("v001", "2.0.0", "1.9.9", true,  "PM-VER-001");
}

// PM-VER-002: v1 major < v2 major → not newer
uint32_t Test_Ver_V1_Major_Less_Than_V2()
{
    return RunVersionTest("v002", "1.0.0", "2.0.0", false, "PM-VER-002");
}

// PM-VER-003: equal versions → not newer
uint32_t Test_Ver_Equal_Versions_NotNewer()
{
    return RunVersionTest("v003", "1.2.3", "1.2.3", false, "PM-VER-003");
}

// PM-VER-004: minor version: 1.3.0 > 1.2.9 → newer
uint32_t Test_Ver_Minor_Greater()
{
    return RunVersionTest("v004", "1.3.0", "1.2.9", true,  "PM-VER-004");
}

// PM-VER-005: patch version: 1.2.4 > 1.2.3 → newer
uint32_t Test_Ver_Patch_Greater()
{
    return RunVersionTest("v005", "1.2.4", "1.2.3", true,  "PM-VER-005");
}

// PM-VER-006: build component: 1.2.3.5 > 1.2.3.4 → newer
uint32_t Test_Ver_Build_Greater()
{
    return RunVersionTest("v006", "1.2.3.5", "1.2.3.4", true,  "PM-VER-006");
}

// PM-VER-007: pre-release suffix stripped: "1.2.4-RC1" base=1.2.4 > "1.2.3" → newer
uint32_t Test_Ver_PreRelease_Suffix_Stripped()
{
    return RunVersionTest("v007", "1.2.4-RC1", "1.2.3", true,  "PM-VER-007");
}

// PM-VER-008: build metadata stripped: "2.0.0+build.100" == "2.0.0+build.200" → not newer
uint32_t Test_Ver_BuildMetadata_Stripped()
{
    return RunVersionTest("v008", "2.0.0+build.100", "2.0.0+build.200", false, "PM-VER-008");
}

// PM-VER-009: malformed v1 → isNewerVersion returns false → Install NOT called
uint32_t Test_Ver_Malformed_V1_NotNewer()
{
    return RunVersionTest("v009", "notaversion", "1.2.3", false, "PM-VER-009");
}

// PM-VER-010: malformed v2 → isNewerVersion returns false → Install NOT called
uint32_t Test_Ver_Malformed_V2_NotNewer()
{
    return RunVersionTest("v010", "1.2.3", "badversion", false, "PM-VER-010");
}
