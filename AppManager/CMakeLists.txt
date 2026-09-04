# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
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

set(PLUGIN_NAME AppManager)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

set(PLUGIN_APP_MANAGER_MODE "Off" CACHE STRING "Controls if the plugin should run in its own process, in process or remote")
set(PLUGIN_APP_MANAGER_AUTOSTART false CACHE STRING "Automatically start AppManager plugin")
set(PLUGIN_APP_MANAGER_EXTRA_LIBRARIES "")
set(PLUGIN_APP_MANAGER_STARTUPORDER "" CACHE STRING "Automatically start AppManager plugin")
option(APP_MANAGER_RESOURCE_MONITOR "Enable ResourceMonitor integration and inactive app state transitions" OFF)
set(PLUGIN_APP_MANAGER_RESOURCE_MONITOR 0)
if(APP_MANAGER_RESOURCE_MONITOR)
    set(PLUGIN_APP_MANAGER_RESOURCE_MONITOR 1)
endif()
set(PLUGIN_APP_MANAGER_PAUSED_TO_SUSPENDED_TIMEOUT 60 CACHE STRING "Seconds before moving a paused app to suspended")
set(PLUGIN_APP_MANAGER_SUSPENDED_TO_HIBERNATED_TIMEOUT 15 CACHE STRING "Seconds before evaluating a suspended app for hibernation")
set(PLUGIN_APP_MANAGER_HIBERNATION_STORAGE_PATH "/media/apps/memcr" CACHE STRING "Filesystem path used for hibernated application data")
option(AIMANAGERS_TELEMETRY_METRICS_SUPPORT "AIMANAGERS_TELEMETRY_METRICS_SUPPORT" OFF)

add_definitions(-DAPP_MANAGER_API_VERSION_NUMBER_MAJOR=1)
add_definitions(-DAPP_MANAGER_API_VERSION_NUMBER_MINOR=0)
add_definitions(-DAPP_MANAGER_API_VERSION_NUMBER_PATCH=0)

set(APP_INFRA_ENHANCED_LOGGING_INDICATOR "" CACHE STRING "Path to enhanced logging indicator file")
if(APP_INFRA_ENHANCED_LOGGING_INDICATOR)
	add_definitions(-DAPP_INFRA_ENHANCED_LOGGING_INDICATOR="${APP_INFRA_ENHANCED_LOGGING_INDICATOR}")
endif()

set(APP_MANAGER_IMPLEMENTATION ${PLUGIN_NAME}Implementation)
add_definitions(-DPLUGIN_APP_MANAGER_IMPLEMENTATION_NAME="${APP_MANAGER_IMPLEMENTATION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
        AppManager.cpp
        Module.cpp
)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

set_source_files_properties(AppManager.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        AppManagersHelpers
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)
	
set(PLUGIN_APP_MANAGER_IMPLEMENTATION_SOURCES
    AppManagerImplementation.cpp
    AppInfo.cpp
    AppInfoManager.cpp
    LifecycleInterfaceConnector.cpp
    AppManagerTelemetryReporting.cpp
    Module.cpp)

if(APP_MANAGER_RESOURCE_MONITOR)
    list(APPEND PLUGIN_APP_MANAGER_IMPLEMENTATION_SOURCES AppStateTransitionManager.cpp)
endif()

add_library(${PLUGIN_IMPLEMENTATION} SHARED
    ${PLUGIN_APP_MANAGER_IMPLEMENTATION_SOURCES}
)

target_include_directories(${PLUGIN_IMPLEMENTATION}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers)

if(APP_MANAGER_RESOURCE_MONITOR)
    target_compile_definitions(${PLUGIN_IMPLEMENTATION} PRIVATE APP_MANAGER_RESOURCE_MONITOR)
endif()

set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

set(APP_MANAGER_INCLUDES $ENV{APP_MANAGER_INCLUDES})
separate_arguments(APP_MANAGER_INCLUDES)
include_directories(BEFORE ${APP_MANAGER_INCLUDES})
if (RDK_SERVICES_L1_TEST)
	target_link_options(${PLUGIN_IMPLEMENTATION} PRIVATE -Wl,--wrap,stat)
endif (RDK_SERVICES_L1_TEST)
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE
    CompileSettingsDebug::CompileSettingsDebug ${NAMESPACE}Plugins::${NAMESPACE}Plugins AppManagersHelpers ${PLUGIN_APP_MANAGER_EXTRA_LIBRARIES})

install(TARGETS ${PLUGIN_IMPLEMENTATION}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
