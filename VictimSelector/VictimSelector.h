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

#pragma once

#include "Module.h"
#include <interfaces/IVictimSelector.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/json/JVictimSelector.h>
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

class VictimSelector : public PluginHost::IPlugin,
                       public PluginHost::JSONRPC {
public:
    VictimSelector(const VictimSelector&) = delete;
    VictimSelector& operator=(const VictimSelector&) = delete;

    VictimSelector();
    ~VictimSelector() override;

    BEGIN_INTERFACE_MAP(VictimSelector)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IDispatcher)
    INTERFACE_AGGREGATE(Exchange::IVictimSelector, mImplementation)
    END_INTERFACE_MAP

    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

    static const string SERVICE_NAME;

private:
    PluginHost::IShell* mService;
    Exchange::IVictimSelector* mImplementation;
    Exchange::IConfiguration* mConfigure;
    uint32_t mConnectionId;
};

} // namespace Plugin
} // namespace WPEFramework
