/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#include "VictimSelector.h"

const string WPEFramework::Plugin::VictimSelector::SERVICE_NAME = "org.rdk.VictimSelector";

namespace WPEFramework {
namespace Plugin {

namespace {
static Plugin::Metadata<Plugin::VictimSelector> metadata(
    1, 0, 0,
    {},
    {},
    {});
}

SERVICE_REGISTRATION(VictimSelector, 1, 0, 0);

VictimSelector::VictimSelector()
    : mService(nullptr)
    , mImplementation(nullptr)
    , mConfigure(nullptr)
    , mConnectionId(0) {
}

VictimSelector::~VictimSelector() {
}

const string VictimSelector::Initialize(PluginHost::IShell* service) {
    ASSERT(nullptr == mService);
    ASSERT(nullptr == mImplementation);
    ASSERT(nullptr == mConfigure);
    ASSERT(0 == mConnectionId);

    SYSLOG(Logging::Startup, (_T("VictimSelector::Initialize: PID=%u"), getpid()));

    string result;
    if (nullptr == service) {
        LOGERR("VictimSelector initialization failed: service is null");
        result = "VictimSelector received an invalid service";
    } else {
        mService = service;
        mService->AddRef();
        mImplementation = mService->Root<Exchange::IVictimSelector>(mConnectionId, 5000, _T("VictimSelectorImplementation"));
        if (nullptr == mImplementation) {
            LOGERR("VictimSelector initialization failed: implementation could not be created");
            result = "VictimSelector implementation could not be created";
        } else {
            mConfigure = mImplementation->QueryInterface<Exchange::IConfiguration>();
            if (nullptr == mConfigure) {
                LOGERR("VictimSelector initialization failed: implementation has no configuration interface");
                result = "VictimSelector implementation has no configuration interface";
            } else {
                const Core::hresult configureStatus = mConfigure->Configure(mService);
                if (Core::ERROR_NONE != configureStatus) {
                    LOGERR("VictimSelector initialization failed: configuration returned status=%d", configureStatus);
                    result = "VictimSelector could not be configured";
                } else {
                    Exchange::JVictimSelector::Register(*this, mImplementation);
                }
            }
        }

        if (!result.empty()) {
            Deinitialize(service);
        }
    }

    SYSLOG(Logging::Startup, (_T("VictimSelector::Initialize exit: %s"),
        result.empty() ? _T("success") : result.c_str()));
    return result;
}

void VictimSelector::Deinitialize(PluginHost::IShell* service) {
    SYSLOG(Logging::Shutdown, (_T("VictimSelector::Deinitialize entry")));

    if (nullptr != mService) {
        ASSERT(mService == service);
    }
    if (nullptr != mImplementation) {
        Exchange::JVictimSelector::Unregister(*this);
        if (nullptr != mConfigure) {
            mConfigure->Release();
            mConfigure = nullptr;
        }
        RPC::IRemoteConnection* connection = (nullptr != mService)
            ? mService->RemoteConnection(mConnectionId)
            : nullptr;
        mImplementation->Release();
        mImplementation = nullptr;
        if (nullptr != connection) {
            connection->Terminate();
            connection->Release();
        }
    }
    mConnectionId = 0;
    if (nullptr != mService) {
        mService->Release();
        mService = nullptr;
    }

    SYSLOG(Logging::Shutdown, (_T("VictimSelector::Deinitialize exit")));
}

string VictimSelector::Information() const {
    return "Victim Selector plugin";
}

} // namespace Plugin
} // namespace WPEFramework
