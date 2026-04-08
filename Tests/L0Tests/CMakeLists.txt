cmake_minimum_required(VERSION 3.16)

project(appmanagers_l0test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PLUGIN_RUNTIME_MANAGER   "Build RuntimeManager L0 test target"    ON)
option(PLUGIN_LIFECYCLE_MANAGER "Build LifecycleManager L0 test target"  ON)

option(APPMGR_ENABLE_COVERAGE "Enable coverage flags for runtimemanager_l0test"      ON)
option(APPLCM_ENABLE_COVERAGE "Enable coverage flags for lifecyclemanager_l0test"  ON)
option(L0_ENABLE_WARNINGS     "Enable additional compiler warnings for L0 targets" ON)

# ---------------------------------------------------------------------------
# Install prefix
# ---------------------------------------------------------------------------
if(NOT DEFINED PREFIX OR "${PREFIX}" STREQUAL "")
    if(DEFINED ENV{PREFIX} AND NOT "$ENV{PREFIX}" STREQUAL "")
        set(PREFIX "$ENV{PREFIX}")
    else()
        set(PREFIX "${CMAKE_SOURCE_DIR}/../../install/usr")
    endif()
endif()
set(PREFIX "${PREFIX}" CACHE STRING "Install prefix to locate Thunder artifacts" FORCE)
message(STATUS "Using PREFIX='${PREFIX}'")

# Detect Thunder/WPEFramework namespace
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

# ---------------------------------------------------------------------------
# Source lists
# ---------------------------------------------------------------------------
set(RUNTIME_MANAGER_L0_SOURCES
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

set(LIFECYCLE_MANAGER_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/LifecycleManager.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/LifecycleManagerImplementation.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/LifecycleManagerTelemetryReporting.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/ApplicationContext.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/StateHandler.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/State.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/RequestHandler.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/StateTransitionHandler.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/RuntimeManagerHandler.cpp
    ${CMAKE_SOURCE_DIR}/../../LifecycleManager/WindowManagerHandler.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/TelemetryReportingBase.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/UtilsTelemetryMetrics.cpp
    # Test sources
    LifecycleManager/LifecycleManagerTest.cpp
    LifecycleManager/LifecycleManager_ShellTests.cpp
    LifecycleManager/LifecycleManager_ImplementationTests.cpp
    LifecycleManager/LifecycleManager_ContextTests.cpp
    common/L0Bootstrap.cpp
)

# ---------------------------------------------------------------------------
# Declare targets
# ---------------------------------------------------------------------------
if(PLUGIN_RUNTIME_MANAGER)
    add_executable(runtimemanager_l0test ${RUNTIME_MANAGER_L0_SOURCES})
else()
    message(STATUS "PLUGIN_RUNTIME_MANAGER=OFF; skipping runtimemanager_l0test target")
endif()

if(PLUGIN_LIFECYCLE_MANAGER)
    add_executable(lifecyclemanager_l0test ${LIFECYCLE_MANAGER_L0_SOURCES})
else()
    message(STATUS "PLUGIN_LIFECYCLE_MANAGER=OFF; skipping lifecyclemanager_l0test target")
endif()

if(NOT PLUGIN_RUNTIME_MANAGER AND NOT PLUGIN_LIFECYCLE_MANAGER)
    message(FATAL_ERROR "No L0 target enabled. Enable at least one PLUGIN_RUNTIME_MANAGER or PLUGIN_LIFECYCLE_MANAGER option.")
endif()

# ---------------------------------------------------------------------------
# RuntimeManager target compile options / definitions / includes
# ---------------------------------------------------------------------------
if(PLUGIN_RUNTIME_MANAGER)
    # Locate jsoncpp headers (needed by AIConfiguration and DobbySpecGenerator)
    find_path(JSONCPP_INCLUDE_DIR NAMES json/json.h
        HINTS /usr/include/jsoncpp /usr/local/include/jsoncpp "${PREFIX}/include/jsoncpp")

    target_compile_definitions(runtimemanager_l0test PRIVATE
        MODULE_NAME=RuntimeManager_L0Test
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
        target_compile_options(runtimemanager_l0test PRIVATE -O0 -g --coverage)
        target_link_options(runtimemanager_l0test PRIVATE --coverage)
    endif()

    if(L0_ENABLE_WARNINGS)
        target_compile_options(runtimemanager_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    target_include_directories(runtimemanager_l0test PRIVATE
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
        target_include_directories(runtimemanager_l0test PRIVATE ${JSONCPP_INCLUDE_DIR})
    endif()

    if(EXISTS "${PREFIX}/lib")
        target_link_directories(runtimemanager_l0test PRIVATE "${PREFIX}/lib")
    endif()
    if(EXISTS "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
        target_link_directories(runtimemanager_l0test PRIVATE "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
    endif()
    if(EXISTS "${PREFIX}/lib/plugins")
        target_link_directories(runtimemanager_l0test PRIVATE "${PREFIX}/lib/plugins")
    endif()
endif()

# ---------------------------------------------------------------------------
# LifecycleManager target compile options / definitions / includes
# ---------------------------------------------------------------------------
if(PLUGIN_LIFECYCLE_MANAGER)
    target_compile_definitions(lifecyclemanager_l0test PRIVATE
        MODULE_NAME=LifecycleManager_L0Test
        USE_THUNDER_R4=1
        LIFECYCLE_MANAGER_API_VERSION_NUMBER_MAJOR=1
        LIFECYCLE_MANAGER_API_VERSION_NUMBER_MINOR=0
        LIFECYCLE_MANAGER_API_VERSION_NUMBER_PATCH=0
        DISABLE_SECURITY_TOKEN=1
        THUNDER_PLATFORM_PC_UNIX=1
        UNIT_TEST=1
    )

    if(APPLCM_ENABLE_COVERAGE)
        target_compile_options(lifecyclemanager_l0test PRIVATE -O0 -g --coverage)
        target_link_options(lifecyclemanager_l0test PRIVATE --coverage)
    endif()

    if(L0_ENABLE_WARNINGS)
        target_compile_options(lifecyclemanager_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    target_include_directories(lifecyclemanager_l0test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/LifecycleManager
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}/../../LifecycleManager
        ${CMAKE_SOURCE_DIR}/../../helpers
        ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry
    )

    if(EXISTS "${PREFIX}/lib")
        target_link_directories(lifecyclemanager_l0test PRIVATE "${PREFIX}/lib")
    endif()
    if(EXISTS "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
        target_link_directories(lifecyclemanager_l0test PRIVATE "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
    endif()
    if(EXISTS "${PREFIX}/lib/plugins")
        target_link_directories(lifecyclemanager_l0test PRIVATE "${PREFIX}/lib/plugins")
    endif()
endif()

# ---------------------------------------------------------------------------
# Thunder / WPEFramework library discovery (shared for both targets)
# ---------------------------------------------------------------------------
find_library(THUNDER_CORE_LIB      NAMES ${NAMESPACE}Core      HINTS "${PREFIX}/lib")
find_library(THUNDER_MESSAGING_LIB NAMES ${NAMESPACE}Messaging HINTS "${PREFIX}/lib")
find_library(THUNDER_PLUGINS_LIB   NAMES ${NAMESPACE}Plugins   HINTS "${PREFIX}/lib")
find_library(THUNDER_COM_LIB       NAMES ${NAMESPACE}COM       HINTS "${PREFIX}/lib")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Fallback include paths for environments without namespace package configs.
if(PLUGIN_RUNTIME_MANAGER)
    target_include_directories(runtimemanager_l0test PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
endif()
if(PLUGIN_LIFECYCLE_MANAGER)
    target_include_directories(lifecyclemanager_l0test PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
endif()

if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(EXISTS "${_prefix}/include")
            if(PLUGIN_RUNTIME_MANAGER)
                target_include_directories(runtimemanager_l0test PRIVATE "${_prefix}/include")
            endif()
            if(PLUGIN_LIFECYCLE_MANAGER)
                target_include_directories(lifecyclemanager_l0test PRIVATE "${_prefix}/include")
            endif()
        endif()
        if(EXISTS "${_prefix}/include/${NAMESPACE}")
            if(PLUGIN_RUNTIME_MANAGER)
                target_include_directories(runtimemanager_l0test PRIVATE "${_prefix}/include/${NAMESPACE}")
            endif()
            if(PLUGIN_LIFECYCLE_MANAGER)
                target_include_directories(lifecyclemanager_l0test PRIVATE "${_prefix}/include/${NAMESPACE}")
            endif()
        endif()
    endforeach()
endif()

if(PLUGIN_RUNTIME_MANAGER)
    if(DEFINED THUNDER_CORE_LIB AND THUNDER_CORE_LIB)
        target_link_libraries(runtimemanager_l0test PRIVATE ${THUNDER_CORE_LIB})
    endif()
    if(DEFINED THUNDER_MESSAGING_LIB AND THUNDER_MESSAGING_LIB)
        target_link_libraries(runtimemanager_l0test PRIVATE ${THUNDER_MESSAGING_LIB})
    endif()
    if(DEFINED THUNDER_PLUGINS_LIB AND THUNDER_PLUGINS_LIB)
        target_link_libraries(runtimemanager_l0test PRIVATE ${THUNDER_PLUGINS_LIB})
    endif()
    if(DEFINED THUNDER_COM_LIB AND THUNDER_COM_LIB)
        target_link_libraries(runtimemanager_l0test PRIVATE ${THUNDER_COM_LIB})
    endif()

    target_link_libraries(runtimemanager_l0test PRIVATE
        Threads::Threads
        m
        dl
    )

    find_library(JSONCPP_LIB  NAMES jsoncpp HINTS "${PREFIX}/lib" /usr/lib /usr/local/lib)
    find_library(CURL_LIB     NAMES curl    HINTS "${PREFIX}/lib" /usr/lib /usr/local/lib)
    # Gateway sources require libip4tc and libip6tc (iptables)
    find_library(LIBIP4TC_LIB NAMES ip4tc libip4tc HINTS /usr/lib /usr/local/lib)
    find_library(LIBIP6TC_LIB NAMES ip6tc libip6tc HINTS /usr/lib /usr/local/lib)

    if(JSONCPP_LIB)
        target_link_libraries(runtimemanager_l0test PRIVATE ${JSONCPP_LIB})
    else()
        target_link_libraries(runtimemanager_l0test PRIVATE jsoncpp)
    endif()
    if(CURL_LIB)
        target_link_libraries(runtimemanager_l0test PRIVATE ${CURL_LIB})
    else()
        target_link_libraries(runtimemanager_l0test PRIVATE curl)
    endif()
    if(LIBIP4TC_LIB)
        target_link_libraries(runtimemanager_l0test PRIVATE ${LIBIP4TC_LIB})
    else()
        target_link_libraries(runtimemanager_l0test PRIVATE ip4tc)
    endif()
    if(LIBIP6TC_LIB)
        target_link_libraries(runtimemanager_l0test PRIVATE ${LIBIP6TC_LIB})
    else()
        target_link_libraries(runtimemanager_l0test PRIVATE ip6tc)
    endif()
endif()

if(PLUGIN_LIFECYCLE_MANAGER)
    if(DEFINED THUNDER_CORE_LIB AND THUNDER_CORE_LIB)
        target_link_libraries(lifecyclemanager_l0test PRIVATE ${THUNDER_CORE_LIB})
    endif()
    if(DEFINED THUNDER_MESSAGING_LIB AND THUNDER_MESSAGING_LIB)
        target_link_libraries(lifecyclemanager_l0test PRIVATE ${THUNDER_MESSAGING_LIB})
    endif()
    if(DEFINED THUNDER_PLUGINS_LIB AND THUNDER_PLUGINS_LIB)
        target_link_libraries(lifecyclemanager_l0test PRIVATE ${THUNDER_PLUGINS_LIB})
    endif()
    if(DEFINED THUNDER_COM_LIB AND THUNDER_COM_LIB)
        target_link_libraries(lifecyclemanager_l0test PRIVATE ${THUNDER_COM_LIB})
    endif()

    target_link_libraries(lifecyclemanager_l0test PRIVATE
        Threads::Threads
        m
        dl
    )
endif()

# ---------------------------------------------------------------------------
# Runtime search paths (rpath — tells the OS where to find .so files at runtime)
# ---------------------------------------------------------------------------
if(PLUGIN_RUNTIME_MANAGER)
    set_target_properties(runtimemanager_l0test PROPERTIES
        BUILD_RPATH   "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
        INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
    )
endif()

if(PLUGIN_LIFECYCLE_MANAGER)
    set_target_properties(lifecyclemanager_l0test PROPERTIES
        BUILD_RPATH   "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
        INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
    )
endif()

# ---------------------------------------------------------------------------
# Utility targets
# ---------------------------------------------------------------------------
if(PLUGIN_RUNTIME_MANAGER)
    add_custom_target(run
        DEPENDS runtimemanager_l0test
        COMMAND $<TARGET_FILE:runtimemanager_l0test>
        COMMENT "Running runtimemanager_l0test"
        VERBATIM
    )
endif()

if(PLUGIN_LIFECYCLE_MANAGER)
    add_custom_target(run_lifecyclemanager
        DEPENDS lifecyclemanager_l0test
        COMMAND $<TARGET_FILE:lifecyclemanager_l0test>
        COMMENT "Running lifecyclemanager_l0test"
        VERBATIM
    )
endif()

find_program(LCOV_EXECUTABLE    lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(NOT LCOV_EXECUTABLE OR NOT GENHTML_EXECUTABLE)
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E echo "lcov/genhtml not found. Install lcov to generate coverage report."
        COMMENT "Coverage tools not found; skipping coverage data generation."
        VERBATIM
    )
else()
    set(COVERAGE_DEPENDS "")
    if(PLUGIN_RUNTIME_MANAGER)
        list(APPEND COVERAGE_DEPENDS runtimemanager_l0test)
    endif()
    if(PLUGIN_LIFECYCLE_MANAGER)
        list(APPEND COVERAGE_DEPENDS lifecyclemanager_l0test)
    endif()

    add_custom_target(coverage
        DEPENDS ${COVERAGE_DEPENDS}
        COMMAND "${LCOV_EXECUTABLE}" -c -o coverage.info -d "${CMAKE_BINARY_DIR}"
        COMMAND "${GENHTML_EXECUTABLE}" -o coverage coverage.info
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/.."
        COMMENT "Generating coverage report with lcov/genhtml"
        VERBATIM
    )
endif()
