cmake_minimum_required(VERSION 3.16)

project(runtimemanager_l0test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(APPMGR_ENABLE_COVERAGE "Enable coverage flags for runtimemanager_l0test" ON)

if(NOT DEFINED PREFIX OR "${PREFIX}" STREQUAL "")
    if(DEFINED ENV{PREFIX} AND NOT "$ENV{PREFIX}" STREQUAL "")
        set(PREFIX "$ENV{PREFIX}")
    else()
        set(PREFIX "${CMAKE_SOURCE_DIR}/../../install/usr")
    endif()
endif()
set(PREFIX "${PREFIX}" CACHE STRING "Install prefix to locate Thunder artifacts" FORCE)

message(STATUS "Using PREFIX='${PREFIX}'")

set(RUNTIMEMANAGER_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager/RuntimeManager.cpp
    RuntimeManager/RuntimeManagerTest.cpp
    RuntimeManager/RuntimeManager_LifecycleTests.cpp
    common/L0Bootstrap.cpp
)

add_executable(runtimemanager_l0test ${RUNTIMEMANAGER_L0_SOURCES})

target_compile_definitions(runtimemanager_l0test PRIVATE
    MODULE_NAME=RuntimeManager_L0Test
    USE_THUNDER_R4=1
    RUNTIME_MANAGER_API_VERSION_NUMBER_MAJOR=1
    RUNTIME_MANAGER_API_VERSION_NUMBER_MINOR=0
    RUNTIME_MANAGER_API_VERSION_NUMBER_PATCH=0
)

if(APPMGR_ENABLE_COVERAGE)
    target_compile_options(runtimemanager_l0test PRIVATE -O0 -g --coverage)
    target_link_options(runtimemanager_l0test PRIVATE --coverage)
endif()

target_include_directories(runtimemanager_l0test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/RuntimeManager
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/../..
    ${CMAKE_SOURCE_DIR}/../../RuntimeManager
    ${CMAKE_SOURCE_DIR}/../../helpers
    ${PREFIX}/include
    ${PREFIX}/include/WPEFramework
)

if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(EXISTS "${_prefix}/include")
            target_include_directories(runtimemanager_l0test PRIVATE "${_prefix}/include")
        endif()
        if(EXISTS "${_prefix}/include/WPEFramework")
            target_include_directories(runtimemanager_l0test PRIVATE "${_prefix}/include/WPEFramework")
        endif()
    endforeach()
endif()

if(EXISTS "${PREFIX}/lib")
    target_link_directories(runtimemanager_l0test PRIVATE "${PREFIX}/lib")
endif()
if(EXISTS "${PREFIX}/lib/wpeframework/plugins")
    target_link_directories(runtimemanager_l0test PRIVATE "${PREFIX}/lib/wpeframework/plugins")
endif()

find_library(THUNDER_CORE_LIB NAMES WPEFrameworkCore ThunderCore HINTS "${PREFIX}/lib")
find_library(THUNDER_MESSAGING_LIB NAMES WPEFrameworkMessaging ThunderMessaging HINTS "${PREFIX}/lib")
find_library(THUNDER_PLUGINS_LIB NAMES WPEFrameworkPlugins ThunderPlugins HINTS "${PREFIX}/lib")
find_library(THUNDER_COM_LIB NAMES WPEFrameworkCOM ThunderCOM HINTS "${PREFIX}/lib")

find_package(Threads REQUIRED)

if(THUNDER_CORE_LIB)
    target_link_libraries(runtimemanager_l0test PRIVATE ${THUNDER_CORE_LIB})
endif()
if(THUNDER_MESSAGING_LIB)
    target_link_libraries(runtimemanager_l0test PRIVATE ${THUNDER_MESSAGING_LIB})
endif()
if(THUNDER_PLUGINS_LIB)
    target_link_libraries(runtimemanager_l0test PRIVATE ${THUNDER_PLUGINS_LIB})
endif()
if(THUNDER_COM_LIB)
    target_link_libraries(runtimemanager_l0test PRIVATE ${THUNDER_COM_LIB})
endif()

target_link_libraries(runtimemanager_l0test PRIVATE
    Threads::Threads
    dl
    m
)

set_target_properties(runtimemanager_l0test PROPERTIES
    BUILD_RPATH "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/wpeframework;${PREFIX}/lib/wpeframework/plugins"
    INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/wpeframework;${PREFIX}/lib/wpeframework/plugins"
)

add_custom_target(run
    DEPENDS runtimemanager_l0test
    COMMAND $<TARGET_FILE:runtimemanager_l0test>
    COMMENT "Running runtimemanager_l0test"
    VERBATIM
)

# ---------------------------------------------------------------------------
# preinstallmanager_l0test
# ---------------------------------------------------------------------------

set(PREINSTALLMANAGER_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../PreinstallManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../PreinstallManager/PreinstallManager.cpp
    ${CMAKE_SOURCE_DIR}/../../PreinstallManager/PreinstallManagerImplementation.cpp
    PreinstalManager/PreinstallManagerTest.cpp
    PreinstalManager/PreinstallManager_PluginTests.cpp
    PreinstalManager/PreinstallManagerImpl_CoreTests.cpp
    PreinstalManager/PreinstallManagerImpl_EventTests.cpp
    PreinstalManager/PreinstallManagerImpl_InstallAndVersionTests.cpp
    common/L0Bootstrap.cpp
)

add_executable(preinstallmanager_l0test ${PREINSTALLMANAGER_L0_SOURCES})

target_compile_definitions(preinstallmanager_l0test PRIVATE
    MODULE_NAME=PreinstallManager_L0Test
    USE_THUNDER_R4=1
    PREINSTALL_MANAGER_API_VERSION_NUMBER_MAJOR=1
    PREINSTALL_MANAGER_API_VERSION_NUMBER_MINOR=0
    PREINSTALL_MANAGER_API_VERSION_NUMBER_PATCH=0
)

if(APPMGR_ENABLE_COVERAGE)
    target_compile_options(preinstallmanager_l0test PRIVATE -O0 -g --coverage)
    target_link_options(preinstallmanager_l0test PRIVATE --coverage)
endif()

target_include_directories(preinstallmanager_l0test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/PreinstalManager
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/../..
    ${CMAKE_SOURCE_DIR}/../../PreinstallManager
    ${CMAKE_SOURCE_DIR}/../../helpers
    ${PREFIX}/include
    ${PREFIX}/include/WPEFramework
)

if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(EXISTS "${_prefix}/include")
            target_include_directories(preinstallmanager_l0test PRIVATE "${_prefix}/include")
        endif()
        if(EXISTS "${_prefix}/include/WPEFramework")
            target_include_directories(preinstallmanager_l0test PRIVATE "${_prefix}/include/WPEFramework")
        endif()
    endforeach()
endif()

if(EXISTS "${PREFIX}/lib")
    target_link_directories(preinstallmanager_l0test PRIVATE "${PREFIX}/lib")
endif()
if(EXISTS "${PREFIX}/lib/wpeframework/plugins")
    target_link_directories(preinstallmanager_l0test PRIVATE "${PREFIX}/lib/wpeframework/plugins")
endif()

if(THUNDER_CORE_LIB)
    target_link_libraries(preinstallmanager_l0test PRIVATE ${THUNDER_CORE_LIB})
endif()
if(THUNDER_MESSAGING_LIB)
    target_link_libraries(preinstallmanager_l0test PRIVATE ${THUNDER_MESSAGING_LIB})
endif()
if(THUNDER_PLUGINS_LIB)
    target_link_libraries(preinstallmanager_l0test PRIVATE ${THUNDER_PLUGINS_LIB})
endif()
if(THUNDER_COM_LIB)
    target_link_libraries(preinstallmanager_l0test PRIVATE ${THUNDER_COM_LIB})
endif()

target_link_libraries(preinstallmanager_l0test PRIVATE
    Threads::Threads
    dl
    m
)

set_target_properties(preinstallmanager_l0test PROPERTIES
    BUILD_RPATH "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/wpeframework;${PREFIX}/lib/wpeframework/plugins"
    INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/wpeframework;${PREFIX}/lib/wpeframework/plugins"
)

add_custom_target(run_preinstall
    DEPENDS preinstallmanager_l0test
    COMMAND $<TARGET_FILE:preinstallmanager_l0test>
    COMMENT "Running preinstallmanager_l0test"
    VERBATIM
)
