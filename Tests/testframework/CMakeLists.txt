###
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2023 RDK Management
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
###

cmake_minimum_required(VERSION 3.3)
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(WPEFramework)

# All packages that did not deliver a CMake Find script (and some deprecated scripts that need to be removed)
# are located in the cmake directory. Include it in the search.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

if(RDK_SERVICE_L2_TEST)
    include(l2tests.cmake)
endif()

if(RDK_SERVICE_L2HALMOCK)
    include(l2HalMock.cmake)
endif()

if(RDK_SERVICES_L1_TEST)
    include(l1tests.cmake)
endif()

if(RDK_SERVICES_L1_TEST)
    add_subdirectory(L1Tests)
endif()

if(RDK_SERVICE_L2_TEST)
    if(L2_TEST_OOP_RPC)
        add_subdirectory(mocks/MockPlugin)
    endif()
    add_subdirectory(L2Tests/L2TestsPlugin)
    add_subdirectory(L2Tests)
endif()
if(PLUGIN_MAINTENANCEMANAGER)
    add_subdirectory(mocks/MockAuthService)
    add_subdirectory(mocks/MockSecManager)
endif()
