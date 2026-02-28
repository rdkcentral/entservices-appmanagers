###
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
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
###

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

set (TEST_INC ${CMAKE_SOURCE_DIR}/helpers)

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

    add_plugin_test_ex(${plugin_opt} "${test_files}" "${CMAKE_SOURCE_DIR}/${plugin_name}" "${NAMESPACE}${plugin_name}")
endmacro()

add_executable(${PROJECT_NAME} ${TEST_SRC})


# PLUGIN_FRAMERATE and  PLUGIN_USERPREFERENCES and PLUGIN_WAREHOUSE and PLUGIN_DEVICEINFO
if(PLUGIN_DEVICEDIAGNOSTICS OR PLUGIN_USERPREFERENCES OR PLUGIN_WAREHOUSE OR PLUGIN_DEVICEINFO OR PLUGIN_DISPLAYINFO OR PLUGIN_DISPLAYSETTINGS OR PLUGIN_FRAMERATE OR PLUGIN_POWERMANAGER OR PLUGIN_SYSTEMSERVICES)
    message(STATUS "linking libWPEFrameworkL1TestsDD to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} "${ENTSERVICES_REPO_L1TEST_LIB_PATH}/libWPEFrameworkL1TestsDD.so")
endif()

#PLUGIN_PRIVACY and PLUGIN_PLAYBACKSYNC and PLUGIN_AUTHSERVICE
if(PLUGIN_PRIVACY OR PLUGIN_PLAYBACKSYNC OR PLUGIN_AUTHSERVICE)
    message(STATUS "linking libWPEFrameworkL1TestsCPC to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} "${ENTSERVICES_REPO_L1TEST_LIB_PATH}/libWPEFrameworkL1TestsCPC.so")
endif()


# PLUGIN_USBDEVICE and PLUGIN_USB_MASS_STORAGE and PLUGIN_OCICONTAINER and PLUGIN_APPMANAGER and PLUGIN_STORAGE_MANAGER
if(PLUGIN_USBDEVICE OR PLUGIN_USB_MASS_STORAGE OR PLUGIN_OCICONTAINER OR PLUGIN_APPMANAGER OR PLUGIN_STORAGE_MANAGER)
    message(STATUS "linking libWPEFrameworkL1TestsIN to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} ${NAMESPACE}L1TestsIN)
endif()

# PLUGIN_FIRMWAREUPDATE and PLUGIN_MAINTENANCEMANAGER and PLUGIN_PACKAGER
if(PLUGIN_FIRMWAREUPDATE OR PLUGIN_MAINTENANCEMANAGER OR PLUGIN_PACKAGER)
    message(STATUS "linking libWPEFrameworkL1TestsSU to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} "${ENTSERVICES_REPO_L1TEST_LIB_PATH}/libWPEFrameworkL1TestsSU.so")
endif()

#PLUGIN_AVINPUT and PLUGIN_AVOUTPUT and PLUGIN_HDMICECSOURCE and PLUGIN_HDMICECSINK and PLUGIN_HDCPPROFILE and PLUGIN_HDMIINPUT 
if(PLUGIN_AVINPUT OR PLUGIN_AVOUTPUT OR PLUGIN_HDMICECSOURCE OR PLUGIN_HDMICECSINK OR PLUGIN_HDCPPROFILE OR PLUGIN_HDMIINPUT)
    message(STATUS "linking libWPEFrameworkL1TestsIO to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} "${ENTSERVICES_REPO_L1TEST_LIB_PATH}/libWPEFrameworkL1TestsIO.so")
endif()

#PLUGIN_MIRACAST and PLUGIN_XCAST
if(PLUGIN_MIRACAST OR PLUGIN_XCAST)
    message(STATUS "linking libWPEFrameworkL1TestsCA to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} "${ENTSERVICES_REPO_L1TEST_LIB_PATH}/libWPEFrameworkL1TestsCA.so")
endif()

#PLUGIN_MOTION_DETECTION and PLUGIN_FRONTPANEL
if(PLUGIN_MOTION_DETECTION OR PLUGIN_FRONTPANEL)
    message(STATUS "linking libWPEFrameworkL1TestsPE to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} "${ENTSERVICES_REPO_L1TEST_LIB_PATH}/libWPEFrameworkL1TestsPE.so")
endif()

#PLUGIN_SCREENCAPTURE and PLUGIN_SYSTEMAUDIOPLAYER and TEXTTOSPEECH and UNIFIEDCASMANAGEMENT
if(PLUGIN_SCREENCAPTURE OR PLUGIN_SYSTEMAUDIOPLAYER OR PLUGIN_TEXTTOSPEECH OR PLUGIN_UNIFIEDCASMANAGEMENT)
    message(STATUS "linking libWPEFrameworkL1TestsMD to RdkServicesL1Test target")
    target_link_libraries(${PROJECT_NAME} "${ENTSERVICES_REPO_L1TEST_LIB_PATH}/libWPEFrameworkL1TestsMD.so")
endif()

include(GoogleTest)
include_directories(${TEST_INC})

target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins)

target_link_libraries(${PROJECT_NAME} gtest_main gmock gtest ${TEST_LIB})

# Link TestMocklib LAST with --whole-archive to ensure all wrapped functions are available
target_link_libraries(${PROJECT_NAME} -Wl,--whole-archive TestMocklib -Wl,--no-whole-archive)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
        ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/devicesettings
        ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/thunder
        ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/libusb
        )

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
