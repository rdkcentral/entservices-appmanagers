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
#include <interfaces/json/JAppStorageManager.h>
#include <interfaces/IAppStorageManager.h>
#include <interfaces/IConfiguration.h>

namespace WPEFramework {
namespace Plugin {

    class AppStorageManager: public PluginHost::IPlugin, public PluginHost::JSONRPC
    {
        public:
            AppStorageManager(const AppStorageManager&) = delete;
            AppStorageManager& operator=(const AppStorageManager&) = delete;

            AppStorageManager();
            virtual ~AppStorageManager();

            BEGIN_INTERFACE_MAP(AppStorageManager)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IAppStorageManager, mStorageManagerImpl)
            END_INTERFACE_MAP

            //  IPlugin methods
            // -------------------------------------------------------------------------------------------------------
            const string Initialize(PluginHost::IShell* service) override;
            void Deinitialize(PluginHost::IShell* service) override;
            string Information() const override;

        private:
            PluginHost::IShell* mCurrentService{};
            uint32_t mConnectionId{};
            Exchange::IAppStorageManager* mStorageManagerImpl{};
            Exchange::IConfiguration* mConfigure{};

        public /* constants */:
            static const string SERVICE_NAME;

    };

} /* namespace Plugin */
} /* namespace WPEFramework */
