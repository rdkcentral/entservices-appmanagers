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

#include "gdialservice.h"
#include <gmock/gmock.h>

gdialServiceImpl* gdialService::impl = nullptr;
GDialNotifier* m_observer = nullptr;

gdialService* gdialService::getInstance(GDialNotifier* observer, const std::vector<std::string>& gdial_args,const std::string& actualprocessName )
{
    static gdialService instance;
    m_observer = observer;
    return &instance;
}

void gdialService::destroyInstance(void) { }

void gdialService::setImpl(gdialServiceImpl* newImpl)
{
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

GDialNotifier* gdialService::getObserverHandle()
{
    EXPECT_TRUE ((nullptr != m_observer));
    return m_observer;
}

GDIAL_SERVICE_ERROR_CODES gdialService::ApplicationStateChanged(std::string applicationName, std::string appState, std::string applicationId, std::string error)
{
    EXPECT_NE(impl, nullptr);
    return (impl->ApplicationStateChanged(applicationName, appState, applicationId, error));
}

GDIAL_SERVICE_ERROR_CODES gdialService::ActivationChanged(std::string activation, std::string friendlyname)
{
    EXPECT_NE(impl, nullptr);
    return(impl->ActivationChanged(activation, friendlyname));
}

GDIAL_SERVICE_ERROR_CODES gdialService::FriendlyNameChanged(std::string friendlyname)
{
    EXPECT_NE(impl, nullptr);
    return(impl->FriendlyNameChanged(friendlyname));
}

std::string gdialService::getProtocolVersion(void)
{
    EXPECT_NE(impl, nullptr);
    return(impl->getProtocolVersion());
}

GDIAL_SERVICE_ERROR_CODES gdialService::RegisterApplications(RegisterAppEntryList* appConfigList)
{
    EXPECT_NE(impl, nullptr);
    return(impl->RegisterApplications(appConfigList));
}

void gdialService::setNetworkStandbyMode(bool nwStandbymode)
{
    EXPECT_NE(impl, nullptr);
    impl->setNetworkStandbyMode(nwStandbymode);
}

GDIAL_SERVICE_ERROR_CODES gdialService::setManufacturerName(std::string manufacturername)
{
    EXPECT_NE(impl, nullptr);
    return(impl->setManufacturerName(manufacturername));
}

GDIAL_SERVICE_ERROR_CODES gdialService::setModelName(std::string modelname)
{
    EXPECT_NE(impl, nullptr);
    return(impl->setModelName(modelname));
}

GDIAL_SERVICE_ERROR_CODES (*ApplicationStateChanged)(std::string applicationName, std::string appState, std::string applicationId, std::string error) = &gdialService::ApplicationStateChanged;
GDIAL_SERVICE_ERROR_CODES (*ActivationChanged)(std::string activation, std::string friendlyname) = &gdialService::ActivationChanged;
GDIAL_SERVICE_ERROR_CODES (*FriendlyNameChanged)(std::string friendlyname) = &gdialService::FriendlyNameChanged;
std::string (*getProtocolVersion)(void) = &gdialService::getProtocolVersion;
GDIAL_SERVICE_ERROR_CODES (*RegisterApplications)(RegisterAppEntryList* appConfigList) = &gdialService::RegisterApplications;
void (*setNetworkStandbyMode)(bool nwStandbymode) = &gdialService::setNetworkStandbyMode;
GDIAL_SERVICE_ERROR_CODES (*setManufacturerName)(std::string manufacturername) = &gdialService::setManufacturerName;
GDIAL_SERVICE_ERROR_CODES (*setModelName)(std::string modelname) = &gdialService::setModelName;