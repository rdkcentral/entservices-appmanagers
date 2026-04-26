/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

/**
 * @file AppStorageManager_LifecycleTests.cpp
 *
 * L0 tests for AppStorageManager plugin lifecycle covering:
 *   - Initialize (success/failure paths)
 *   - Deinitialize
 *   - Information
 */

#include <iostream>
#include <string>

#include "AppStorageManager.h"
#include "AppStorageManagerImplementation.h"
#include "ServiceMock.h"
#include "fakes/FakePersistentStore.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

using WPEFramework::PluginHost::IPlugin;
using WPEFramework::Plugin::AppStorageManager;
using WPEFramework::Plugin::StorageManagerImplementation;

namespace {

/**
 * @brief Create a plugin instance using Thunder Core::Service factory
 */
IPlugin* CreatePlugin()
{
    return WPEFramework::Core::Service<AppStorageManager>::Create<IPlugin>();
}

/**
 * @brief Fake implementation that always fails Configure()
 */
class FakeStorageManagerImplFailConfigure : public WPEFramework::Exchange::IAppStorageManager,
                                            public WPEFramework::Exchange::IConfiguration {
public:
    FakeStorageManagerImplFailConfigure()
        : _refCount(1)
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
        if (id == WPEFramework::Exchange::IAppStorageManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppStorageManager*>(this);
        }
        if (id == WPEFramework::Exchange::IConfiguration::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    // IAppStorageManager methods (stubs)
    WPEFramework::Core::hresult CreateStorage(const string&, const uint32_t&, string&, string&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    WPEFramework::Core::hresult GetStorage(const string&, const int32_t&, const int32_t&, string&, uint32_t&, uint32_t&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    WPEFramework::Core::hresult DeleteStorage(const string&, string&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    WPEFramework::Core::hresult Clear(const string&, string&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    WPEFramework::Core::hresult ClearAll(const string&, string&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // IConfiguration method - always fails
    uint32_t Configure(WPEFramework::PluginHost::IShell*) override
    {
        return WPEFramework::Core::ERROR_GENERAL;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
};

} // namespace

/* ========================================================================== */
/* Test_ASM_Lifecycle_InitializeFailsWhenRootNull
 *
 * Verifies that Initialize() returns an error message when the implementation
 * creation via Root<> fails.
 */
uint32_t Test_ASM_Lifecycle_InitializeFailsWhenRootNull()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock service;
    service.SetRootCreationResult(nullptr); // Force Root<> to return nullptr

    IPlugin* plugin = CreatePlugin();
    L0Test::ExpectTrue(tr, plugin != nullptr, "Plugin created");

    const std::string result = plugin->Initialize(&service);
    L0Test::ExpectTrue(tr, !result.empty(), "Initialize returns error message");
    L0Test::ExpectTrue(tr, result.find("Failed") != std::string::npos ||
                           result.find("failed") != std::string::npos ||
                           result.find("ERROR") != std::string::npos,
                       "Error message indicates failure");

    plugin->Release();
    return tr.failures;
}

/* ========================================================================== */
/* Test_ASM_Lifecycle_InitializeSuccessAndDeinitialize
 *
 * Verifies the successful Initialize() and Deinitialize() lifecycle path.
 */
uint32_t Test_ASM_Lifecycle_InitializeSuccessAndDeinitialize()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock service;
    L0Test::FakePersistentStore* fakeStore = new L0Test::FakePersistentStore();
    service.SetPersistentStoreResult(fakeStore);

    // Create a real implementation
    StorageManagerImplementation* impl = WPEFramework::Core::Service<StorageManagerImplementation>::Create<StorageManagerImplementation>();
    service.SetRootCreationResult(impl);

    IPlugin* plugin = CreatePlugin();
    L0Test::ExpectTrue(tr, plugin != nullptr, "Plugin created");

    const std::string result = plugin->Initialize(&service);
    L0Test::ExpectEqStr(tr, result, std::string(), "Initialize returns empty string on success");

    // Deinitialize
    plugin->Deinitialize(&service);

    plugin->Release();
    fakeStore->Release();
    if (impl) impl->Release();

    return tr.failures;
}

/* ========================================================================== */
/* Test_ASM_Lifecycle_InitializeFailsWhenConfigureFails
 *
 * Verifies that Initialize() returns an error when Configure() fails.
 */
uint32_t Test_ASM_Lifecycle_InitializeFailsWhenConfigureFails()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock service;
    FakeStorageManagerImplFailConfigure* failImpl = new FakeStorageManagerImplFailConfigure();
    service.SetRootCreationResult(failImpl);

    IPlugin* plugin = CreatePlugin();
    L0Test::ExpectTrue(tr, plugin != nullptr, "Plugin created");

    const std::string result = plugin->Initialize(&service);
    L0Test::ExpectTrue(tr, !result.empty(), "Initialize returns error message");

    plugin->Release();
    failImpl->Release();

    return tr.failures;
}

/* ========================================================================== */
/* Test_ASM_Lifecycle_InformationReturnsServiceName
 *
 * Verifies that Information() returns the service name.
 */
uint32_t Test_ASM_Lifecycle_InformationReturnsServiceName()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock service;
    L0Test::FakePersistentStore* fakeStore = new L0Test::FakePersistentStore();
    service.SetPersistentStoreResult(fakeStore);

    StorageManagerImplementation* impl = WPEFramework::Core::Service<StorageManagerImplementation>::Create<StorageManagerImplementation>();
    service.SetRootCreationResult(impl);

    IPlugin* plugin = CreatePlugin();
    const std::string initResult = plugin->Initialize(&service);
    L0Test::ExpectEqStr(tr, initResult, std::string(), "Initialize success");

    const std::string info = plugin->Information();
    L0Test::ExpectTrue(tr, !info.empty(), "Information returns non-empty string");
    L0Test::ExpectTrue(tr, info.find("AppStorageManager") != std::string::npos,
                       "Information contains service name");

    plugin->Deinitialize(&service);
    plugin->Release();
    fakeStore->Release();
    if (impl) impl->Release();

    return tr.failures;
}

/* ========================================================================== */
/* Test_ASM_Lifecycle_DeinitializeWithNullService
 *
 * Verifies that Deinitialize() handles null service pointer gracefully.
 */
uint32_t Test_ASM_Lifecycle_DeinitializeWithNullService()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock service;
    L0Test::FakePersistentStore* fakeStore = new L0Test::FakePersistentStore();
    service.SetPersistentStoreResult(fakeStore);

    StorageManagerImplementation* impl = WPEFramework::Core::Service<StorageManagerImplementation>::Create<StorageManagerImplementation>();
    service.SetRootCreationResult(impl);

    IPlugin* plugin = CreatePlugin();
    const std::string initResult = plugin->Initialize(&service);
    L0Test::ExpectEqStr(tr, initResult, std::string(), "Initialize success");

    // Call Deinitialize with nullptr - should not crash
    plugin->Deinitialize(nullptr);

    plugin->Release();
    fakeStore->Release();
    if (impl) impl->Release();

    return tr.failures;
}
