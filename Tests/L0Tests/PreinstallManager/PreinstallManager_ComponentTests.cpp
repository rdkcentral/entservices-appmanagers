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

/**
 * @file PreinstallManager_ComponentTests.cpp
 *
 * Component (white-box) tests for PreinstallManagerImplementation private methods.
 * Test IDs: L0-IMPL-020 to L0-IMPL-027 (isNewerVersion)
 *            L0-IMPL-030 to L0-IMPL-034 (getFailReason)
 *
 * IMPORTANT: #define private public is intentionally scoped to this translation
 * unit to expose private methods for direct testing.  It does NOT change binary
 * layout (only access-control check) so the behaviour is identical to the
 * production build.
 */

// Expose private members for direct testing in this TU *only*.
// Must appear before the first inclusion of PreinstallManagerImplementation.h.
#define private   public
#define protected public
#include "PreinstallManagerImplementation.h"
#undef protected
#undef private

#include <iostream>
#include <string>

#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Helper: create an implementation object (same as in ImplementationTests).
// ──────────────────────────────────────────────────────────────────────────────
namespace {

WPEFramework::Plugin::PreinstallManagerImplementation* CreateCompImpl()
{
    return WPEFramework::Core::Service<
        WPEFramework::Plugin::PreinstallManagerImplementation>::Create<
        WPEFramework::Plugin::PreinstallManagerImplementation>();
}

} // anonymous namespace

// ══════════════════════════════════════════════════════════════════════════════
//  isNewerVersion() tests   (L0-IMPL-020 to L0-IMPL-027)
// ══════════════════════════════════════════════════════════════════════════════

// L0-IMPL-020 – v1 major > v2 major → true
uint32_t Test_Comp_IsNewerVersion_MajorIncrement()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectTrue(tr, impl->isNewerVersion("2.0.0", "1.0.0"),
                       "L0-IMPL-020: 2.0.0 newer than 1.0.0 → true");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-021 – v1 major < v2 major → false
uint32_t Test_Comp_IsNewerVersion_OlderMajor()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectTrue(tr, !impl->isNewerVersion("1.0.0", "2.0.0"),
                       "L0-IMPL-021: 1.0.0 NOT newer than 2.0.0 → false");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-022 – exactly equal versions → false
uint32_t Test_Comp_IsNewerVersion_EqualVersions()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectTrue(tr, !impl->isNewerVersion("1.2.3", "1.2.3"),
                       "L0-IMPL-022: 1.2.3 == 1.2.3 → not newer → false");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-023 – v1 minor > v2 minor (same major) → true
uint32_t Test_Comp_IsNewerVersion_MinorIncrement()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectTrue(tr, impl->isNewerVersion("1.1.0", "1.0.0"),
                       "L0-IMPL-023: 1.1.0 newer than 1.0.0 → true");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-024 – v1 patch > v2 patch (same major.minor) → true
uint32_t Test_Comp_IsNewerVersion_PatchIncrement()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectTrue(tr, impl->isNewerVersion("1.0.1", "1.0.0"),
                       "L0-IMPL-024: 1.0.1 newer than 1.0.0 → true");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-025 – suffix stripping: "1.0.0-beta" vs "1.0.0" → both strip to "1.0.0" → false
uint32_t Test_Comp_IsNewerVersion_SuffixStrippedEqual()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    // After stripping '-beta', both become "1.0.0" → equal → not newer
    L0Test::ExpectTrue(tr, !impl->isNewerVersion("1.0.0-beta", "1.0.0"),
                       "L0-IMPL-025: 1.0.0-beta strips to 1.0.0, same as 1.0.0 → false");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-026 – 4-part version: build component (v2.0.0.1 > v2.0.0.0) → true
uint32_t Test_Comp_IsNewerVersion_FourPartBuildIncrement()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectTrue(tr, impl->isNewerVersion("2.0.0.1", "2.0.0.0"),
                       "L0-IMPL-026: 2.0.0.1 newer than 2.0.0.0 → true");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-027 – invalid format (< 3 parts after strip): LOGERR + returns false
uint32_t Test_Comp_IsNewerVersion_InvalidFormatReturnsFalse()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    // sscanf with "%d.%d.%d.%d" requires at least 3 parts; "1.0" has only 2
    L0Test::ExpectTrue(tr, !impl->isNewerVersion("1.0", "1.0.0"),
                       "L0-IMPL-027: '1.0' (< 3 parts) is invalid → returns false");
    L0Test::ExpectTrue(tr, !impl->isNewerVersion("1.0.0", "1.0"),
                       "L0-IMPL-027: '1.0' (< 3 parts) in v2 is invalid → returns false");
    L0Test::ExpectTrue(tr, !impl->isNewerVersion("", "1.0.0"),
                       "L0-IMPL-027: empty v1 is invalid → returns false");
    L0Test::ExpectTrue(tr, !impl->isNewerVersion("1.0.0", ""),
                       "L0-IMPL-027: empty v2 is invalid → returns false");
    impl->Release();
    return tr.failures;
}


// ══════════════════════════════════════════════════════════════════════════════
//  getFailReason() tests   (L0-IMPL-030 to L0-IMPL-034)
// ══════════════════════════════════════════════════════════════════════════════

using FailReason = WPEFramework::Exchange::IPackageInstaller::FailReason;

// L0-IMPL-030 – SIGNATURE_VERIFICATION_FAILURE
uint32_t Test_Comp_GetFailReason_SignatureVerification()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectEqStr(
        tr,
        impl->getFailReason(FailReason::SIGNATURE_VERIFICATION_FAILURE),
        std::string("SIGNATURE_VERIFICATION_FAILURE"),
        "L0-IMPL-030: SIGNATURE_VERIFICATION_FAILURE maps to correct string");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-031 – PACKAGE_MISMATCH_FAILURE
uint32_t Test_Comp_GetFailReason_PackageMismatch()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectEqStr(
        tr,
        impl->getFailReason(FailReason::PACKAGE_MISMATCH_FAILURE),
        std::string("PACKAGE_MISMATCH_FAILURE"),
        "L0-IMPL-031: PACKAGE_MISMATCH_FAILURE maps to correct string");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-032 – INVALID_METADATA_FAILURE
uint32_t Test_Comp_GetFailReason_InvalidMetadata()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectEqStr(
        tr,
        impl->getFailReason(FailReason::INVALID_METADATA_FAILURE),
        std::string("INVALID_METADATA_FAILURE"),
        "L0-IMPL-032: INVALID_METADATA_FAILURE maps to correct string");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-033 – PERSISTENCE_FAILURE
uint32_t Test_Comp_GetFailReason_PersistenceFailure()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectEqStr(
        tr,
        impl->getFailReason(FailReason::PERSISTENCE_FAILURE),
        std::string("PERSISTENCE_FAILURE"),
        "L0-IMPL-033: PERSISTENCE_FAILURE maps to correct string");
    impl->Release();
    return tr.failures;
}

// L0-IMPL-034 – NONE (and default case) → "NONE"
uint32_t Test_Comp_GetFailReason_None()
{
    L0Test::TestResult tr;

    auto* impl = CreateCompImpl();
    L0Test::ExpectEqStr(
        tr,
        impl->getFailReason(FailReason::NONE),
        std::string("NONE"),
        "L0-IMPL-034: NONE (default case) maps to 'NONE'");
    impl->Release();
    return tr.failures;
}
