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

#include "AppManagerImplementation.h"
#include "LifecycleInterfaceConnector.h"
#include "common/AppManagerL0Fakes.hpp"
#include "common/L0Expect.hpp"

uint32_t Test_AM_MapLifecycleStateAndErrorReason()
{
    L0Test::TestResult tr;
    L0Test::AppManagerServiceMock service;
    WPEFramework::Plugin::LifecycleInterfaceConnector connector(&service);

    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(connector.mapAppLifecycleState(WPEFramework::Exchange::ILifecycleManager::LifecycleState::UNLOADED)),
        static_cast<uint32_t>(WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED),
        "UNLOADED maps to APP_STATE_UNLOADED");
    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(connector.mapAppLifecycleState(WPEFramework::Exchange::ILifecycleManager::LifecycleState::ACTIVE)),
        static_cast<uint32_t>(WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE),
        "ACTIVE maps to APP_STATE_ACTIVE");
    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(connector.mapAppLifecycleState(static_cast<WPEFramework::Exchange::ILifecycleManager::LifecycleState>(999))),
        static_cast<uint32_t>(WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN),
        "Unknown lifecycle state maps to APP_STATE_UNKNOWN");

    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(connector.mapErrorReason(std::string("ERROR_CREATE_DISPLAY"))),
        static_cast<uint32_t>(WPEFramework::Exchange::IAppManager::AppErrorReason::APP_ERROR_CREATE_DISPLAY),
        "ERROR_CREATE_DISPLAY maps correctly");
    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(connector.mapErrorReason(std::string("does-not-exist"))),
        static_cast<uint32_t>(WPEFramework::Exchange::IAppManager::AppErrorReason::APP_ERROR_UNKNOWN),
        "Unknown error string maps to APP_ERROR_UNKNOWN");

    return tr.failures;
}

uint32_t Test_AM_LifecycleConnectorNullServiceConstructor()
{
    L0Test::TestResult tr;

    {
        WPEFramework::Plugin::LifecycleInterfaceConnector connector(nullptr);
        L0Test::ExpectTrue(tr, nullptr != WPEFramework::Plugin::LifecycleInterfaceConnector::_instance, "Lifecycle connector singleton is set during construction");
        L0Test::ExpectEqStr(tr, connector.GetAppInstanceId(std::string("missing")), std::string(), "GetAppInstanceId() returns empty for missing app");
    }

    L0Test::ExpectTrue(tr, nullptr == WPEFramework::Plugin::LifecycleInterfaceConnector::_instance, "Lifecycle connector singleton is cleared after destruction");
    return tr.failures;
}

uint32_t Test_AM_LifecycleConnectorOnAppStateChangedWithoutManager()
{
    L0Test::TestResult tr;
    L0Test::AppManagerServiceMock service;
    WPEFramework::Plugin::LifecycleInterfaceConnector connector(&service);

    WPEFramework::Plugin::AppManagerImplementation::_instance = nullptr;
    connector.OnAppStateChanged(std::string("app1"), WPEFramework::Exchange::ILifecycleManager::LifecycleState::ACTIVE, std::string("ERROR_INVALID_PARAM"));
    L0Test::ExpectTrue(tr, true, "OnAppStateChanged() remains stable when AppManagerImplementation singleton is null");

    return tr.failures;
}

uint32_t Test_AM_LifecycleConnectorRemoveAppInfoByAppId()
{
    L0Test::TestResult tr;
    L0Test::AppManagerServiceMock service;
    WPEFramework::Plugin::LifecycleInterfaceConnector connector(&service);

    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    WPEFramework::Plugin::AppInfo info;
    info.setAppInstanceId("instance-1");
    WPEFramework::Plugin::AppInfoManager::getInstance().upsert("app-to-remove", [&](WPEFramework::Plugin::AppInfo& a) { a = info; });

    connector.removeAppInfoByAppId("app-to-remove");
    L0Test::ExpectTrue(tr, !WPEFramework::Plugin::AppInfoManager::getInstance().exists("app-to-remove"), "removeAppInfoByAppId() removes existing app");

    connector.removeAppInfoByAppId("missing-app");
    L0Test::ExpectTrue(tr, true, "removeAppInfoByAppId() remains stable for missing app");

    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_LifecycleConnectorGetAppInstanceId()
{
    L0Test::TestResult tr;
    L0Test::AppManagerServiceMock service;
    WPEFramework::Plugin::LifecycleInterfaceConnector connector(&service);

    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    WPEFramework::Plugin::AppInfoManager::getInstance().setAppInstanceId("app1", "instance-1");
    L0Test::ExpectEqStr(tr, connector.GetAppInstanceId("app1"), std::string("instance-1"), "GetAppInstanceId() returns the stored instance id");
    L0Test::ExpectEqStr(tr, connector.GetAppInstanceId("missing"), std::string(), "GetAppInstanceId() returns empty for missing entry");

    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    return tr.failures;
}
