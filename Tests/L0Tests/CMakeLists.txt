cmake_minimum_required(VERSION 3.16)

project(appmanagers_l0test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PLUGIN_RUNTIME_MANAGER    "Build RuntimeManager L0 test target"    ON)
option(PLUGIN_LIFECYCLE_MANAGER  "Build LifecycleManager L0 test target"  ON)
option(PLUGIN_RDK_WINDOW_MANAGER "Build RDKWindowManager L0 test target"  ON)
option(PLUGIN_APP_STORAGE_MANAGER "Build AppStorageManager L0 test target" ON)
option(PLUGIN_PACKAGE_MANAGER    "Build PackageManager L0 test target"    ON)
option(PLUGIN_PREINSTALL_MANAGER "Build PreinstallManager L0 test target" ON)

option(APPMGR_ENABLE_COVERAGE  "Enable coverage flags for runtimemanager_l0test"    ON)
option(APPLCM_ENABLE_COVERAGE  "Enable coverage flags for lifecyclemanager_l0test"  ON)
option(APPWM_ENABLE_COVERAGE   "Enable coverage flags for rdkwindowmanager_l0test"  ON)
option(APPASM_ENABLE_COVERAGE  "Enable coverage flags for appstorage_l0test"       ON)
option(APPPKG_ENABLE_COVERAGE  "Enable coverage flags for packagemanager_l0test"    ON)
option(APPMGR_PREINSTALL_ENABLE_COVERAGE  "Enable coverage flags for preinstall_manager_l0test"  ON)
option(L0_ENABLE_WARNINGS      "Enable additional compiler warnings for L0 targets" ON)

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

set(RDKWINDOWMANAGER_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../RDKWindowManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../RDKWindowManager/RDKWindowManager.cpp
    ${CMAKE_SOURCE_DIR}/../../RDKWindowManager/RDKWindowManagerImplementation.cpp
    ${CMAKE_SOURCE_DIR}/../../RDKWindowManager/RDKWindowManagerTelemetryReporting.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/TelemetryReportingBase.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/UtilsTelemetryMetrics.cpp
    RDKWindowManager/RDKWindowManagerTest.cpp
    RDKWindowManager/RDKWindowManager_LifecycleTests.cpp
    RDKWindowManager/RDKWindowManager_ImplementationTests.cpp
    RDKWindowManager/fakes/FakeRdkWindowManager.cpp
    common/L0Bootstrap.cpp
)

set(APPSTORAGE_MANAGER_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/AppStorageManager.cpp
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/AppStorageManagerImplementation.cpp
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/AppStorageManagerTelemetryReporting.cpp
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/RequestHandler.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/TelemetryReportingBase.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/UtilsTelemetryMetrics.cpp
    # Test sources
    AppStorageManager/AppStorageManagerTest.cpp
    AppStorageManager/AppStorageManager_LifecycleTests.cpp
    AppStorageManager/AppStorageManager_ImplementationTests.cpp
    AppStorageManager/AppStorageManager_ComponentTests.cpp
    common/L0MockPersistentStore.cpp
    common/L0Bootstrap.cpp
)

set(PREINSTALL_MANAGER_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../PreinstallManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../PreinstallManager/PreinstallManager.cpp
    ${CMAKE_SOURCE_DIR}/../../PreinstallManager/PreinstallManagerImplementation.cpp
    PreinstallManager/PreinstallManagerTest.cpp
    PreinstallManager/PreinstallManager_LifecycleTests.cpp
    PreinstallManager/PreinstallManager_ImplementationTests.cpp
    PreinstallManager/PreinstallManager_ComponentTests.cpp
    common/L0Bootstrap.cpp
)

set(PACKAGE_MANAGER_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../PackageManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../PackageManager/PackageManager.cpp
    ${CMAKE_SOURCE_DIR}/../../PackageManager/PackageManagerImplementation.cpp
    ${CMAKE_SOURCE_DIR}/../../PackageManager/PackageManagerTelemetryReporting.cpp
    ${CMAKE_SOURCE_DIR}/../../PackageManager/HttpClient.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/TelemetryReportingBase.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/UtilsTelemetryMetrics.cpp
    PackageManager/PackageManagerTest.cpp
    PackageManager/PackageManager_ShellTests.cpp
    PackageManager/PackageManager_ImplementationTests.cpp
    PackageManager/PackageManager_ComponentTests.cpp
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

if(PLUGIN_RDK_WINDOW_MANAGER)
    add_executable(rdkwindowmanager_l0test ${RDKWINDOWMANAGER_L0_SOURCES})
else()
    message(STATUS "PLUGIN_RDK_WINDOW_MANAGER=OFF; skipping rdkwindowmanager_l0test target")
endif()

if(PLUGIN_APP_STORAGE_MANAGER)
    add_executable(appstorage_l0test ${APPSTORAGE_MANAGER_L0_SOURCES})
else()
    message(STATUS "PLUGIN_APP_STORAGE_MANAGER=OFF; skipping appstorage_l0test target")
endif()

if(PLUGIN_PACKAGE_MANAGER)
    add_executable(packagemanager_l0test ${PACKAGE_MANAGER_L0_SOURCES})
else()
    message(STATUS "PLUGIN_PACKAGE_MANAGER=OFF; skipping packagemanager_l0test target")
endif()

if(PLUGIN_PREINSTALL_MANAGER)
    add_executable(preinstall_manager_l0test ${PREINSTALL_MANAGER_L0_SOURCES})
else()
    message(STATUS "PLUGIN_PREINSTALL_MANAGER=OFF; skipping preinstall_manager_l0test target")
endif()

if(NOT PLUGIN_RUNTIME_MANAGER AND NOT PLUGIN_LIFECYCLE_MANAGER AND NOT PLUGIN_RDK_WINDOW_MANAGER AND NOT PLUGIN_PACKAGE_MANAGER AND NOT PLUGIN_APP_STORAGE_MANAGER AND NOT PLUGIN_PREINSTALL_MANAGER)
    message(FATAL_ERROR "No L0 target enabled. Enable at least one of PLUGIN_RUNTIME_MANAGER, PLUGIN_LIFECYCLE_MANAGER, PLUGIN_RDK_WINDOW_MANAGER, PLUGIN_PACKAGE_MANAGER, PLUGIN_APP_STORAGE_MANAGER, or PLUGIN_PREINSTALL_MANAGER.")
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
        PLUGIN_RUNTIME_MANAGER_IMPLEMENTATION_NAME="RuntimeManagerImplementation"
        RALF_PACKAGE_SUPPORT_ENABLED=1
    )

    # Force include L0TestConfig.h in all source files
    target_compile_options(runtimemanager_l0test PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/common/L0TestConfig.h
    )

    if(APPMGR_ENABLE_COVERAGE)
        target_compile_options(runtimemanager_l0test PRIVATE -O0 -g --coverage)
        target_link_options(runtimemanager_l0test PRIVATE --coverage)
    endif()

    if(L0_ENABLE_WARNINGS)
        target_compile_options(runtimemanager_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    # Suppress deprecation warnings from Thunder/WPEFramework headers
    target_compile_options(runtimemanager_l0test PRIVATE -Wno-error=deprecated-declarations)

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
    )

    # Mark WPEFramework/Thunder headers as SYSTEM to suppress external warnings
    target_include_directories(runtimemanager_l0test SYSTEM PRIVATE
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
    )

    # Force include L0TestConfig.h in all source files
    target_compile_options(lifecyclemanager_l0test PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/common/L0TestConfig.h
    )

    if(APPLCM_ENABLE_COVERAGE)
        target_compile_options(lifecyclemanager_l0test PRIVATE -O0 -g --coverage)
        target_link_options(lifecyclemanager_l0test PRIVATE --coverage)
    endif()

    if(L0_ENABLE_WARNINGS)
        target_compile_options(lifecyclemanager_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    # Suppress deprecation warnings from Thunder/WPEFramework headers
    target_compile_options(lifecyclemanager_l0test PRIVATE -Wno-error=deprecated-declarations)

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
# RDKWindowManager target compile options / definitions / includes
# ---------------------------------------------------------------------------
if(PLUGIN_RDK_WINDOW_MANAGER)
    target_compile_definitions(rdkwindowmanager_l0test PRIVATE
        MODULE_NAME=RDKWindowManager_L0Test
        PLUGIN_RDK_WINDOW_MANAGER_IMPLEMENTATION_NAME="RDKWindowManagerImplementation"
    )

    # Force include L0TestConfig.h in all source files
    target_compile_options(rdkwindowmanager_l0test PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/common/L0TestConfig.h
    )

    if(APPWM_ENABLE_COVERAGE)
        target_compile_options(rdkwindowmanager_l0test PRIVATE -O0 -g --coverage)
        target_link_options(rdkwindowmanager_l0test PRIVATE --coverage)
    endif()

    if(L0_ENABLE_WARNINGS)
        target_compile_options(rdkwindowmanager_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    # Suppress deprecation warnings from Thunder/WPEFramework headers
    target_compile_options(rdkwindowmanager_l0test PRIVATE -Wno-error=deprecated-declarations)

    target_include_directories(rdkwindowmanager_l0test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/RDKWindowManager
        ${CMAKE_CURRENT_SOURCE_DIR}/RDKWindowManager/dependencies
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}/../../RDKWindowManager
        ${CMAKE_SOURCE_DIR}/../../helpers
        ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry
    )

    # Mark WPEFramework/Thunder headers as SYSTEM to suppress external warnings
    target_include_directories(rdkwindowmanager_l0test SYSTEM PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )

    if(EXISTS "${PREFIX}/lib")
        target_link_directories(rdkwindowmanager_l0test PRIVATE "${PREFIX}/lib")
    endif()
    if(EXISTS "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
        target_link_directories(rdkwindowmanager_l0test PRIVATE "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
    endif()
    if(EXISTS "${PREFIX}/lib/plugins")
        target_link_directories(rdkwindowmanager_l0test PRIVATE "${PREFIX}/lib/plugins")
    endif()
endif()

# ---------------------------------------------------------------------------
# AppStorageManager target compile options / definitions / includes
# ---------------------------------------------------------------------------
if(PLUGIN_APP_STORAGE_MANAGER)
    target_compile_definitions(appstorage_l0test PRIVATE
        MODULE_NAME=AppStorageManager_L0Test
    )

    # Force include L0TestConfig.h in all source files
    target_compile_options(appstorage_l0test PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/common/L0TestConfig.h
    )

    if(APPASM_ENABLE_COVERAGE)
        target_compile_options(appstorage_l0test PRIVATE -O0 -g --coverage)
        target_link_options(appstorage_l0test PRIVATE --coverage)
    endif()

    if(L0_ENABLE_WARNINGS)
        target_compile_options(appstorage_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    # Suppress deprecation warnings from Thunder/WPEFramework headers
    target_compile_options(appstorage_l0test PRIVATE -Wno-error=deprecated-declarations)

    target_include_directories(appstorage_l0test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/AppStorageManager
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}/../../AppStorageManager
        ${CMAKE_SOURCE_DIR}/../../helpers
        ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry
    )

    # Mark WPEFramework/Thunder headers as SYSTEM to suppress external warnings
    target_include_directories(appstorage_l0test SYSTEM PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )

    if(EXISTS "${PREFIX}/lib")
        target_link_directories(appstorage_l0test PRIVATE "${PREFIX}/lib")
    endif()
    if(EXISTS "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
        target_link_directories(appstorage_l0test PRIVATE "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
    endif()
    if(EXISTS "${PREFIX}/lib/plugins")
        target_link_directories(appstorage_l0test PRIVATE "${PREFIX}/lib/plugins")
    endif()
endif()
# PackageManager target compile options / definitions / includes
# ---------------------------------------------------------------------------
if(PLUGIN_PACKAGE_MANAGER)
    find_path(JSONCPP_INCLUDE_DIR_PKG NAMES json/json.h
        HINTS /usr/include/jsoncpp /usr/local/include/jsoncpp "${PREFIX}/include/jsoncpp")

    target_compile_definitions(packagemanager_l0test PRIVATE
        MODULE_NAME=PackageManager_L0Test
    )

    # Force include L0TestConfig.h in all source files
    target_compile_options(packagemanager_l0test PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/common/L0TestConfig.h
    )

    if(APPPKG_ENABLE_COVERAGE)
        target_compile_options(packagemanager_l0test PRIVATE -O0 -g --coverage)
        target_link_options(packagemanager_l0test PRIVATE --coverage)
    endif()

    if(L0_ENABLE_WARNINGS)
        target_compile_options(packagemanager_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    target_include_directories(packagemanager_l0test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/PackageManager
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}/../../PackageManager
        ${CMAKE_SOURCE_DIR}/../../helpers
        ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry
        ${CMAKE_SOURCE_DIR}/../../Tests/mocks
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
    
    if(JSONCPP_INCLUDE_DIR_PKG)
        target_include_directories(packagemanager_l0test PRIVATE ${JSONCPP_INCLUDE_DIR_PKG})
    endif()

    if(EXISTS "${PREFIX}/lib")
        target_link_directories(packagemanager_l0test PRIVATE "${PREFIX}/lib")
    endif()
    if(EXISTS "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
        target_link_directories(packagemanager_l0test PRIVATE "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
    endif()
    if(EXISTS "${PREFIX}/lib/plugins")
        target_link_directories(packagemanager_l0test PRIVATE "${PREFIX}/lib/plugins")
    endif()
endif()

# ---------------------------------------------------------------------------
# PreinstallManager target compile options / definitions / includes
# ---------------------------------------------------------------------------
if(PLUGIN_PREINSTALL_MANAGER)
    target_compile_definitions(preinstall_manager_l0test PRIVATE
        MODULE_NAME=PreinstallManager_L0Test
        PLUGIN_PREINSTALL_MANAGER_IMPLEMENTATION_NAME="PreinstallManagerImplementation"
    )

    # Force include L0TestConfig.h in all source files
    target_compile_options(preinstall_manager_l0test PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/common/L0TestConfig.h
    )

    if(APPMGR_PREINSTALL_ENABLE_COVERAGE)
        target_compile_options(preinstall_manager_l0test PRIVATE -O0 -g --coverage)
        target_link_options(preinstall_manager_l0test PRIVATE --coverage)
    endif()

    if(L0_ENABLE_WARNINGS)
        target_compile_options(preinstall_manager_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    target_include_directories(preinstall_manager_l0test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/PreinstallManager
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}/../../PreinstallManager
        ${CMAKE_SOURCE_DIR}/../../helpers
        ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )

    if(EXISTS "${PREFIX}/lib")
        target_link_directories(preinstall_manager_l0test PRIVATE "${PREFIX}/lib")
    endif()
    if(EXISTS "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
        target_link_directories(preinstall_manager_l0test PRIVATE "${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins")
    endif()
    if(EXISTS "${PREFIX}/lib/plugins")
        target_link_directories(preinstall_manager_l0test PRIVATE "${PREFIX}/lib/plugins")
    endif()
endif()

# ---------------------------------------------------------------------------
# Thunder / WPEFramework library discovery (shared for all targets)
# ---------------------------------------------------------------------------
find_library(THUNDER_CORE_LIB      NAMES ${NAMESPACE}Core      HINTS "${PREFIX}/lib")
find_library(THUNDER_MESSAGING_LIB NAMES ${NAMESPACE}Messaging HINTS "${PREFIX}/lib")
find_library(THUNDER_PLUGINS_LIB   NAMES ${NAMESPACE}Plugins   HINTS "${PREFIX}/lib")
find_library(THUNDER_COM_LIB       NAMES ${NAMESPACE}COM       HINTS "${PREFIX}/lib")
find_library(THUNDER_DEFINITIONS_LIB NAMES ${NAMESPACE}Definitions HINTS "${PREFIX}/lib")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Fallback include paths for environments without namespace package configs.
if(PLUGIN_RUNTIME_MANAGER)
    target_include_directories(runtimemanager_l0test SYSTEM PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
endif()
if(PLUGIN_LIFECYCLE_MANAGER)
    target_include_directories(lifecyclemanager_l0test SYSTEM PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
endif()
if(PLUGIN_RDK_WINDOW_MANAGER)
    target_include_directories(rdkwindowmanager_l0test SYSTEM PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
endif()
if(PLUGIN_APP_STORAGE_MANAGER)
    target_include_directories(appstorage_l0test SYSTEM PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
endif()
if(PLUGIN_PACKAGE_MANAGER)
    target_include_directories(packagemanager_l0test PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
endif()
if(PLUGIN_PREINSTALL_MANAGER)
    target_include_directories(preinstall_manager_l0test PRIVATE
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
endif()

if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(EXISTS "${_prefix}/include")
            if(PLUGIN_RUNTIME_MANAGER)
                target_include_directories(runtimemanager_l0test SYSTEM PRIVATE "${_prefix}/include")
            endif()
            if(PLUGIN_LIFECYCLE_MANAGER)
                target_include_directories(lifecyclemanager_l0test SYSTEM PRIVATE "${_prefix}/include")
            endif()
            if(PLUGIN_RDK_WINDOW_MANAGER)
                target_include_directories(rdkwindowmanager_l0test SYSTEM PRIVATE "${_prefix}/include")
            endif()
            if(PLUGIN_APP_STORAGE_MANAGER)
                target_include_directories(appstorage_l0test SYSTEM PRIVATE "${_prefix}/include")
            endif()
            if(PLUGIN_PACKAGE_MANAGER)
                target_include_directories(packagemanager_l0test PRIVATE "${_prefix}/include")
            endif()
            if(PLUGIN_PREINSTALL_MANAGER)
                target_include_directories(preinstall_manager_l0test PRIVATE "${_prefix}/include")
            endif()
        endif()
        if(EXISTS "${_prefix}/include/${NAMESPACE}")
            if(PLUGIN_RUNTIME_MANAGER)
                target_include_directories(runtimemanager_l0test SYSTEM PRIVATE "${_prefix}/include/${NAMESPACE}")
            endif()
            if(PLUGIN_LIFECYCLE_MANAGER)
                target_include_directories(lifecyclemanager_l0test SYSTEM PRIVATE "${_prefix}/include/${NAMESPACE}")
            endif()
            if(PLUGIN_RDK_WINDOW_MANAGER)
                target_include_directories(rdkwindowmanager_l0test SYSTEM PRIVATE "${_prefix}/include/${NAMESPACE}")
            endif()
            if(PLUGIN_APP_STORAGE_MANAGER)
                target_include_directories(appstorage_l0test SYSTEM PRIVATE "${_prefix}/include/${NAMESPACE}")
            endif()
            if(PLUGIN_PACKAGE_MANAGER)
                target_include_directories(packagemanager_l0test PRIVATE "${_prefix}/include/${NAMESPACE}")
            endif()
            if(PLUGIN_PREINSTALL_MANAGER)
                target_include_directories(preinstall_manager_l0test PRIVATE "${_prefix}/include/${NAMESPACE}")
            endif()
        endif()
    endforeach()
endif()

# Common Thunder libs + system libs for all enabled targets
set(_L0_TARGETS "")
if(PLUGIN_RUNTIME_MANAGER)
    list(APPEND _L0_TARGETS runtimemanager_l0test)
endif()
if(PLUGIN_LIFECYCLE_MANAGER)
    list(APPEND _L0_TARGETS lifecyclemanager_l0test)
endif()
if(PLUGIN_RDK_WINDOW_MANAGER)
    list(APPEND _L0_TARGETS rdkwindowmanager_l0test)
endif()
if(PLUGIN_APP_STORAGE_MANAGER)
    list(APPEND _L0_TARGETS appstorage_l0test)
endif()
if(PLUGIN_PACKAGE_MANAGER)
    list(APPEND _L0_TARGETS packagemanager_l0test)
endif()
if(PLUGIN_PREINSTALL_MANAGER)
    list(APPEND _L0_TARGETS preinstall_manager_l0test)
endif()

foreach(_target IN LISTS _L0_TARGETS)
    if(THUNDER_CORE_LIB)
        target_link_libraries(${_target} PRIVATE ${THUNDER_CORE_LIB})
    endif()
    if(THUNDER_MESSAGING_LIB)
        target_link_libraries(${_target} PRIVATE ${THUNDER_MESSAGING_LIB})
    endif()
    if(THUNDER_PLUGINS_LIB)
        target_link_libraries(${_target} PRIVATE ${THUNDER_PLUGINS_LIB})
    endif()
    if(THUNDER_COM_LIB)
        target_link_libraries(${_target} PRIVATE ${THUNDER_COM_LIB})
    endif()
    if(THUNDER_DEFINITIONS_LIB)
        target_link_libraries(${_target} PRIVATE ${THUNDER_DEFINITIONS_LIB})
    endif()
    target_link_libraries(${_target} PRIVATE
        Threads::Threads
        m
        dl
    )
endforeach()

# RuntimeManager-specific libraries (jsoncpp, curl, iptables)
if(PLUGIN_RUNTIME_MANAGER)
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

# PackageManager-specific libraries (jsoncpp, curl)
if(PLUGIN_PACKAGE_MANAGER)
    find_library(JSONCPP_LIB_PKG NAMES jsoncpp HINTS "${PREFIX}/lib" /usr/lib /usr/local/lib)
    find_library(CURL_LIB_PKG    NAMES curl    HINTS "${PREFIX}/lib" /usr/lib /usr/local/lib)

    if(JSONCPP_LIB_PKG)
        target_link_libraries(packagemanager_l0test PRIVATE ${JSONCPP_LIB_PKG})
    else()
        target_link_libraries(packagemanager_l0test PRIVATE jsoncpp)
    endif()
    if(CURL_LIB_PKG)
        target_link_libraries(packagemanager_l0test PRIVATE ${CURL_LIB_PKG})
    else()
        target_link_libraries(packagemanager_l0test PRIVATE curl)
    endif()
endif()

# PreinstallManager-specific libraries (jsoncpp, curl)
if(PLUGIN_PREINSTALL_MANAGER)
    find_library(JSONCPP_LIB_PREINSTALL NAMES jsoncpp HINTS "${PREFIX}/lib" /usr/lib /usr/local/lib)
    find_library(CURL_LIB_PREINSTALL    NAMES curl    HINTS "${PREFIX}/lib" /usr/lib /usr/local/lib)

    if(JSONCPP_LIB_PREINSTALL)
        target_link_libraries(preinstall_manager_l0test PRIVATE ${JSONCPP_LIB_PREINSTALL})
    else()
        target_link_libraries(preinstall_manager_l0test PRIVATE jsoncpp)
    endif()
    if(CURL_LIB_PREINSTALL)
        target_link_libraries(preinstall_manager_l0test PRIVATE ${CURL_LIB_PREINSTALL})
    else()
        target_link_libraries(preinstall_manager_l0test PRIVATE curl)
    endif()
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

if(PLUGIN_RDK_WINDOW_MANAGER)
    set_target_properties(rdkwindowmanager_l0test PROPERTIES
        BUILD_RPATH   "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
        INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
    )
endif()

if(PLUGIN_APP_STORAGE_MANAGER)
    set_target_properties(appstorage_l0test PROPERTIES
        BUILD_RPATH   "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
        INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
    )
endif()

if(PLUGIN_PACKAGE_MANAGER)
    set_target_properties(packagemanager_l0test PROPERTIES
        BUILD_RPATH   "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
        INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/${STORAGE_DIRECTORY};${PREFIX}/lib/${STORAGE_DIRECTORY}/plugins;${PREFIX}/lib/plugins"
    )
endif()

if(PLUGIN_PREINSTALL_MANAGER)
    set_target_properties(preinstall_manager_l0test PROPERTIES
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

if(PLUGIN_RDK_WINDOW_MANAGER)
    add_custom_target(run_rdkwindowmanager
        DEPENDS rdkwindowmanager_l0test
        COMMAND $<TARGET_FILE:rdkwindowmanager_l0test>
        COMMENT "Running rdkwindowmanager_l0test"
        VERBATIM
    )
endif()

if(PLUGIN_APP_STORAGE_MANAGER)
    add_custom_target(run_appstorage
        DEPENDS appstorage_l0test
        COMMAND $<TARGET_FILE:appstorage_l0test>
        COMMENT "Running appstorage_l0test"
        VERBATIM
    )
endif()

if(PLUGIN_PACKAGE_MANAGER)
    add_custom_target(run_packagemanager
        DEPENDS packagemanager_l0test
        COMMAND $<TARGET_FILE:packagemanager_l0test>
        COMMENT "Running packagemanager_l0test"
        VERBATIM
    )
endif()

if(PLUGIN_PREINSTALL_MANAGER)
    add_custom_target(run_preinstall
        DEPENDS preinstall_manager_l0test
        COMMAND $<TARGET_FILE:preinstall_manager_l0test>
        COMMENT "Running preinstall_manager_l0test"
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
    if(PLUGIN_RDK_WINDOW_MANAGER)
        list(APPEND COVERAGE_DEPENDS rdkwindowmanager_l0test)
    endif()
    if(PLUGIN_APP_STORAGE_MANAGER)
        list(APPEND COVERAGE_DEPENDS appstorage_l0test)
    endif()
    if(PLUGIN_PACKAGE_MANAGER)
        list(APPEND COVERAGE_DEPENDS packagemanager_l0test)
    endif()
    if(PLUGIN_PREINSTALL_MANAGER)
        list(APPEND COVERAGE_DEPENDS preinstall_manager_l0test)
    endif()

    add_custom_target(coverage
        DEPENDS ${COVERAGE_DEPENDS}
        COMMAND "${LCOV_EXECUTABLE}" --zerocounters -d "${CMAKE_BINARY_DIR}"
        COMMAND "${LCOV_EXECUTABLE}" -c -o coverage.info -d "${CMAKE_BINARY_DIR}"
        COMMAND "${GENHTML_EXECUTABLE}" -o coverage coverage.info
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/.."
        COMMENT "Generating coverage report with lcov/genhtml"
        VERBATIM
    )
endif()
