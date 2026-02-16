/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
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
*/

#pragma once

#include <gmock/gmock.h>
#include <interfaces/IOCIContainer.h>

using ::WPEFramework::Exchange::IOCIContainer;
using namespace WPEFramework;


class OCIContainerMock : public IOCIContainer {
public:
    OCIContainerMock() = default;
    virtual ~OCIContainerMock() = default;

    MOCK_METHOD(Core::hresult, Register,(WPEFramework::Exchange::IOCIContainer::INotification *notification), (override));
    MOCK_METHOD(Core::hresult, Unregister,(WPEFramework::Exchange::IOCIContainer::INotification *notification), (override));
    MOCK_METHOD(Core::hresult, ListContainers, (string& containers , bool& success , string& errorReason ), (override));
    MOCK_METHOD(Core::hresult, GetContainerInfo, (const string& containerId , string& info , bool& success, string& errorReason ), (override));
    MOCK_METHOD(Core::hresult, GetContainerState, (const string& containerId /* @in */, ContainerState& state /* @out */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, StartContainer, (const string& containerId /* @in */, const string& bundlePath /* @in */, const string& command /* @in */, const string& westerosSocket /* @in */, int32_t& descriptor /* @out */, bool& success /* @out */, string& errorReason /* @out */), ( override));
    MOCK_METHOD(Core::hresult, StartContainerFromDobbySpec, (const string& containerId /* @in */, const string& dobbySpec /* @in */, const string& command /* @in */, const string& westerosSocket /* @in */, int32_t& descriptor /* @out */, bool& success /* @out */, string& errorReason /* @out */), ( override));
    MOCK_METHOD(Core::hresult, StopContainer, (const string& containerId /* @in */, bool force /* @in */, bool& success /* @out */, string& errorReason /* @out */), ( override));
    MOCK_METHOD(Core::hresult, PauseContainer, (const string& containerId /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, ResumeContainer, (const string& containerId /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, HibernateContainer, (const string& containerId /* @in */, const string& options /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, WakeupContainer, (const string& containerId /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, ExecuteCommand, (const string& containerId /* @in */, const string& options /* @in */, const string& command /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, Annotate, (const string& containerId /* @in */, const string& key /* @in */, const string& value /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, RemoveAnnotation, (const string& containerId /* @in */, const string& key /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, Mount, (const string& containerId /* @in */, const string& source /* @in */, const string& target /* @in */, const string& type /* @in */, const string& options /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, Unmount, (const string& containerId /* @in */, const string& target /* @in */, bool& success /* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));
};


class IOCIContainerNotificationMock : public IOCIContainer::INotification {
    public:
    IOCIContainerNotificationMock() = default;
    virtual ~IOCIContainerNotificationMock() = default;

    MOCK_METHOD(void, OnContainerStarted, (const string& containerId, const string& name), (override));
    MOCK_METHOD(void, OnContainerStopped, (const string& containerId, const string& name), (override));
    MOCK_METHOD(void, OnContainerFailed, (const string& containerId, const string& name, uint32_t error), (override));
    MOCK_METHOD(void, OnContainerStateChanged, (const string& containerId, WPEFramework::Exchange::IOCIContainer::ContainerState state), (override));

};
