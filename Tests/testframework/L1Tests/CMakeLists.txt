
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
project(RdkServicesL1Test)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(GTest REQUIRED)

set (TEST_SRC
    #tests/test_UtilsFile.cpp
    test_JSON.cpp
)

set (TEST_LIB
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
  )

set(ENTSERVICES_REPO_L1TEST_LIB_PATH ${CMAKE_INSTALL_PREFIX}/lib)

set (TEST_INC ../../helpers)

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

# Add Wraps.cpp directly to executable so wrap mechanism works correctly
list(APPEND TEST_SRC ../mocks/Wraps.cpp)

add_executable(${PROJECT_NAME} ${TEST_SRC})
target_link_libraries(${PROJECT_NAME} TestMocklib)

# Add wrap flags for the executable to create __real_* symbols and export them for shared libraries
target_link_options(${PROJECT_NAME} PRIVATE 
    -Wl,--export-dynamic
    -Wl,--wrap,system
    -Wl,--wrap,popen
    -Wl,--wrap,syslog
    -Wl,--wrap,v_secure_popen
    -Wl,--wrap,v_secure_pclose
    -Wl,--wrap,unlink
    -Wl,--wrap,v_secure_system
    -Wl,--wrap,pclose
    -Wl,--wrap,access
    -Wl,--wrap,rmdir
    -Wl,--wrap,closedir
    -Wl,--wrap,mkdir
    -Wl,--wrap,readdir
    -Wl,--wrap,chown
    -Wl,--wrap,nftw
    -Wl,--wrap,open
    -Wl,--wrap,opendir
    -Wl,--wrap,stat
    -Wl,--wrap,statvfs
    -Wl,--wrap,setmntent
    -Wl,--wrap,getmntent
)

# AppManager plugins: AppManager, LifecycleManager, StorageManager, RuntimeManager, DownloadManager, PreinstallManager, PackageManager
if(PLUGIN_APPMANAGER OR PLUGIN_LIFECYCLE_MANAGER OR PLUGIN_STORAGE_MANAGER OR PLUGIN_RUNTIME_MANAGER OR PLUGIN_DOWNLOADMANAGER OR PLUGIN_PREINSTALL_MANAGER OR PLUGIN_PACKAGE_MANAGER)
    message(STATUS "linking libWPEFrameworkL1TestsIN to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} ${NAMESPACE}L1TestsIN)
endif()

include(GoogleTest)
include_directories(${TEST_INC})

target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins)

target_link_libraries(${PROJECT_NAME} gtest_main gmock gtest ${TEST_LIB})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        ../mocks
        ../mocks/devicesettings
        ../mocks/thunder
        ../mocks/libusb
        ${CMAKE_CURRENT_SOURCE_DIR}/../../OCIContainer/stubs
        ../../../Thunder/Source/plugins
        )

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
