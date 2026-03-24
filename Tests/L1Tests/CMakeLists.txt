# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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
set(PLUGIN_NAME L1TestsIN)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(GTest REQUIRED)

set (TEST_SRC
    tests/test_UtilsFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/Wraps.cpp
)

set (TEST_LIB
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

set(TEST_INC ../../helpers)
set(TEST_RUNNER_NAME RdkServicesL1Test)
option(BUILD_L1_TESTS_SHARED_MODULE "Build legacy L1 tests shared module" OFF)

#########################################################################################
# add_plugin_test_ex: Macro to add plugin tests, it will append to TEST_SRC, TEST_INC,
#                     and TEST_LIB. Args are positional.
#                     arg1: test file list as **string**
#                     arg2: include dir list as **string**
#                     arg3: plugin libs to link list as **string**
#
# sample invocation : 
#    add_plugin_test_ex(PLUGIN_NAME
#                       "test/file1.cpp;test/file2.cpp"
#                       "../../inc_dir1;../../inc_dir2"
#                       "${NAMESPACE}PluginName;${NAMESPACE}PluginNameImplementation")
# ----------------------------------------- OR ------------------------------------------
# list(APPEND PLUGIN_NAME_SRC test/file1.cpp)
# list(APPEND PLUGIN_NAME_SRC test/file2.cpp)
#
# list(APPEND PLUGIN_NAME_INC ../../inc_dir1)
# list(APPEND PLUGIN_NAME_INC ../../inc_dir2)
#
# list(APPEND PLUGIN_NAME_LIB ${NAMESPACE}PluginName)
# list(APPEND PLUGIN_NAME_LIB ${NAMESPACE}PluginNameImplementation)
#
#    add_plugin_test_ex(PLUGIN_NAME
#                       "${PLUGIN_NAME_SRC}"
#                       "${PLUGIN_NAME_INC}"
#                       "${PLUGIN_NAME_LIB}")
#
# NOTE: Alternatively test can choose to update `TEST_SRC`, `TEST_INC` & `TEST_LIB`
#       directly (like in the case of Miracast)
#########################################################################################
macro(add_plugin_test_ex plugin_opt plugin_test_sources_str plugin_includes_str plugin_libs_str)
    # Check if the plugin option is enabled
    if(${plugin_opt})
        message(STATUS "${plugin_opt}=ON")

        string(REPLACE ";" ";" srclist "${plugin_test_sources_str}")
        string(REPLACE ";" ";" inclist "${plugin_includes_str}")
        string(REPLACE ";" ";" liblist "${plugin_libs_str}")

        foreach(item IN LISTS srclist)
            # Add each test source file
            list(APPEND TEST_SRC ${item})
        endforeach()

        foreach(item IN LISTS inclist)
            # Add each include directory
            list(APPEND TEST_INC ${item})
        endforeach()

        foreach(item IN LISTS liblist)
            # Add each libraries to link
            list(APPEND TEST_LIB ${item})
        endforeach()
    else()
        message(STATUS "${plugin_opt}=OFF")
    endif()
endmacro()

# helper to add plugin test
macro(add_plugin_test plugin_name test_files)
    # Convert plugin name to uppercase for the option variable
    string(TOUPPER "${plugin_name}" plugin_option)
    set(plugin_opt "PLUGIN_${plugin_option}")

    add_plugin_test_ex(${plugin_opt} "${test_files}" "../../${plugin_name}" "${NAMESPACE}${plugin_name}")
endmacro()

# PLUGIN_APPMANAGER
set(APPMANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/AppManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(APPMANAGER_LIBS ${NAMESPACE}AppManager ${NAMESPACE}AppManagerImplementation)
add_plugin_test_ex(PLUGIN_APPMANAGER tests/test_AppManager.cpp "${APPMANAGER_INC}" "${APPMANAGER_LIBS}")

# PLUGIN_RUNTIME_MANAGER
set(RUNTIMEMANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/RuntimeManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(RUNTIMEMANAGER_LIBS ${NAMESPACE}RuntimeManager ${NAMESPACE}RuntimeManagerImplementation)
add_plugin_test_ex(PLUGIN_RUNTIME_MANAGER tests/test_RunTimeManager.cpp "${RUNTIMEMANAGER_INC}" "${RUNTIMEMANAGER_LIBS}")

# PLUGIN_APP_STORAGE_MANAGER
set(STORAGE_MANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/AppStorageManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(STORAGE_MANAGER_LIBS ${NAMESPACE}AppStorageManager ${NAMESPACE}AppStorageManagerImplementation)
add_plugin_test_ex(PLUGIN_APP_STORAGE_MANAGER tests/test_AppStorageManager.cpp "${STORAGE_MANAGER_INC}" "${STORAGE_MANAGER_LIBS}")

# PLUGIN_LIFECYCLE_MANAGER
set(LIFECYCLEMANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/LifecycleManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(LIFECYCLEMANAGER_LIBS ${NAMESPACE}LifecycleManager ${NAMESPACE}Definitions)
add_plugin_test_ex(PLUGIN_LIFECYCLE_MANAGER tests/test_LifecycleManager.cpp "${LIFECYCLEMANAGER_INC}" "${LIFECYCLEMANAGER_LIBS}")

# PLUGIN_PREINSTALL_MANAGER
set(PREINSTALL_MANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/PreinstallManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(PREINSTALL_MANAGER_LIBS ${NAMESPACE}PreinstallManager ${NAMESPACE}PreinstallManagerImplementation)
add_plugin_test_ex(PLUGIN_PREINSTALL_MANAGER tests/test_PreinstallManager.cpp "${PREINSTALL_MANAGER_INC}" "${PREINSTALL_MANAGER_LIBS}")

# PLUGIN_PACKAGE_MANAGER
set(PACKAGEMANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/PackageManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(PACKAGEMANAGER_LIBS ${NAMESPACE}PackageManagerRDKEMS)
add_plugin_test_ex(PLUGIN_PACKAGE_MANAGER tests/test_PackageManager.cpp "${PACKAGEMANAGER_INC}" "${PACKAGEMANAGER_LIBS}")

# PLUGIN_DOWNLOADMANAGER
set(DOWNLOADMANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/DownloadManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(DOWNLOADMANAGER_LIBS ${NAMESPACE}DownloadManager ${NAMESPACE}DownloadManagerImplementation)
add_plugin_test_ex(PLUGIN_DOWNLOADMANAGER tests/test_DownloadManager.cpp "${DOWNLOADMANAGER_INC}" "${DOWNLOADMANAGER_LIBS}")

# PLUGIN_RDK_WINDOW_MANAGER
set(RDKWINDOWMANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/RDKWindowManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(RDKWINDOWMANAGER_LIBS ${NAMESPACE}RDKWindowManager ${NAMESPACE}RDKWindowManagerImplementation)
add_plugin_test_ex(PLUGIN_RDK_WINDOW_MANAGER tests/test_RDKWindowManager.cpp "${RDKWINDOWMANAGER_INC}" "${RDKWINDOWMANAGER_LIBS}")

if(BUILD_L1_TESTS_SHARED_MODULE)
    add_library(${MODULE_NAME} SHARED ${TEST_SRC})
endif()

add_executable(${TEST_RUNNER_NAME} ${TEST_SRC})

include_directories(${TEST_INC})

if(BUILD_L1_TESTS_SHARED_MODULE)
    target_link_directories(${MODULE_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins)
endif()
target_link_directories(${TEST_RUNNER_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins)

if(BUILD_L1_TESTS_SHARED_MODULE)
    target_link_libraries(${MODULE_NAME} ${TEST_LIB})

    target_include_directories(${MODULE_NAME} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
    )

    target_include_directories(${MODULE_NAME}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
            ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/Tests/mocks
            ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/Tests/mocks/thunder
            ${CMAKE_SOURCE_DIR}/../Thunder/Source/plugins
    )

    target_compile_definitions(${MODULE_NAME} PRIVATE UNIT_TEST)

    if(RDK_SERVICES_L1_TEST)
        target_link_options(${MODULE_NAME} PRIVATE "-Wl,-wrap,opendir")
    endif()
endif()

target_link_libraries(${TEST_RUNNER_NAME} gtest_main gmock gtest ${TEST_LIB})

target_compile_definitions(${TEST_RUNNER_NAME} PRIVATE UNIT_TEST)

target_link_options(${TEST_RUNNER_NAME} PRIVATE
    -Wl,--wrap,system
    -Wl,--wrap,popen
    -Wl,--wrap,pclose
    -Wl,--wrap,setmntent
    -Wl,--wrap,getmntent
    -Wl,--wrap,unlink
    -Wl,--wrap,statvfs
    -Wl,--wrap,mkdir
    -Wl,--wrap,stat
    -Wl,--wrap,open
    -Wl,--wrap,rmdir
    -Wl,--wrap,access
    -Wl,--wrap,chown
    -Wl,--wrap,nftw
    -Wl,--wrap,opendir
    -Wl,--wrap,readdir
    -Wl,--wrap,closedir
)

target_include_directories(${TEST_RUNNER_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/Tests/mocks
        ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/Tests/mocks/thunder
        ${CMAKE_SOURCE_DIR}/../Thunder/Source/plugins
)

install(TARGETS ${TEST_RUNNER_NAME} DESTINATION bin)

if(BUILD_L1_TESTS_SHARED_MODULE)
    install(TARGETS ${MODULE_NAME} DESTINATION lib)
    write_config(${PLUGIN_NAME})
endif()
