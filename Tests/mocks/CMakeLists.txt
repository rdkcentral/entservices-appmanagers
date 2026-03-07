# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2025 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.8)

set(MODULE_NAME TestMocklib)

set(MODULE_SOURCES
    Rfc.cpp
    Iarm.cpp
    Wraps.cpp
    RBus.cpp
    Telemetry.cpp
    readprocMockInterface.cpp
    Udev.cpp
    libusb.cpp
    DRMScreenCapture.cpp
    systemaudioplatform.cpp
    dsFPD.cpp
    MfrMock.cpp
    PowerManagerHalMock.cpp
    RenderSession.cpp
    gdialservice.cpp
    btmgr.cpp
    MotionDetection.cpp
    devicesettings.cpp
    HdmiCec.cpp
    thunder/Module.cpp
    Dobby.cpp
    rdkshell.cpp
    opkgMock.cpp
    PowerManagerMock.cpp
    tr181api.cpp
)

add_library(${MODULE_NAME} SHARED ${MODULE_SOURCES})

target_include_directories(${MODULE_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/Tests/mocks
        ${CMAKE_SOURCE_DIR}/Tests/mocks/thunder
        ${CMAKE_SOURCE_DIR}/Tests/mocks/devicesettings
        ${CMAKE_SOURCE_DIR}/Tests/mocks/libusb
        ${CMAKE_INSTALL_PREFIX}/include
        ${CMAKE_INSTALL_PREFIX}/include/WPEFramework
        ${CMAKE_CURRENT_SOURCE_DIR}/../../OCIContainer/stubs
        ../../../Thunder/Source/plugins
)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES)

install(TARGETS ${MODULE_NAME}
    DESTINATION lib)
