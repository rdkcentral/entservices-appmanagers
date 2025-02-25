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

#include "RuntimeManagerHandler.h"
#include "UtilsLogging.h"
#include "tracing/Logging.h"
#include <sstream>

namespace WPEFramework {
namespace Plugin {

#define FIREBOLT_ENDPOINT_APPACCESS_PORT 3473

RuntimeManagerHandler::RuntimeManagerHandler()
: mRuntimeManager(nullptr), mFireboltAccessPort(FIREBOLT_ENDPOINT_APPACCESS_PORT), mEventHandler(nullptr)
{
    LOGINFO("Create RuntimeManagerHandler Instance");
}

RuntimeManagerHandler::~RuntimeManagerHandler()
{
    LOGINFO("Delete RuntimeManagerHandler Instance");
}

bool RuntimeManagerHandler::initialize(PluginHost::IShell* service, IEventHandler* eventHandler)
{
    bool ret = false;
    mEventHandler = eventHandler;
    mRuntimeManager = service->QueryInterfaceByCallsign<Exchange::IRuntimeManager>("org.rdk.RuntimeManager");
    if (mRuntimeManager != nullptr)
    {
        ret = true;
        // TODO any event handler registrations
        //registerEventHandlers();
    }
    else
    {
        LOGERR("runtimemanager is null \n");
    }
    // PENDING any event handler registrations
    return ret;
}

void RuntimeManagerHandler::deinitialize()
{
    uint32_t result = mRuntimeManager->Release();
    LOGINFO("Terminated runtime manager hanlder [%d] \n", result);
    mRuntimeManager = nullptr;
}

bool RuntimeManagerHandler::getRuntimeStats(const string& appInstanceId, string& info)
{
    Core::hresult result = mRuntimeManager->GetInfo(appInstanceId, info);
    if (Core::ERROR_NONE != result)
    {
        LOGINFO("unable to get information about application");
        return false;
    }
    return true;
}

bool RuntimeManagerHandler::run(const string& appInstanceId, const string& appPath, const string& appConfig, const string& runtimeAppId, const string& runtimePath, const string& runtimeConfig, const string& environmentVars, const bool enableDebugger, const string& launchArgs, const string& xdgRuntimeDir, const string& displayName, string& errorReason)
{
    JsonArray environmentVarsArray, debugSettingsArray, pathsArray, portsArray;
    // read data from parameters
    environmentVarsArray.FromString(environmentVars);
    // QUESTION HOW TO GET OTHER PARAMS ports, paths, debugsettings?
    // convert launchArgs to meaningful environment variables when launching a container

    // FOR Q1
    // -- debugSettings - [] //for vbn builds
    // -- paths - paths to set in container - path for XDG_RUNTIME_DIR
    
    uint32_t userId = 0, groupId = 0;
    std::list<string> environmentVarsList, debugSettingsList, pathsList;
    std::list<uint32_t> portsList;

    portsList.push_back(mFireboltAccessPort);

    pathsList.push_back(xdgRuntimeDir);

    for (unsigned int i=0; i<environmentVarsArray.Length(); i++)
    {
        environmentVarsList.push_back(environmentVarsArray[i].String());
    }
    std::stringstream ss;
    ss << "FIREBOLT_ENDPOINT=http://localhost:" << mFireboltAccessPort << "?session=" << appInstanceId;
    string fireboltEndPoint(ss.str());
    environmentVarsList.push_back(fireboltEndPoint);

    std::stringstream xdgRuntimeEnv;
    xdgRuntimeEnv << "XDG_RUNTIME_DIR=" << xdgRuntimeDir;
    string xdgRuntimeEnvString(xdgRuntimeEnv.str());
    environmentVarsList.push_back(xdgRuntimeEnvString);

    std::stringstream waylandDisplayEnv;
    waylandDisplayEnv << "WAYLAND_DISPLAY=" << displayName;
    string displayNameEnvString(waylandDisplayEnv.str());
    environmentVarsList.push_back(displayNameEnvString);
   
    // prepare arguments to pass
    RPC::IStringIterator* environmentVarsIterator{};
    RPC::IStringIterator* debugSettingsIterator{};
    RPC::IStringIterator* pathsIterator{};
    RPC::IValueIterator* portsIterator{};

    environmentVarsIterator = Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(environmentVarsList);
    debugSettingsIterator = Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(debugSettingsList);
    pathsIterator = Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(pathsList);
    portsIterator = Core::Service<RPC::ValueIterator>::Create<RPC::IValueIterator>(portsList);

    Core::hresult result = mRuntimeManager->Run(appInstanceId, appPath, runtimePath, environmentVarsIterator, userId, groupId, portsIterator, pathsIterator, debugSettingsIterator);
    if (Core::ERROR_NONE != result)
    {
        errorReason = "unable to start running application";
        return false;
    }
    return true;
}

bool RuntimeManagerHandler::kill(const string& appInstanceId, string& errorReason)
{
    Core::hresult result = mRuntimeManager->Kill(appInstanceId);
    if (Core::ERROR_NONE != result)
    {
        errorReason = "unable to kill application";
        return false;
    }
    return true;
}

bool RuntimeManagerHandler::terminate(const string& appInstanceId, string& errorReason)
{
    Core::hresult result = mRuntimeManager->Terminate(appInstanceId);
    if (Core::ERROR_NONE != result)
    {
        errorReason = "unable to terminate application";
        return false;
    }
    return true;
}

bool RuntimeManagerHandler::suspend(const string& appInstanceId, string& errorReason)
{
    Core::hresult result = mRuntimeManager->Suspend(appInstanceId);
    if (Core::ERROR_NONE != result)
    {
        errorReason = "unable to suspend application";
        return false;
    }
    return true;
}

bool RuntimeManagerHandler::resume(const string& appInstanceId, string& errorReason)
{
    Core::hresult result = mRuntimeManager->Resume(appInstanceId);
    if (Core::ERROR_NONE != result)
    {
        errorReason = "unable to resume application";
        return false;
    }
    return true;
}

bool RuntimeManagerHandler::hibernate(const string& appInstanceId, string& errorReason)
{
    Core::hresult result = mRuntimeManager->Hibernate(appInstanceId);
    if (Core::ERROR_NONE != result)
    {
        errorReason = "unable to hibernate application";
        return false;
    }
    return true;
}

} // namespace Plugin
} // namespace WPEFramework
