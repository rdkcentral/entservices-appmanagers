###
# Copyright 2024 RDK Management
# Licensed under the Apache License, Version 2.0
###

cmake_minimum_required(VERSION 3.8)
project(RdkServicesL1Test)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================================
# Dependencies
# ================================

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(GTest REQUIRED)

# ================================
# Base Test Sources
# ================================

set(TEST_SRC
    test_JSON.cpp
)

set(TEST_LIB
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

set(TEST_INC
    ${CMAKE_SOURCE_DIR}/helpers
)

# ================================
# Plugin Test Helper Macros
# ================================

macro(add_plugin_test_ex plugin_opt plugin_test_sources_str plugin_includes_str plugin_libs_str)

    if(${plugin_opt})
        message(STATUS "${plugin_opt}=ON")

        foreach(item ${plugin_test_sources_str})
            list(APPEND TEST_SRC ${item})
        endforeach()

        foreach(item ${plugin_includes_str})
            list(APPEND TEST_INC ${item})
        endforeach()

        foreach(item ${plugin_libs_str})
            list(APPEND TEST_LIB ${item})
        endforeach()
    else()
        message(STATUS "${plugin_opt}=OFF")
    endif()

endmacro()

macro(add_plugin_test plugin_name test_files)

    string(TOUPPER "${plugin_name}" plugin_option)
    set(plugin_opt "PLUGIN_${plugin_option}")

    add_plugin_test_ex(
        ${plugin_opt}
        "${test_files}"
        "../../${plugin_name}"
        "${NAMESPACE}${plugin_name}"
    )

endmacro()

# ================================
# Executable
# ================================

add_executable(${PROJECT_NAME} ${TEST_SRC})

# ================================
# Always Link Core Mock Library
# ================================

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        TestMocklib
)

# ================================
# Conditional Mock Libraries (TARGET-BASED LINKING)
# ================================

if(PLUGIN_DEVICEDIAGNOSTICS OR PLUGIN_USERPREFERENCES OR
   PLUGIN_WAREHOUSE OR PLUGIN_DEVICEINFO OR
   PLUGIN_DISPLAYINFO OR PLUGIN_DISPLAYSETTINGS OR
   PLUGIN_FRAMERATE OR PLUGIN_POWERMANAGER OR
   PLUGIN_SYSTEMSERVICES)

    target_link_libraries(${PROJECT_NAME} PRIVATE WPEFrameworkL1TestsDD)
endif()

if(PLUGIN_PRIVACY OR PLUGIN_PLAYBACKSYNC OR PLUGIN_AUTHSERVICE)
    target_link_libraries(${PROJECT_NAME} PRIVATE WPEFrameworkL1TestsCPC)
endif()

if(PLUGIN_USBDEVICE OR PLUGIN_USB_MASS_STORAGE OR
   PLUGIN_OCICONTAINER OR PLUGIN_APPMANAGER OR
   PLUGIN_STORAGE_MANAGER)

    target_link_libraries(${PROJECT_NAME} PRIVATE WPEFrameworkL1TestsIN)
endif()

if(PLUGIN_FIRMWAREUPDATE OR PLUGIN_MAINTENANCEMANAGER OR PLUGIN_PACKAGER)
    target_link_libraries(${PROJECT_NAME} PRIVATE WPEFrameworkL1TestsSU)
endif()

if(PLUGIN_AVINPUT OR PLUGIN_AVOUTPUT OR
   PLUGIN_HDMICECSOURCE OR PLUGIN_HDMICECSINK OR
   PLUGIN_HDCPPROFILE OR PLUGIN_HDMIINPUT)

    target_link_libraries(${PROJECT_NAME} PRIVATE WPEFrameworkL1TestsIO)
endif()

if(PLUGIN_MIRACAST OR PLUGIN_XCAST)
    target_link_libraries(${PROJECT_NAME} PRIVATE WPEFrameworkL1TestsCA)
endif()

if(PLUGIN_MOTION_DETECTION OR PLUGIN_FRONTPANEL)
    target_link_libraries(${PROJECT_NAME} PRIVATE WPEFrameworkL1TestsPE)
endif()

if(PLUGIN_SCREENCAPTURE OR PLUGIN_SYSTEMAUDIOPLAYER OR
   PLUGIN_TEXTTOSPEECH OR PLUGIN_UNIFIEDCASMANAGEMENT)

    target_link_libraries(${PROJECT_NAME} PRIVATE WPEFrameworkL1TestsMD)
endif()

# ================================
# GoogleTest Integration
# ================================

include(GoogleTest)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        gtest
        gtest_main
        gmock
        ${TEST_LIB}
)

# ================================
# Include Directories
# ================================

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${TEST_INC}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Tests/mocks
        ${CMAKE_SOURCE_DIR}/Tests/mocks/devicesettings
        ${CMAKE_SOURCE_DIR}/Tests/mocks/thunder
        ${CMAKE_SOURCE_DIR}/Tests/mocks/libusb
        ${CMAKE_SOURCE_DIR}/OCIContainer/stubs
        ${CMAKE_SOURCE_DIR}/../Thunder/Source/plugins
)

# ================================
# Install
# ================================

install(TARGETS ${PROJECT_NAME} DESTINATION bin)