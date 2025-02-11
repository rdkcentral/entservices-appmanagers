/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
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
**/

#include "RuntimeManager.h"

const string WPEFramework::Plugin::RuntimeManager::SERVICE_NAME = "org.rdk.RuntimeManager";

namespace WPEFramework
{
    namespace {

        static Plugin::Metadata<Plugin::RuntimeManager> metadata(
            /* Version (Major, Minor, Patch) */
            RUNTIME_MANAGER_API_VERSION_NUMBER_MAJOR, RUNTIME_MANAGER_API_VERSION_NUMBER_MINOR, RUNTIME_MANAGER_API_VERSION_NUMBER_PATCH,
            /* Preconditions */
            {},
            /* Terminations */
            {},
            /* Controls */
            {}
        );
    }

    namespace Plugin
    {

        SERVICE_REGISTRATION(RuntimeManager, RUNTIME_MANAGER_API_VERSION_NUMBER_MAJOR, RUNTIME_MANAGER_API_VERSION_NUMBER_MINOR, RUNTIME_MANAGER_API_VERSION_NUMBER_PATCH);

        RuntimeManager* RuntimeManager::sInstance = nullptr;

        RuntimeManager::RuntimeManager(): _service(nullptr), mConnectionId(0), mRuntimeManagerImplementation(nullptr)
        {
            SYSLOG(Logging::Startup, (_T("RuntimeManager Constructor")));
            RuntimeManager::sInstance = this;
        }

        RuntimeManager::~RuntimeManager()
        {
            SYSLOG(Logging::Startup, (_T("RuntimeManager Destructor")));
            RuntimeManager::sInstance = nullptr;
	    mRuntimeManagerImplementation = nullptr;
        }

        const string RuntimeManager::Initialize(PluginHost::IShell* service )
        {
            //uint32_t result = Core::ERROR_GENERAL;
            string retStatus = "";
            ASSERT(nullptr != service);
            ASSERT(0 == mConnectionId);
            ASSERT(nullptr == _service);
            SYSLOG(Logging::Startup, (_T("RuntimeManager::Initialize: PID=%u"), getpid()));
            //{
            //    SYSLOG(Logging::Startup, (_T("RuntimeManager::Initialize: service is not valid")));
            //    retStatus = _T("RuntimeManager plugin could not be initialised");
            //}
	    _service = service;
	    _service->AddRef(); 
	    mRuntimeManagerImplementation = _service->Root<Exchange::IRuntimeManager>(mConnectionId, 5000, _T("RuntimeManagerImplementation"));
            return retStatus;
        }

        void RuntimeManager::Deinitialize(PluginHost::IShell* service)
        {
            SYSLOG(Logging::Startup, (_T("RuntimeManager::Deinitialize: PID=%u"), getpid()));
            ASSERT(_service == service);
            if (nullptr != mRuntimeManagerImplementation)
            {
                // Stop processing:
                RPC::IRemoteConnection* connection = service->RemoteConnection(mConnectionId);
                VARIABLE_IS_NOT_USED uint32_t result = mRuntimeManagerImplementation->Release();
                mRuntimeManagerImplementation = nullptr;
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                if (nullptr != connection)
                {
                   // Lets trigger the cleanup sequence for
                   // out-of-process code. Which will guard
                   // that unwilling processes, get shot if
                   // not stopped friendly :-)
                   connection->Terminate();
                   connection->Release();
                }
            }
            mConnectionId = 0;
            _service->Release();
	    _service = nullptr;
            SYSLOG(Logging::Shutdown, (string(_T("RuntimeManager de-initialised"))));
        }

        string RuntimeManager::Information() const
        {
            return(string("{\"service\": \"") + SERVICE_NAME + string("\"}"));
        }
    } /* namespace Plugin */
} /* namespace WPEFramework */
