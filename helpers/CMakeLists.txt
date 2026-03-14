# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2026 RDK Management
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

set(HELPERS_LIBRARY_NAME AppManagersHelpers)

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)

add_library(${HELPERS_LIBRARY_NAME} SHARED
    UtilsTelemetryMetrics.cpp
    TelemetryReportingBase.cpp)

target_include_directories(${HELPERS_LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/interfaces)

target_link_libraries(${HELPERS_LIBRARY_NAME}
    PUBLIC
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

target_compile_definitions(${HELPERS_LIBRARY_NAME}
    PRIVATE MODULE_NAME=AppManagersHelpers)

if(AIMANAGERS_TELEMETRY_METRICS_SUPPORT)
    target_compile_definitions(${HELPERS_LIBRARY_NAME}
        PUBLIC ENABLE_AIMANAGERS_TELEMETRY_METRICS)
endif()

install(TARGETS ${HELPERS_LIBRARY_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)
