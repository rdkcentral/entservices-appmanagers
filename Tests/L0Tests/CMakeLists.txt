cmake_minimum_required(VERSION 3.16)

project(appmanagers_l0test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(APPMGR_ENABLE_COVERAGE "Enable coverage flags for appmanagers_l0test" ON)
option(APPMGR_ENABLE_WARNINGS "Enable additional compiler warnings for appmanagers_l0test" ON)

if(NOT DEFINED PREFIX OR "${PREFIX}" STREQUAL "")
    if(DEFINED ENV{PREFIX} AND NOT "$ENV{PREFIX}" STREQUAL "")
        set(PREFIX "$ENV{PREFIX}")
    else()
        set(PREFIX "${CMAKE_SOURCE_DIR}/../../install/usr")
    endif()
endif()
set(PREFIX "${PREFIX}" CACHE STRING "Install prefix to locate Thunder artifacts" FORCE)

message(STATUS "Using PREFIX='${PREFIX}'")

# Detect Thunder/WPEFramework namespace.  Production CMakeLists derive this
# via find_package(WPEFramework) which sets NAMESPACE; here we replicate that
# logic for the standalone test build.
if(NOT DEFINED NAMESPACE OR "${NAMESPACE}" STREQUAL "")
    if(EXISTS "${PREFIX}/include/Thunder")
        set(NAMESPACE "Thunder")
    else()
        set(NAMESPACE "WPEFramework")
    endif()
endif()
set(NAMESPACE "${NAMESPACE}" CACHE STRING "Thunder framework namespace (Thunder or WPEFramework)" FORCE)
string(TOLOWER "${NAMESPACE}" STORAGE_DIRECTORY)
message(STATUS "Using NAMESPACE='${NAMESPACE}', STORAGE_DIRECTORY='${STORAGE_DIRECTORY}'")

# Locate jsoncpp headers (needed by AIConfiguration and DobbySpecGenerator)
find_path(JSONCPP_INCLUDE_DIR NAMES json/json.h
    HINTS /usr/include/jsoncpp /usr/local/include/jsoncpp "${PREFIX}/include/jsoncpp")

set(APPMANAGERS_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/RuntimeManager.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/RuntimeManagerImplementation.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/RuntimeManagerTelemetryReporting.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/DobbySpecGenerator.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/WindowManagerConnector.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/DobbyEventListener.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/UserIdManager.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/AIConfiguration.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/TelemetryReportingBase.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/UtilsTelemetryMetrics.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/ralf/RalfSupport.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/ralf/RalfPackageBuilder.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/ralf/RalfOCIConfigGenerator.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/Gateway/WebInspector.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/Gateway/NetFilter.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/Gateway/NetFilterLock.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/Gateway/ContainerUtils.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/Gateway/NetFilterUtils.c
    # Test sources
    RuntimeManager/RuntimeManagerTest.cpp
    RuntimeManager/RuntimeManager_LifecycleTests.cpp
    RuntimeManager/RuntimeManager_ImplementationTests.cpp
    RuntimeManager/RuntimeManager_ComponentTests.cpp
    RuntimeManager/ralf/Ralf_SupportTests.cpp
    RuntimeManager/ralf/Ralf_PackageBuilderTests.cpp
    RuntimeManager/ralf/Ralf_OCIConfigGeneratorTests.cpp
    RuntimeManager/Gateway/Gateway_ContainerUtilsTests.cpp
    RuntimeManager/Gateway/Gateway_WebInspectorTests.cpp
    RuntimeManager/Gateway/Gateway_NetFilterTests.cpp
    common/L0Bootstrap.cpp
)

add_executable(appmanagers_l0test ${APPMANAGERS_L0_SOURCES})

target_compile_definitions(appmanagers_l0test PRIVATE
    MODULE_NAME=AppManagers_L0Test
    USE_THUNDER_R4=1
    RUNTIME_MANAGER_API_VERSION_NUMBER_MAJOR=1
    RUNTIME_MANAGER_API_VERSION_NUMBER_MINOR=0
    RUNTIME_MANAGER_API_VERSION_NUMBER_PATCH=0
    RDK_APPMANAGERS_DEBUG=1
    RALF_PACKAGE_SUPPORT_ENABLED=1
    PLUGIN_RUNTIME_MANAGER_IMPLEMENTATION_NAME="RuntimeManagerImplementation"
    AI_DEBUG=1
    AI_RELEASE=2
    AI_BUILD_TYPE=AI_DEBUG
    DISABLE_SECURITY_TOKEN=1
    THUNDER_PLATFORM_PC_UNIX=1
)

if(APPMGR_ENABLE_COVERAGE)
    target_compile_options(appmanagers_l0test PRIVATE -O0 -g --coverage)
    target_link_options(appmanagers_l0test PRIVATE --coverage)
endif()

if(APPMGR_ENABLE_WARNINGS)
    target_compile_options(appmanagers_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_include_directories(appmanagers_l0test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/RuntimeManager
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/../..
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/ralf
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/Gateway
    ${CMAKE_SOURCE_DIR}/../../helpers
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry
    ${PREFIX}/include
    ${PREFIX}/include/${NAMESPACE}
)

if(JSONCPP_INCLUDE_DIR)
    target_include_directories(appmanagers_l0test PRIVATE ${JSONCPP_INCLUDE_DIR})
endif()

if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(EXISTS "${_prefix}/include")
            target_include_directories(appmanagers_l0test PRIVATE "${_prefix}/include")
        endif()
        if(EXISTS "${_prefix}/include/${NAMESPACE}")
            target_include_directories(appmanagers_l0test PRIVATE "${_prefix}/include/${NAMESPACE}")
        endif()
    endforeach()
endif()

if(EXISTS "${PREFIX}/lib")
    target_link_directories(appmanagers_l0test PRIVATE "${PREFIX}/lib")
endif()
if(EXISTS "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
    target_link_directories(appmanagers_l0test PRIVATE "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
endif()
if(EXISTS "${PREFIX}/lib/plugins")
    target_link_directories(appmanagers_l0test PRIVATE "${PREFIX}/lib/plugins")
endif()

find_library(THUNDER_CORE_LIB NAMES ${NAMESPACE}Core HINTS "${PREFIX}/lib")
find_library(THUNDER_MESSAGING_LIB NAMES ${NAMESPACE}Messaging HINTS "${PREFIX}/lib")
find_library(THUNDER_PLUGINS_LIB NAMES ${NAMESPACE}Plugins HINTS "${PREFIX}/lib")
find_library(THUNDER_COM_LIB NAMES ${NAMESPACE}COM HINTS "${PREFIX}/lib")
find_library(JSONCPP_LIB NAMES jsoncpp HINTS "${PREFIX}/lib" /usr/lib /usr/local/lib)
find_library(CURL_LIB NAMES curl HINTS "${PREFIX}/lib" /usr/lib /usr/local/lib)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Gateway production sources require libip4tc and libip6tc (from iptables).
find_library(LIBIP4TC_LIB NAMES ip4tc libip4tc HINTS /usr/lib /usr/local/lib)
find_library(LIBIP6TC_LIB NAMES ip6tc libip6tc HINTS /usr/lib /usr/local/lib)

if(THUNDER_CORE_LIB)
    target_link_libraries(appmanagers_l0test PRIVATE ${THUNDER_CORE_LIB})
endif()
if(THUNDER_MESSAGING_LIB)
    target_link_libraries(appmanagers_l0test PRIVATE ${THUNDER_MESSAGING_LIB})
endif()
if(THUNDER_PLUGINS_LIB)
    target_link_libraries(appmanagers_l0test PRIVATE ${THUNDER_PLUGINS_LIB})
endif()
if(THUNDER_COM_LIB)
    target_link_libraries(appmanagers_l0test PRIVATE ${THUNDER_COM_LIB})
endif()

target_link_libraries(appmanagers_l0test PRIVATE
    Threads::Threads
    dl
    m
)

if(JSONCPP_LIB)
    target_link_libraries(appmanagers_l0test PRIVATE ${JSONCPP_LIB})
else()
    target_link_libraries(appmanagers_l0test PRIVATE jsoncpp)
endif()

if(CURL_LIB)
    target_link_libraries(appmanagers_l0test PRIVATE ${CURL_LIB})
else()
    target_link_libraries(appmanagers_l0test PRIVATE curl)
endif()

# Link ip4tc and ip6tc — required by Gateway/NetFilter.cpp and Gateway/NetFilterUtils.c
if(LIBIP4TC_LIB)
    target_link_libraries(appmanagers_l0test PRIVATE ${LIBIP4TC_LIB})
else()
    target_link_libraries(appmanagers_l0test PRIVATE ip4tc)
endif()
if(LIBIP6TC_LIB)
    target_link_libraries(appmanagers_l0test PRIVATE ${LIBIP6TC_LIB})
else()
    target_link_libraries(appmanagers_l0test PRIVATE ip6tc)
endif()

set_target_properties(appmanagers_l0test PROPERTIES
    BUILD_RPATH "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
    INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
)

add_custom_target(run
    DEPENDS appmanagers_l0test
    COMMAND $<TARGET_FILE:appmanagers_l0test>
    COMMENT "Running appmanagers_l0test"
    VERBATIM
)

# 'coverage' target.
# Commands:
#   lcov -c -o coverage.info -d ${CMAKE_BINARY_DIR}
#   genhtml -o coverage coverage.info
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(NOT LCOV_EXECUTABLE OR NOT GENHTML_EXECUTABLE)
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E echo "lcov/genhtml not found. Install lcov to generate coverage report."
        COMMENT "Coverage tools not found; skipping coverage data generation."
        VERBATIM
    )
else()
    add_custom_target(coverage
        DEPENDS appmanagers_l0test
        COMMAND "${LCOV_EXECUTABLE}" -c -o coverage.info -d "${CMAKE_BINARY_DIR}"
        COMMAND "${GENHTML_EXECUTABLE}" -o coverage coverage.info
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/.."
        COMMENT "Generating AppManagers L0 coverage report"
        VERBATIM
    )
endif()
