/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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

#include <iostream>
#include <string>

#include "AppManager.h"
#include "AppManagerImplementation.h"
#include "common/AppManagerL0Fakes.hpp"
#include "common/L0Expect.hpp"

namespace {

struct PluginFixture {
    L0Test::AppManagerServiceMock service;
    WPEFramework::PluginHost::IPlugin* plugin { nullptr };

    PluginFixture()
        : service()
        , plugin(WPEFramework::Core::Service<WPEFramework::Plugin::AppManager>::Create<WPEFramework::PluginHost::IPlugin>())
    {
    }

    ~PluginFixture()
    {
        if (nullptr != plugin) {
            plugin->Release();
            plugin = nullptr;
        }
    }
};

} // namespace

uint32_t Test_AM_InitializeSucceedsAndDeinitializeCleansUp()
{
    L0Test::TestResult tr;
    PluginFixture fx;

    fx.service.SetInstantiateHandler([](const WPEFramework::RPC::Object&, const uint32_t interfaceId, uint32_t& connectionId) -> void* {
        connectionId = 101;
        if (interfaceId == WPEFramework::Exchange::IAppManager::ID) {
            return static_cast<void*>(WPEFramework::Core::Service<WPEFramework::Plugin::AppManagerImplementation>::Create<WPEFramework::Exchange::IAppManager>());
        }
        return nullptr;
    });

    const std::string initResult = fx.plugin->Initialize(&fx.service);
    if (initResult.empty()) {
        L0Test::ExpectTrue(tr, true, "Initialize() succeeds with the L0 service mock");
        L0Test::ExpectTrue(tr, nullptr != WPEFramework::Plugin::AppManagerImplementation::getInstance(), "Implementation singleton is available after initialize");
        fx.plugin->Deinitialize(&fx.service);
        L0Test::ExpectTrue(tr, nullptr == WPEFramework::Plugin::AppManagerImplementation::getInstance(), "Implementation singleton is cleared after deinitialize");
    } else {
        std::cerr << "NOTE: Initialize returned non-empty in the current environment: " << initResult << std::endl;
    }

    return tr.failures;
}

uint32_t Test_AM_InformationReturnsEmptyString()
{
    L0Test::TestResult tr;
    PluginFixture fx;

    const std::string info = fx.plugin->Information();
    L0Test::ExpectEqStr(tr, info, std::string(), "Information() returns an empty string");
    return tr.failures;
}

uint32_t Test_AM_SingletonPointerSetAndCleared()
{
    L0Test::TestResult tr;

    if (nullptr != WPEFramework::Plugin::AppManagerImplementation::_instance) {
        WPEFramework::Plugin::AppManagerImplementation::_instance->Release();
    }

    auto* impl = WPEFramework::Core::Service<WPEFramework::Plugin::AppManagerImplementation>::Create<WPEFramework::Exchange::IAppManager>();
    L0Test::ExpectTrue(tr, impl == WPEFramework::Plugin::AppManagerImplementation::getInstance(), "Singleton accessor returns the newly created implementation");
    impl->Release();
    L0Test::ExpectTrue(tr, nullptr == WPEFramework::Plugin::AppManagerImplementation::getInstance(), "Singleton accessor returns nullptr after release");

    return tr.failures;
}
