# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
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


set(PLUGIN_NAME L2Tests)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(THUNDER_PORT 9998)


find_package(${NAMESPACE}Plugins REQUIRED)
find_package(GTest REQUIRED)

set(SRC_FILES
    tests/Module.cpp
    tests/L2Tests.cpp
    tests/L2TestsMock.cpp)

# NOTE: AppManager and AppStorageManager test files are compiled exclusively
# into WPEFrameworkL2TestsIN (Part 2) to avoid duplicate symbols when
# WPEFrameworkL2Tests links against WPEFrameworkL2TestsIN.

add_library(${MODULE_NAME} SHARED ${SRC_FILES})


set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME}
            PRIVATE
            MODULE_NAME=Plugin_${PLUGIN_NAME}
            THUNDER_PORT="${THUNDER_PORT}")

target_compile_options(${MODULE_NAME} PRIVATE -Wno-error)

# Link group-specific L2 test library bundles from sibling entservices repos
if(PLUGIN_DEVICEDIAGNOSTICS OR PLUGIN_USERPREFERENCES OR PLUGIN_WAREHOUSE OR PLUGIN_DISPLAYSETTINGS OR PLUGIN_POWERMANAGER OR PLUGIN_SYSTEMSERVICES OR PLUGIN_DEVICEINFO)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsDD.so)
endif()

if(PLUGIN_PRIVACY OR PLUGIN_DEVICEPROVISIONING OR PLUGIN_AUTHSERVICE)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsCPC.so)
endif()

set(LINK_L2TESTS_IN OFF)
if(PLUGIN_ANALYTICS OR PLUGIN_APPMANAGER OR PLUGIN_TELEMETRY OR PLUGIN_USBDEVICE OR PLUGIN_USB_MASS_STORAGE OR PLUGIN_USERSETTINGS OR PLUGIN_MIGRATION OR PLUGIN_SHAREDSTORAGE OR PLUGIN_OCICONTAINER OR PLUGIN_PERSISTENTSTORE OR PLUGIN_CLOUDSTORE OR PLUGIN_APP_STORAGE_MANAGER)
    set(LINK_L2TESTS_IN ON)
endif()

if(PLUGIN_FIRMWAREUPDATE OR PLUGIN_MAINTENANCEMANAGER)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsSU.so)
endif()

if(PLUGIN_AVOUTPUT OR PLUGIN_HDMICECSINK OR PLUGIN_AVINPUT OR PLUGIN_HDCPPROFILE OR PLUGIN_HDMICECSOURCE)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsIO.so)
endif()

if(PLUGIN_TEXTTOSPEECH OR PLUGIN_SCREENCAPTURE)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsMD.so)
endif()

if(PLUGIN_LEDCONTROL)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsPE.so)
endif()

if(PLUGIN_BLUETOOTH)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsBT.so)
endif()

if(CT_VERIFIER_TESTS)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsCT.so)
endif()

if (NOT L2_TEST_OOP_RPC)
    find_library(TESTMOCKLIB_LIBRARIES NAMES TestMocklib)
    if (TESTMOCKLIB_LIBRARIES)
        message ("Found mock library - ${TESTMOCKLIB_LIBRARIES}")
        target_link_libraries(${MODULE_NAME} PRIVATE ${TESTMOCKLIB_LIBRARIES})
    else (TESTMOCKLIB_LIBRARIES)
         message ("Require ${TESTMOCKLIB_LIBRARIES} library")
    endif (TESTMOCKLIB_LIBRARIES)
endif (NOT L2_TEST_OOP_RPC)

find_library(MOCKACCESSOR_LIBRARIES NAMES MockAccessor)
if (MOCKACCESSOR_LIBRARIES)
    message ("Found MockAccessor library - ${MOCKACCESSOR_LIBRARIES}")
    target_link_libraries(${MODULE_NAME} PRIVATE ${MOCKACCESSOR_LIBRARIES})
else (MOCKACCESSOR_LIBRARIES)
     message ("Require ${MOCKACCESSOR_LIBRARIES} library")
endif (MOCKACCESSOR_LIBRARIES)

target_link_directories(${MODULE_NAME} PUBLIC
    ${CMAKE_INSTALL_PREFIX}/lib
    ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins)
target_link_libraries(${MODULE_NAME} PRIVATE
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    gtest_main gmock gtest)

set(ENTSERVICES_INC
    ../../../entservices-deviceanddisplay/helpers
    ../../../entservices-infra/helpers
    ../../../entservices-mediaanddrm/helpers
    ../../../entservices-softwareupdate/helpers
    ../../../entservices-inputoutput/helpers
    ../../../entservices-peripherals/helpers
    ../../../entservices-connectivity/helpers)

target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../helpers
    ../headers
    tests/
    ../mocks
    ../mocks/thunder
    ../mocks/devicesettings
    ../../Network
    ../mocks/MockPlugin
    ../mocks/MockAuthService
    ../mocks/MockSecManager
    ${ENTSERVICES_INC}
    ${CMAKE_INSTALL_PREFIX}/include
    ${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE})

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})

# =============================================================================
# Part 2: L2TestsIN - Appmanagers-specific L2 test library
#         (linked into the main L2Tests plugin at runtime)
# =============================================================================

set(PLUGIN_NAME_IN L2TestsIN)
set(MODULE_NAME_IN ${NAMESPACE}${PLUGIN_NAME_IN})

unset(SRC_FILES_IN)

if(PLUGIN_APPMANAGER)
    set(SRC_FILES_IN ${SRC_FILES_IN} tests/AppManager_L2Test.cpp)
endif()

if(PLUGIN_APP_STORAGE_MANAGER)
    set(SRC_FILES_IN ${SRC_FILES_IN} tests/AppStorageManager_L2Test.cpp)
endif()

if(SRC_FILES_IN)
    add_library(${MODULE_NAME_IN} SHARED ${SRC_FILES_IN})

    set_target_properties(${MODULE_NAME_IN} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED YES)

    target_compile_definitions(${MODULE_NAME_IN}
                PRIVATE
                MODULE_NAME=Plugin_${PLUGIN_NAME_IN}
                THUNDER_PORT="${THUNDER_PORT}")

    target_compile_options(${MODULE_NAME_IN} PRIVATE -Wno-error)
    target_link_directories(${MODULE_NAME_IN} PRIVATE
        ${CMAKE_INSTALL_PREFIX}/lib)
    target_link_libraries(${MODULE_NAME_IN} PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        gtest gmock)

    if (NOT L2_TEST_OOP_RPC)
        if (TESTMOCKLIB_LIBRARIES)
            target_link_libraries(${MODULE_NAME_IN} PRIVATE ${TESTMOCKLIB_LIBRARIES})
        endif (TESTMOCKLIB_LIBRARIES)
    endif (NOT L2_TEST_OOP_RPC)

    if (MOCKACCESSOR_LIBRARIES)
        target_link_libraries(${MODULE_NAME_IN} PRIVATE ${MOCKACCESSOR_LIBRARIES})
    endif (MOCKACCESSOR_LIBRARIES)

    target_include_directories(${MODULE_NAME_IN} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ../headers
        tests/
        ../../helpers
        ../mocks
        ../mocks/thunder
        ../mocks/devicesettings
        ../mocks/MockPlugin
        ${CMAKE_INSTALL_PREFIX}/include
        ${CMAKE_INSTALL_PREFIX}/include/WPEFramework
        ${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE})

    install(TARGETS ${MODULE_NAME_IN} DESTINATION lib)

    # Link WPEFrameworkL2Tests against the IN library as a proper CMake target
    # (not an install-path string) so the build order is enforced automatically.
    if(LINK_L2TESTS_IN)
        target_link_libraries(${MODULE_NAME} PRIVATE ${MODULE_NAME_IN})
        add_dependencies(${MODULE_NAME} ${MODULE_NAME_IN})
    endif()
endif()

# =============================================================================
# Part 3: RdkServicesL2Test - Out-of-process test runner executable
#         Spawns WPEFramework and invokes the L2Tests plugin via JSON-RPC.
#         Timeout matches the reference framework (ref/entservices-testframework).
# =============================================================================
set(TEST_RUNNER_NAME RdkServicesL2Test)

if(NOT TEST_COMPLETION_TIMEOUT)
    set(TEST_COMPLETION_TIMEOUT 900000)
endif()

add_executable(${TEST_RUNNER_NAME}
        tests/L2testController.cpp
        tests/L2testController.h
        tests/Module.cpp
        )

set_source_files_properties(
    tests/L2testController.cpp
    tests/L2testController.h
    tests/Module.cpp
PROPERTIES COMPILE_FLAGS "-fexceptions")

target_link_libraries(${TEST_RUNNER_NAME}
        ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkCore.so
        ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkCOM.so
        ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkPlugins.so
        ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkMessaging.so
        )

if (USE_THUNDER_R4)
target_link_libraries(${TEST_RUNNER_NAME} ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkWebSocket.so)
else ()
target_link_libraries(${TEST_RUNNER_NAME} ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkProtocols.so)
target_link_libraries(${TEST_RUNNER_NAME} ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkTracing.so)
endif (USE_THUNDER_R4)

target_compile_definitions(${TEST_RUNNER_NAME}
                             PRIVATE
                             CMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
                             THUNDER_PORT="${THUNDER_PORT}"
                             TEST_COMPLETION_TIMEOUT=${TEST_COMPLETION_TIMEOUT}
                           )

target_include_directories(${TEST_RUNNER_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tests>
    ../headers
        ${CMAKE_INSTALL_PREFIX}/include/WPEFramework
        ${CMAKE_INSTALL_PREFIX}/include
        )

install(TARGETS ${TEST_RUNNER_NAME} DESTINATION bin)
