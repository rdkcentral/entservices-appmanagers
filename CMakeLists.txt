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

find_package(WPEFramework)

# Ensure namespace-scoped plugin/definitions targets exist across toolchains
# that expose either modern imported targets or legacy *_LIBRARIES variables.
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions QUIET)
if(NOT ${NAMESPACE}Definitions_FOUND)
    find_package(${NAMESPACE}Cpc_Definitions QUIET)
endif()

if(NOT TARGET ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
    foreach(_plugins_candidate
            ${NAMESPACE}::Plugins
            Plugins::Plugins
            ThunderPlugins::ThunderPlugins)
        if(TARGET ${_plugins_candidate})
            add_library(${NAMESPACE}Plugins::${NAMESPACE}Plugins ALIAS ${_plugins_candidate})
            break()
        endif()
    endforeach()

    if(NOT TARGET ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
        add_library(${NAMESPACE}Plugins::${NAMESPACE}Plugins INTERFACE IMPORTED)

        if(DEFINED ${NAMESPACE}Plugins_INCLUDE_DIRS)
            set_property(TARGET ${NAMESPACE}Plugins::${NAMESPACE}Plugins
                APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES "${${NAMESPACE}Plugins_INCLUDE_DIRS}")
        endif()

        if(DEFINED ${NAMESPACE}Plugins_LIBRARIES)
            set_property(TARGET ${NAMESPACE}Plugins::${NAMESPACE}Plugins
                APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${${NAMESPACE}Plugins_LIBRARIES}")
        elseif(DEFINED ${NAMESPACE}Plugins_LIBRARY)
            set_property(TARGET ${NAMESPACE}Plugins::${NAMESPACE}Plugins
                APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${${NAMESPACE}Plugins_LIBRARY}")
        endif()
    endif()
endif()

if(NOT TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions)
    foreach(_definitions_candidate
            ${NAMESPACE}::Definitions
            Definitions::Definitions
            ${NAMESPACE}Cpc_Definitions::${NAMESPACE}Cpc_Definitions
            WPEFrameworkCpc_Definitions::WPEFrameworkCpc_Definitions
            ThunderDefinitions::ThunderDefinitions)
        if(TARGET ${_definitions_candidate})
            add_library(${NAMESPACE}Definitions::${NAMESPACE}Definitions ALIAS ${_definitions_candidate})
            break()
        endif()
    endforeach()

    if(NOT TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions)
        add_library(${NAMESPACE}Definitions::${NAMESPACE}Definitions INTERFACE IMPORTED)

        if(DEFINED ${NAMESPACE}Definitions_INCLUDE_DIRS)
            set_property(TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions
                APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES "${${NAMESPACE}Definitions_INCLUDE_DIRS}")
        elseif(DEFINED ${NAMESPACE}Cpc_Definitions_INCLUDE_DIRS)
            set_property(TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions
                APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES "${${NAMESPACE}Cpc_Definitions_INCLUDE_DIRS}")
        endif()

        if(DEFINED ${NAMESPACE}Definitions_LIBRARIES)
            set_property(TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions
                APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${${NAMESPACE}Definitions_LIBRARIES}")
        elseif(DEFINED ${NAMESPACE}Definitions_LIBRARY)
            set_property(TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions
                APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${${NAMESPACE}Definitions_LIBRARY}")
        elseif(DEFINED ${NAMESPACE}Cpc_Definitions_LIBRARIES)
            set_property(TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions
                APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${${NAMESPACE}Cpc_Definitions_LIBRARIES}")
        elseif(DEFINED ${NAMESPACE}Cpc_Definitions_LIBRARY)
            set_property(TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions
                APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${${NAMESPACE}Cpc_Definitions_LIBRARY}")
        endif()
    endif()
endif()

# All packages that did not deliver a CMake Find script (and some deprecated scripts that need to be removed)
# are located in the cmake directory. Include it in the search.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Product configuration dir
set(PRODUCT_CONFIG_DIR "/etc/entservices")

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGE_DIRECTORY)

# for writing pc and config files
include(CmakeHelperFunctions)

if(RDK_SERVICE_L2_TEST)
    add_subdirectory(Tests/L2Tests)
endif()

option(DISABLE_SECURITY_TOKEN "Disable security token" OFF)

if(RDK_SERVICES_L1_TEST)
    add_subdirectory(Tests/L1Tests)
endif()

if(DISABLE_SECURITY_TOKEN)
    add_definitions(-DDISABLE_SECURITY_TOKEN)
endif()

option(AIMANAGERS_TELEMETRY_METRICS_SUPPORT "AIMANAGERS_TELEMETRY_METRICS_SUPPORT" OFF)

add_subdirectory(helpers)

if(PLUGIN_RDK_WINDOW_MANAGER)
    add_subdirectory(RDKWindowManager)
endif()

if(PLUGIN_LIFECYCLE_MANAGER)
    add_subdirectory(LifecycleManager)
endif()

if(PLUGIN_APPMANAGER)
    add_subdirectory(AppManager)
endif()

if(PLUGIN_RUNTIME_MANAGER)
	add_subdirectory(RuntimeManager)
endif()

if(PLUGIN_APP_STORAGE_MANAGER)
	add_subdirectory(AppStorageManager)
endif()

if(PLUGIN_PACKAGE_MANAGER)
    add_subdirectory(PackageManager)
endif()

if(PLUGIN_DOWNLOADMANAGER)
    add_subdirectory(DownloadManager)
endif()

if(PLUGIN_PREINSTALL_MANAGER)
    add_subdirectory(PreinstallManager)
endif()

if(PLUGIN_TELEMETRYMETRICS)
    add_subdirectory(TelemetryMetrics)
endif()


if(WPEFRAMEWORK_CREATE_IPKG_TARGETS)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_COMPONENTS_GROUPING IGNORE)

    set(CPACK_DEBIAN_PACKAGE_NAME "${WPEFRAMEWORK_PLUGINS_OPKG_NAME}")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${WPEFRAMEWORK_PLUGINS_OPKG_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${WPEFRAMEWORK_PLUGINS_OPKG_ARCHITECTURE}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${WPEFRAMEWORK_PLUGINS_OPKG_MAINTAINER}")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${WPEFRAMEWORK_PLUGINS_OPKG_DESCRIPTION}")
    set(CPACK_PACKAGE_FILE_NAME "${WPEFRAMEWORK_PLUGINS_OPKG_FILE_NAME}")

    include(CPack)
endif()
