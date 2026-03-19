cmake_minimum_required(VERSION 3.16)

project(appmanager_l0test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(APPMGR_ENABLE_COVERAGE "Enable coverage flags for appmanager_l0test" ON)
option(APPMGR_ENABLE_WARNINGS "Enable additional compiler warnings for appmanager_l0test" ON)

# ─── Locate Thunder install prefix ───────────────────────────────────────────
# Priority: -DPREFIX=<path>  >  $PREFIX env var  >  ../../install/usr
if(NOT DEFINED PREFIX OR "${PREFIX}" STREQUAL "")
    if(DEFINED ENV{PREFIX} AND NOT "$ENV{PREFIX}" STREQUAL "")
        set(PREFIX "$ENV{PREFIX}")
    else()
        set(PREFIX "${CMAKE_SOURCE_DIR}/../../../../../install/usr")
    endif()
endif()
set(PREFIX "${PREFIX}" CACHE STRING "Install prefix to locate Thunder artifacts" FORCE)

message(STATUS "appmanager_l0test: Using PREFIX='${PREFIX}'")

# ─── Source files ─────────────────────────────────────────────────────────────
# CMAKE_SOURCE_DIR = Tests/L0Tests/AppManager/
# Plugin root      = Tests/L0Tests/AppManager/../../../  = <repo root>/
# common           = Tests/L0Tests/AppManager/../common/ = Tests/L0Tests/common/

set(APPMANAGER_L0_SOURCES
    # ── Production sources (compiled directly, no shared lib) ────────────────
    ${CMAKE_SOURCE_DIR}/../../../AppManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../../AppManager/AppManager.cpp
    ${CMAKE_SOURCE_DIR}/../../../AppManager/AppManagerImplementation.cpp
    ${CMAKE_SOURCE_DIR}/../../../AppManager/LifecycleInterfaceConnector.cpp
    # ── Test framework ────────────────────────────────────────────────────────
    ${CMAKE_SOURCE_DIR}/../common/L0Bootstrap.cpp
    # ── Test sources ──────────────────────────────────────────────────────────
    ${CMAKE_SOURCE_DIR}/AppManagerTest.cpp
    ${CMAKE_SOURCE_DIR}/AppManager_LifecycleTests.cpp
    ${CMAKE_SOURCE_DIR}/AppManager_ImplementationTests.cpp
    ${CMAKE_SOURCE_DIR}/AppManager_ComponentTests.cpp
)

add_executable(appmanager_l0test ${APPMANAGER_L0_SOURCES})

# ─── Compile definitions ──────────────────────────────────────────────────────
target_compile_definitions(appmanager_l0test PRIVATE
    MODULE_NAME=AppManager_L0Test
    USE_THUNDER_R4=1
    APP_MANAGER_API_VERSION_NUMBER_MAJOR=1
    APP_MANAGER_API_VERSION_NUMBER_MINOR=0
    APP_MANAGER_API_VERSION_NUMBER_PATCH=0
    DISABLE_SECURITY_TOKEN=1
    THUNDER_PLATFORM_PC_UNIX=1
    RDK_APPMANAGERS_DEBUG=1
)

# ─── Coverage & warning flags ─────────────────────────────────────────────────
if(APPMGR_ENABLE_COVERAGE)
    target_compile_options(appmanager_l0test PRIVATE -O0 -g --coverage)
    target_link_options(appmanager_l0test PRIVATE --coverage)
endif()

if(APPMGR_ENABLE_WARNINGS)
    target_compile_options(appmanager_l0test PRIVATE
        -Wall -Wextra -Wno-unused-parameter)
endif()

# ─── Include directories ──────────────────────────────────────────────────────
target_include_directories(appmanager_l0test PRIVATE
    # Test source dir (for ServiceMock.h etc.)
    ${CMAKE_SOURCE_DIR}
    # Common test framework headers
    ${CMAKE_SOURCE_DIR}/../
    ${CMAKE_SOURCE_DIR}/../common
    # Plugin source headers
    ${CMAKE_SOURCE_DIR}/../../../AppManager
    # Helper utilities
    ${CMAKE_SOURCE_DIR}/../../../helpers
    # Thunder install headers
    ${PREFIX}/include
    ${PREFIX}/include/WPEFramework
)

# Honour additional CMAKE_PREFIX_PATH entries (e.g. when Thunder is installed
# to a non-standard location via -DCMAKE_PREFIX_PATH=...).
if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(EXISTS "${_prefix}/include")
            target_include_directories(appmanager_l0test PRIVATE "${_prefix}/include")
        endif()
        if(EXISTS "${_prefix}/include/WPEFramework")
            target_include_directories(appmanager_l0test PRIVATE "${_prefix}/include/WPEFramework")
        endif()
    endforeach()
endif()

# ─── Link directories ─────────────────────────────────────────────────────────
if(EXISTS "${PREFIX}/lib")
    target_link_directories(appmanager_l0test PRIVATE "${PREFIX}/lib")
endif()
if(EXISTS "${PREFIX}/lib/wpeframework/plugins")
    target_link_directories(appmanager_l0test PRIVATE "${PREFIX}/lib/wpeframework/plugins")
endif()

# ─── Find and link Thunder libraries ─────────────────────────────────────────
find_library(THUNDER_CORE_LIB NAMES WPEFrameworkCore ThunderCore
    HINTS "${PREFIX}/lib")
find_library(THUNDER_MESSAGING_LIB NAMES WPEFrameworkMessaging ThunderMessaging
    HINTS "${PREFIX}/lib")
find_library(THUNDER_PLUGINS_LIB NAMES WPEFrameworkPlugins ThunderPlugins
    HINTS "${PREFIX}/lib")
find_library(THUNDER_COM_LIB NAMES WPEFrameworkCOM ThunderCOM
    HINTS "${PREFIX}/lib")

find_package(Threads REQUIRED)

if(THUNDER_CORE_LIB)
    target_link_libraries(appmanager_l0test PRIVATE ${THUNDER_CORE_LIB})
endif()
if(THUNDER_MESSAGING_LIB)
    target_link_libraries(appmanager_l0test PRIVATE ${THUNDER_MESSAGING_LIB})
endif()
if(THUNDER_PLUGINS_LIB)
    target_link_libraries(appmanager_l0test PRIVATE ${THUNDER_PLUGINS_LIB})
endif()
if(THUNDER_COM_LIB)
    target_link_libraries(appmanager_l0test PRIVATE ${THUNDER_COM_LIB})
endif()

target_link_libraries(appmanager_l0test PRIVATE
    Threads::Threads
    dl
    m
)

# ─── RPATH for test binary deployment ─────────────────────────────────────────
set_target_properties(appmanager_l0test PROPERTIES
    BUILD_RPATH  "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/wpeframework;${PREFIX}/lib/wpeframework/plugins"
    INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/wpeframework;${PREFIX}/lib/wpeframework/plugins"
)

# ─── Convenience run target ───────────────────────────────────────────────────
add_custom_target(run
    DEPENDS appmanager_l0test
    COMMAND $<TARGET_FILE:appmanager_l0test>
    COMMENT "Running appmanager_l0test"
    VERBATIM
)

# ─── Coverage report target ───────────────────────────────────────────────────
# Usage after building and running:
#   lcov -c -o coverage.info -d <build_dir>
#   genhtml -o coverage_html coverage.info
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    add_custom_target(coverage
        COMMAND ${LCOV_EXECUTABLE} -c -o coverage.info -d ${CMAKE_BINARY_DIR}
                --no-external
        COMMAND ${GENHTML_EXECUTABLE} -o coverage_html coverage.info
        DEPENDS appmanager_l0test
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating AppManager L0 coverage report"
        VERBATIM
    )
else()
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E echo
            "lcov/genhtml not found. Install lcov to generate coverage report."
        COMMENT "lcov/genhtml unavailable"
    )
endif()
