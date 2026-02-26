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

set(SRC_FILES Module.cpp L2Tests.cpp L2TestsMock.cpp)

if(PLUGIN_SAMPLEPLUGIN)
    set(SRC_FILES ${SRC_FILES} tests/SamplePlugin_L2test.cpp)
endif()

add_library(${MODULE_NAME} SHARED ${SRC_FILES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME}
            PRIVATE
            MODULE_NAME=Plugin_${PLUGIN_NAME}
            THUNDER_PORT="${THUNDER_PORT}")

target_compile_options(${MODULE_NAME} PRIVATE -Wno-error)

if(PLUGIN_DEVICEDIAGNOSTICS OR PLUGIN_USERPREFERENCES OR PLUGIN_WAREHOUSE OR PLUGIN_DISPLAYSETTINGS OR PLUGIN_POWERMANAGER OR PLUGIN_SYSTEMSERVICES OR PLUGIN_DEVICEINFO)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsDD.so)
endif()

if(PLUGIN_PRIVACY OR PLUGIN_DEVICEPROVISIONING OR PLUGIN_AUTHSERVICE)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsCPC.so)
endif()

if(PLUGIN_ANALYTICS OR PLUGIN_APPMANAGER OR PLUGIN_TELEMETRY OR PLUGIN_USBDEVICE OR PLUGIN_USB_MASS_STORAGE OR PLUGIN_USERSETTINGS)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsIN.so)
endif()

if(PLUGIN_FIRMWAREUPDATE OR PLUGIN_MAINTENANCEMANAGER)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsSU.so)
endif()

if(PLUGIN_AVOUTPUT OR PLUGIN_HDMICECSINK OR PLUGIN_AVINPUT)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsIO.so)
endif()

if(PLUGIN_TEXTTOSPEECH OR PLUGIN_SCREENCAPTURE)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsMD.so)
endif()

if(PLUGIN_LEDCONTROL)
    target_link_libraries(${MODULE_NAME} PRIVATE ${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkL2TestsPE.so)
endif()

#Added for Contract Verifier tests
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

target_link_directories(${MODULE_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins)
target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins gtest_main gmock gtest)

set(ENTSERVICES_INC ../../../../entservices-deviceanddisplay/helpers ../../../../entservices-infra/helpers ../../../../entservices-mediaanddrm/helpers ../../../../entservices-softwareupdate/helpers ../../../../entservices-inputoutput/helpers ../../../../entservices-peripherals/helpers)

target_include_directories(${MODULE_NAME} PRIVATE ../../../helpers)
target_include_directories(${MODULE_NAME} PRIVATE ./)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks/thunder)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks/devicesettings)
target_include_directories(${MODULE_NAME} PRIVATE ../../../Network)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks/MockPlugin)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks/MockAuthService)
target_include_directories(${MODULE_NAME} PRIVATE ../../mocks/MockSecManager)
target_include_directories(${MODULE_NAME} PRIVATE ${ENTSERVICES_INC})
 
install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
