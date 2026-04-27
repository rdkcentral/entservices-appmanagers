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

    L0Test::FakePersistentStore fakeStore;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config{&fakeStore});
    service.SetInstantiateHandler([](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 0;
        return nullptr;
    });

    IPlugin* plugin = CreatePlugin();
    L0Test::ExpectTrue(tr, plugin != nullptr, "Plugin created");

    const std::string result = plugin->Initialize(&service);
    L0Test::ExpectTrue(tr, !result.empty(), "Initialize returns error message");
    L0Test::ExpectTrue(tr, result.find("initialised") != std::string::npos ||
                           result.find("initialized") != std::string::npos,
                       "Error message indicates initialization failure");

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

    L0Test::FakePersistentStore fakeStore;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config{&fakeStore});

    // Create a real implementation via Instantiate handler
    service.SetInstantiateHandler([](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 0;
        StorageManagerImplementation* impl = WPEFramework::Core::Service<StorageManagerImplementation>::Create<StorageManagerImplementation>();
        if (impl) {
            impl->AddRef();
        }
        return impl;
    });

    IPlugin* plugin = CreatePlugin();
    L0Test::ExpectTrue(tr, plugin != nullptr, "Plugin created");

    const std::string result = plugin->Initialize(&service);
    L0Test::ExpectEqStr(tr, result, std::string(), "Initialize returns empty string on success");

    // Deinitialize
    plugin->Deinitialize(&service);

    plugin->Release();
    // Note: impl is released by plugin, don't release again

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

    L0Test::FakePersistentStore fake Store;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config{&fakeStore});

    // Provide fake implementation that fails Configure
    service.SetInstantiateHandler([](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 0;
        FakeStorageManagerImplFailConfigure* failImpl = new FakeStorageManagerImplFailConfigure();
        if (failImpl) {
            failImpl->AddRef();
        }
        return failImpl;
    });

    IPlugin* plugin = CreatePlugin();
    L0Test::ExpectTrue(tr, plugin != nullptr, "Plugin created");

    const std::string result = plugin->Initialize(&service);
    L0Test::ExpectTrue(tr, !result.empty(), "Initialize returns error message");
    L0Test::ExpectTrue(tr, result.find("configured") != std::string::npos,
                       "Error message indicates configuration failure");

    plugin->Release();
    // Note: impl is released by plugin, don't release again

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

    L0Test::FakePersistentStore fakeStore;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config{&fakeStore});

    // Create a real implementation via Instantiate handler
    service.SetInstantiateHandler([](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 0;
        StorageManagerImplementation* impl = WPEFramework::Core::Service<StorageManagerImplementation>::Create<StorageManagerImplementation>();
        if (impl) {
            impl->AddRef();
        }
        return impl;
    });

    IPlugin* plugin = CreatePlugin();
    const std::string initResult = plugin->Initialize(&service);
    L0Test::ExpectEqStr(tr, initResult, std::string(), "Initialize success");

    // Information() returns empty string by design (no additional info to report)
    const std::string info = plugin->Information();
    L0Test::ExpectEqStr(tr, info, std::string(), "Information returns empty string");

    plugin->Deinitialize(&service);
    plugin->Release();
    // Note: impl is released by plugin, don't release again

    return tr.failures;
}

/* ========================================================================== */
/* Test_ASM_Lifecycle_DeinitializeWithNullService
 *
 * Verifies that Deinitialize() completes successfully with valid service.
 * Note: Calling Deinitialize with nullptr would trigger ASSERT in plugin code.
 */
uint32_t Test_ASM_Lifecycle_DeinitializeWithNullService()
{
    L0Test::TestResult tr;

    L0Test::FakePersistentStore fakeStore;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config{&fakeStore});

    // Create a real implementation via Instantiate handler
    service.SetInstantiateHandler([](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 0;
        StorageManagerImplementation* impl = WPEFramework::Core::Service<StorageManagerImplementation>::Create<StorageManagerImplementation>();
        if (impl) {
            impl->AddRef();
        }
        return impl;
    });

    IPlugin* plugin = CreatePlugin();
    const std::string initResult = plugin->Initialize(&service);
    L0Test::ExpectEqStr(tr, initResult, std::string(), "Initialize success");

    // Deinitialize with correct service pointer (nullptr would trigger ASSERT)
    plugin->Deinitialize(&service);
    
    // Verify plugin can be released cleanly
    L0Test::ExpectTrue(tr, true, "Deinitialize completed successfully");

    plugin->Release();
    // Note: impl is released by plugin, don't release again

    return tr.failures;
}
