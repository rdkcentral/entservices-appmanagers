cmake_minimum_required(VERSION 3.16)
project(AppManagerAutoFuzz LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Add stubs directory early (BEFORE other includes) to find fallback headers only
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/stubs/include)
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(WARNING "Clang is recommended for libFuzzer builds")
endif()
# Workaround for macOS ASAN header issue: ensure stdint types are globally visible
add_compile_options(-include stdint.h)
# Compiler flags for fuzzing - libFuzzer + sanitizer set
add_compile_options(-fPIC)
add_compile_options(-fno-omit-frame-pointer -g -O1)
if(APPLE)
  # AppleClang does not support -fsanitize=leak on this target.
  set(_FUZZ_DEFAULT_SANITIZERS "fuzzer,address,undefined")
else()
  set(_FUZZ_DEFAULT_SANITIZERS "fuzzer,address,undefined,leak")
endif()
set(FUZZ_SANITIZERS "${_FUZZ_DEFAULT_SANITIZERS}" CACHE STRING "Sanitizers to enable for fuzz targets")
add_compile_options(-fsanitize=${FUZZ_SANITIZERS})
add_link_options(-fsanitize=${FUZZ_SANITIZERS})
add_compile_options(-fno-sanitize-recover=all)
set(FUZZ_DEPENDENCIES_ROOT "/Users/mthiru270@apac.comcast.com/fuzz7/dependencies")
option(FUZZ_INCLUDE_APP_SOURCES "Compile AppManager sources into fuzz targets" ON)
# Include paths: order matters on macOS - project root paths first
include_directories(${CMAKE_SOURCE_DIR}/../../..)
include_directories(${CMAKE_SOURCE_DIR}/../../../helpers)
include_directories(${CMAKE_SOURCE_DIR}/stubs)
# Add dependencies root for API headers
include_directories(${FUZZ_DEPENDENCIES_ROOT}/entservices-apis/apis)
include_directories(${FUZZ_DEPENDENCIES_ROOT})
# Add explicit include directories for common dependency patterns
file(GLOB DEP_API_DIRS LIST_DIRECTORIES true ${FUZZ_DEPENDENCIES_ROOT}/entservices-apis/apis/*/)
foreach(api_dir ${DEP_API_DIRS})
  if(IS_DIRECTORY ${api_dir})
    include_directories(${api_dir})
  endif()
endforeach()
# Discover jsoncpp/messaging include roots from dependencies when present.
file(GLOB_RECURSE FUZZ_JSONCPP_HEADERS ${FUZZ_DEPENDENCIES_ROOT}/*/json/json.h)
foreach(json_header ${FUZZ_JSONCPP_HEADERS})
  get_filename_component(_json_subdir ${json_header} DIRECTORY)
  get_filename_component(_json_root ${_json_subdir} DIRECTORY)
  include_directories(${_json_root})
endforeach()
file(GLOB_RECURSE FUZZ_MESSAGING_HEADERS ${FUZZ_DEPENDENCIES_ROOT}/*/messaging/messaging.h)
foreach(messaging_header ${FUZZ_MESSAGING_HEADERS})
  get_filename_component(_messaging_subdir ${messaging_header} DIRECTORY)
  get_filename_component(_messaging_root ${_messaging_subdir} DIRECTORY)
  include_directories(${_messaging_root})
endforeach()
set(FUZZ_OPTIONAL_LIBS)
find_library(FUZZ_JSONCPP_LIB NAMES jsoncpp PATHS ${FUZZ_DEPENDENCIES_ROOT} PATH_SUFFIXES lib lib64 build/lib NO_DEFAULT_PATH)
if(FUZZ_JSONCPP_LIB)
  list(APPEND FUZZ_OPTIONAL_LIBS ${FUZZ_JSONCPP_LIB})
endif()
find_library(FUZZ_MESSAGING_LIB NAMES messaging WPEFrameworkMessaging PATHS ${FUZZ_DEPENDENCIES_ROOT} PATH_SUFFIXES lib lib64 build/lib NO_DEFAULT_PATH)
if(FUZZ_MESSAGING_LIB)
  list(APPEND FUZZ_OPTIONAL_LIBS ${FUZZ_MESSAGING_LIB})
endif()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include_directories(${CMAKE_SOURCE_DIR}/../../../AppManager)
include_directories(${CMAKE_SOURCE_DIR}/../../../AppStorageManager)
include_directories(${CMAKE_SOURCE_DIR}/../../../DownloadManager)
include_directories(${CMAKE_SOURCE_DIR}/../../../LifecycleManager)
include_directories(${CMAKE_SOURCE_DIR}/../../../PackageManager)
include_directories(${CMAKE_SOURCE_DIR}/../../../PreinstallManager)
include_directories(${CMAKE_SOURCE_DIR}/../../../RDKWindowManager)
include_directories(${CMAKE_SOURCE_DIR}/../../../TelemetryMetrics)
set(APP_MANAGER_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../AppManager/AppInfo.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppManager/AppInfoManager.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppManager/AppManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppManager/AppManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppManager/LifecycleInterfaceConnector.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppStorageManager/AppStorageManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppStorageManager/AppStorageManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppStorageManager/RequestHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../DownloadManager/DownloadManagerHttpClient.cpp
  ${CMAKE_SOURCE_DIR}/../../../DownloadManager/DownloadManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../DownloadManager/DownloadManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/ApplicationContext.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/LifecycleManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/LifecycleManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/RequestHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/RuntimeManagerHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/State.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/StateHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/StateTransitionHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/WindowManagerHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../PackageManager/HttpClient.cpp
  ${CMAKE_SOURCE_DIR}/../../../PackageManager/PackageManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../PackageManager/PackageManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../PreinstallManager/PreinstallManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../RDKWindowManager/RDKWindowManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../RDKWindowManager/RDKWindowManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../TelemetryMetrics/TelemetryMetricsImplementation.cpp
)
set(STUB_SOURCES
  ${CMAKE_SOURCE_DIR}/stubs/fuzz_stub_profiles.c
)
add_library(appmanager_fuzz_stub_support OBJECT ${STUB_SOURCES})
set_target_properties(appmanager_fuzz_stub_support PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(appmanager_fuzz_stub_support PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set(FUZZ_APPMANAGER_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../AppManager/AppInfo.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppManager/AppInfoManager.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppManager/AppManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppManager/AppManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppManager/LifecycleInterfaceConnector.cpp
)
if(FUZZ_INCLUDE_APP_SOURCES)
  add_library(appmanager_fuzz_appmanager SHARED ${FUZZ_APPMANAGER_SOURCES})
  set_target_properties(appmanager_fuzz_appmanager PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin
  )
  target_sources(appmanager_fuzz_appmanager PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)
  if(FUZZ_OPTIONAL_LIBS)
    target_link_libraries(appmanager_fuzz_appmanager PRIVATE ${FUZZ_OPTIONAL_LIBS})
  endif()
endif()
set(FUZZ_APPSTORAGEMANAGER_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../AppStorageManager/AppStorageManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppStorageManager/AppStorageManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../AppStorageManager/RequestHandler.cpp
)
if(FUZZ_INCLUDE_APP_SOURCES)
  add_library(appmanager_fuzz_appstoragemanager SHARED ${FUZZ_APPSTORAGEMANAGER_SOURCES})
  set_target_properties(appmanager_fuzz_appstoragemanager PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin
  )
  target_sources(appmanager_fuzz_appstoragemanager PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)
  if(FUZZ_OPTIONAL_LIBS)
    target_link_libraries(appmanager_fuzz_appstoragemanager PRIVATE ${FUZZ_OPTIONAL_LIBS})
  endif()
endif()
set(FUZZ_DOWNLOADMANAGER_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../DownloadManager/DownloadManagerHttpClient.cpp
  ${CMAKE_SOURCE_DIR}/../../../DownloadManager/DownloadManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../DownloadManager/DownloadManagerTelemetryReporting.cpp
)
if(FUZZ_INCLUDE_APP_SOURCES)
  add_library(appmanager_fuzz_downloadmanager SHARED ${FUZZ_DOWNLOADMANAGER_SOURCES})
  set_target_properties(appmanager_fuzz_downloadmanager PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin
  )
  target_sources(appmanager_fuzz_downloadmanager PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)
  if(FUZZ_OPTIONAL_LIBS)
    target_link_libraries(appmanager_fuzz_downloadmanager PRIVATE ${FUZZ_OPTIONAL_LIBS})
  endif()
endif()
set(FUZZ_LIFECYCLEMANAGER_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/ApplicationContext.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/LifecycleManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/LifecycleManagerTelemetryReporting.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/RequestHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/RuntimeManagerHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/State.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/StateHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/StateTransitionHandler.cpp
  ${CMAKE_SOURCE_DIR}/../../../LifecycleManager/WindowManagerHandler.cpp
)
if(FUZZ_INCLUDE_APP_SOURCES)
  add_library(appmanager_fuzz_lifecyclemanager SHARED ${FUZZ_LIFECYCLEMANAGER_SOURCES})
  set_target_properties(appmanager_fuzz_lifecyclemanager PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin
  )
  target_sources(appmanager_fuzz_lifecyclemanager PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)
  if(FUZZ_OPTIONAL_LIBS)
    target_link_libraries(appmanager_fuzz_lifecyclemanager PRIVATE ${FUZZ_OPTIONAL_LIBS})
  endif()
endif()
set(FUZZ_PACKAGEMANAGER_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../PackageManager/HttpClient.cpp
  ${CMAKE_SOURCE_DIR}/../../../PackageManager/PackageManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../PackageManager/PackageManagerTelemetryReporting.cpp
)
if(FUZZ_INCLUDE_APP_SOURCES)
  add_library(appmanager_fuzz_packagemanager SHARED ${FUZZ_PACKAGEMANAGER_SOURCES})
  set_target_properties(appmanager_fuzz_packagemanager PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin
  )
  target_sources(appmanager_fuzz_packagemanager PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)
  if(FUZZ_OPTIONAL_LIBS)
    target_link_libraries(appmanager_fuzz_packagemanager PRIVATE ${FUZZ_OPTIONAL_LIBS})
  endif()
endif()
set(FUZZ_PREINSTALLMANAGER_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../PreinstallManager/PreinstallManagerImplementation.cpp
)
if(FUZZ_INCLUDE_APP_SOURCES)
  add_library(appmanager_fuzz_preinstallmanager SHARED ${FUZZ_PREINSTALLMANAGER_SOURCES})
  set_target_properties(appmanager_fuzz_preinstallmanager PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin
  )
  target_sources(appmanager_fuzz_preinstallmanager PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)
  if(FUZZ_OPTIONAL_LIBS)
    target_link_libraries(appmanager_fuzz_preinstallmanager PRIVATE ${FUZZ_OPTIONAL_LIBS})
  endif()
endif()
set(FUZZ_RDKWINDOWMANAGER_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../RDKWindowManager/RDKWindowManagerImplementation.cpp
  ${CMAKE_SOURCE_DIR}/../../../RDKWindowManager/RDKWindowManagerTelemetryReporting.cpp
)
if(FUZZ_INCLUDE_APP_SOURCES)
  add_library(appmanager_fuzz_rdkwindowmanager SHARED ${FUZZ_RDKWINDOWMANAGER_SOURCES})
  set_target_properties(appmanager_fuzz_rdkwindowmanager PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin
  )
  target_sources(appmanager_fuzz_rdkwindowmanager PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)
  if(FUZZ_OPTIONAL_LIBS)
    target_link_libraries(appmanager_fuzz_rdkwindowmanager PRIVATE ${FUZZ_OPTIONAL_LIBS})
  endif()
endif()
set(FUZZ_TELEMETRYMETRICS_SOURCES
  ${CMAKE_SOURCE_DIR}/../../../TelemetryMetrics/TelemetryMetricsImplementation.cpp
)
if(FUZZ_INCLUDE_APP_SOURCES)
  add_library(appmanager_fuzz_telemetrymetrics SHARED ${FUZZ_TELEMETRYMETRICS_SOURCES})
  set_target_properties(appmanager_fuzz_telemetrymetrics PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH ${CMAKE_BINARY_DIR}/bin
  )
  target_sources(appmanager_fuzz_telemetrymetrics PRIVATE $<TARGET_OBJECTS:appmanager_fuzz_stub_support>)
  if(FUZZ_OPTIONAL_LIBS)
    target_link_libraries(appmanager_fuzz_telemetrymetrics PRIVATE ${FUZZ_OPTIONAL_LIBS})
  endif()
endif()
add_executable(fuzz_3043bb122cd5f6ca
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3043bb122cd5f6ca.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_3043bb122cd5f6ca PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3043bb122cd5f6ca PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3043bb122cd5f6ca PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_47dd0cc80b4f48b5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_47dd0cc80b4f48b5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_47dd0cc80b4f48b5 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_47dd0cc80b4f48b5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_47dd0cc80b4f48b5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7d0ac3c620fbfb52
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7d0ac3c620fbfb52.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_7d0ac3c620fbfb52 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7d0ac3c620fbfb52 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7d0ac3c620fbfb52 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9348f55913cfc989
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9348f55913cfc989.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_9348f55913cfc989 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9348f55913cfc989 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9348f55913cfc989 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f0c49b4ab4c020b7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f0c49b4ab4c020b7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_f0c49b4ab4c020b7 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f0c49b4ab4c020b7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f0c49b4ab4c020b7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ec9e58338bf2ec09
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ec9e58338bf2ec09.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_ec9e58338bf2ec09 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ec9e58338bf2ec09 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ec9e58338bf2ec09 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_67fb74f88436bfbf
  ${CMAKE_SOURCE_DIR}/generated/fuzz_67fb74f88436bfbf.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_67fb74f88436bfbf PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_67fb74f88436bfbf PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_67fb74f88436bfbf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_17f9c1c1c3fa30da
  ${CMAKE_SOURCE_DIR}/generated/fuzz_17f9c1c1c3fa30da.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_17f9c1c1c3fa30da PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_17f9c1c1c3fa30da PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_17f9c1c1c3fa30da PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_92369418f58d1487
  ${CMAKE_SOURCE_DIR}/generated/fuzz_92369418f58d1487.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_92369418f58d1487 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_92369418f58d1487 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_92369418f58d1487 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_88ade219eba695ea
  ${CMAKE_SOURCE_DIR}/generated/fuzz_88ade219eba695ea.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_88ade219eba695ea PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_88ade219eba695ea PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_88ade219eba695ea PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5e79c08574175506
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5e79c08574175506.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_5e79c08574175506 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5e79c08574175506 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5e79c08574175506 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_523f8fba8fb68f94
  ${CMAKE_SOURCE_DIR}/generated/fuzz_523f8fba8fb68f94.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_523f8fba8fb68f94 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_523f8fba8fb68f94 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_523f8fba8fb68f94 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_82b2db10ad526878
  ${CMAKE_SOURCE_DIR}/generated/fuzz_82b2db10ad526878.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_82b2db10ad526878 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_82b2db10ad526878 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_82b2db10ad526878 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_162d29a31b42448f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_162d29a31b42448f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_162d29a31b42448f PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_162d29a31b42448f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_162d29a31b42448f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e107de0b47d23d32
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e107de0b47d23d32.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_e107de0b47d23d32 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e107de0b47d23d32 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e107de0b47d23d32 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b58810975b95566e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b58810975b95566e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_b58810975b95566e PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b58810975b95566e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b58810975b95566e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6721c8e2dc183778
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6721c8e2dc183778.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_6721c8e2dc183778 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6721c8e2dc183778 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6721c8e2dc183778 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6bd56dece3a4c5e7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6bd56dece3a4c5e7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_6bd56dece3a4c5e7 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6bd56dece3a4c5e7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6bd56dece3a4c5e7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_650c091c0eeff0a8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_650c091c0eeff0a8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_650c091c0eeff0a8 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_650c091c0eeff0a8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_650c091c0eeff0a8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_43833f032e150bd7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_43833f032e150bd7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_43833f032e150bd7 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_43833f032e150bd7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_43833f032e150bd7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3e7b8ffebb713596
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3e7b8ffebb713596.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_3e7b8ffebb713596 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3e7b8ffebb713596 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3e7b8ffebb713596 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_81a276095a08ad22
  ${CMAKE_SOURCE_DIR}/generated/fuzz_81a276095a08ad22.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_81a276095a08ad22 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_81a276095a08ad22 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_81a276095a08ad22 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5206f38956ca9a2f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5206f38956ca9a2f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_5206f38956ca9a2f PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5206f38956ca9a2f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5206f38956ca9a2f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4737f1f38d854654
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4737f1f38d854654.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_4737f1f38d854654 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4737f1f38d854654 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4737f1f38d854654 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_273b553ab497cf84
  ${CMAKE_SOURCE_DIR}/generated/fuzz_273b553ab497cf84.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_273b553ab497cf84 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_273b553ab497cf84 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_273b553ab497cf84 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6d2f0a041048da4d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6d2f0a041048da4d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_6d2f0a041048da4d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6d2f0a041048da4d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6d2f0a041048da4d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_be4e6f3425608b74
  ${CMAKE_SOURCE_DIR}/generated/fuzz_be4e6f3425608b74.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_be4e6f3425608b74 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_be4e6f3425608b74 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_be4e6f3425608b74 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0c34dd86809e5c32
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0c34dd86809e5c32.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_0c34dd86809e5c32 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0c34dd86809e5c32 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0c34dd86809e5c32 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c363ddb29d224a16
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c363ddb29d224a16.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c363ddb29d224a16 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c363ddb29d224a16 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c363ddb29d224a16 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c462dcb1983e91ba
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c462dcb1983e91ba.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c462dcb1983e91ba PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c462dcb1983e91ba PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c462dcb1983e91ba PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c4df6233ffb76d68
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c4df6233ffb76d68.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c4df6233ffb76d68 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c4df6233ffb76d68 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c4df6233ffb76d68 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_debc8ed5c84448ca
  ${CMAKE_SOURCE_DIR}/generated/fuzz_debc8ed5c84448ca.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_debc8ed5c84448ca PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_debc8ed5c84448ca PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_debc8ed5c84448ca PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6d27b933bd6b3ad0
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6d27b933bd6b3ad0.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_6d27b933bd6b3ad0 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6d27b933bd6b3ad0 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6d27b933bd6b3ad0 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2b71457174bdf63a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2b71457174bdf63a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2b71457174bdf63a PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2b71457174bdf63a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2b71457174bdf63a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f516380a903e56f0
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f516380a903e56f0.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_f516380a903e56f0 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f516380a903e56f0 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f516380a903e56f0 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8e5f756c9bd12258
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8e5f756c9bd12258.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_8e5f756c9bd12258 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8e5f756c9bd12258 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8e5f756c9bd12258 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ce05b6ed76d460cf
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ce05b6ed76d460cf.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_ce05b6ed76d460cf PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ce05b6ed76d460cf PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ce05b6ed76d460cf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_46d8a28e58616c61
  ${CMAKE_SOURCE_DIR}/generated/fuzz_46d8a28e58616c61.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_46d8a28e58616c61 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_46d8a28e58616c61 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_46d8a28e58616c61 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2bcdfd96a945562d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2bcdfd96a945562d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2bcdfd96a945562d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2bcdfd96a945562d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2bcdfd96a945562d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_efd23abf48860730
  ${CMAKE_SOURCE_DIR}/generated/fuzz_efd23abf48860730.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_efd23abf48860730 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_efd23abf48860730 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_efd23abf48860730 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d667e634a3770d59
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d667e634a3770d59.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_d667e634a3770d59 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d667e634a3770d59 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d667e634a3770d59 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_418d5888b7477b2f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_418d5888b7477b2f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_418d5888b7477b2f PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_418d5888b7477b2f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_418d5888b7477b2f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bb5d3bc4322be068
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bb5d3bc4322be068.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_bb5d3bc4322be068 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bb5d3bc4322be068 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bb5d3bc4322be068 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8b2f480dadf9689f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8b2f480dadf9689f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_8b2f480dadf9689f PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8b2f480dadf9689f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8b2f480dadf9689f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d077c855e886f26e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d077c855e886f26e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_d077c855e886f26e PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d077c855e886f26e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d077c855e886f26e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7b18549a68b9f215
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7b18549a68b9f215.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_7b18549a68b9f215 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7b18549a68b9f215 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7b18549a68b9f215 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e4fdc8c10595fe40
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e4fdc8c10595fe40.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_e4fdc8c10595fe40 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e4fdc8c10595fe40 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e4fdc8c10595fe40 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c9d03d2acb1a8b21
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c9d03d2acb1a8b21.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c9d03d2acb1a8b21 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c9d03d2acb1a8b21 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c9d03d2acb1a8b21 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_75478cecda4809e3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_75478cecda4809e3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_75478cecda4809e3 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_75478cecda4809e3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_75478cecda4809e3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cbadd9249076186b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cbadd9249076186b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_cbadd9249076186b PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cbadd9249076186b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cbadd9249076186b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_25bb01c6cc4086b2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_25bb01c6cc4086b2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_25bb01c6cc4086b2 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_25bb01c6cc4086b2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_25bb01c6cc4086b2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_45420a32fe55dfd8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_45420a32fe55dfd8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_45420a32fe55dfd8 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_45420a32fe55dfd8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_45420a32fe55dfd8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a415d04fe384fa33
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a415d04fe384fa33.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_a415d04fe384fa33 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a415d04fe384fa33 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a415d04fe384fa33 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2949803b4e7be365
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2949803b4e7be365.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2949803b4e7be365 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2949803b4e7be365 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2949803b4e7be365 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7f4ea3bca8ab5858
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7f4ea3bca8ab5858.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_7f4ea3bca8ab5858 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7f4ea3bca8ab5858 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7f4ea3bca8ab5858 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1d57314425b585f8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1d57314425b585f8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_1d57314425b585f8 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1d57314425b585f8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1d57314425b585f8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_824460657e151252
  ${CMAKE_SOURCE_DIR}/generated/fuzz_824460657e151252.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_824460657e151252 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_824460657e151252 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_824460657e151252 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_78802bb14d883e65
  ${CMAKE_SOURCE_DIR}/generated/fuzz_78802bb14d883e65.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_78802bb14d883e65 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_78802bb14d883e65 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_78802bb14d883e65 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fa51c0250076706d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fa51c0250076706d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_fa51c0250076706d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fa51c0250076706d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fa51c0250076706d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2b7b857e2b206875
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2b7b857e2b206875.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2b7b857e2b206875 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2b7b857e2b206875 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2b7b857e2b206875 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3002bff7e5530a97
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3002bff7e5530a97.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_3002bff7e5530a97 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3002bff7e5530a97 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3002bff7e5530a97 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c79a5ecc2e776dde
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c79a5ecc2e776dde.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c79a5ecc2e776dde PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c79a5ecc2e776dde PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c79a5ecc2e776dde PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_985677813c09fc7e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_985677813c09fc7e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_985677813c09fc7e PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_985677813c09fc7e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_985677813c09fc7e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_83967c5bf2da11d1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_83967c5bf2da11d1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_83967c5bf2da11d1 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_83967c5bf2da11d1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_83967c5bf2da11d1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a12b86527e008b4d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a12b86527e008b4d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_a12b86527e008b4d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a12b86527e008b4d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a12b86527e008b4d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_00df9693c88ba635
  ${CMAKE_SOURCE_DIR}/generated/fuzz_00df9693c88ba635.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_00df9693c88ba635 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_00df9693c88ba635 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_00df9693c88ba635 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0313bf9aed4df543
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0313bf9aed4df543.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_0313bf9aed4df543 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0313bf9aed4df543 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0313bf9aed4df543 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1a1949720e8609d2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1a1949720e8609d2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_1a1949720e8609d2 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1a1949720e8609d2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1a1949720e8609d2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_04aaa8c069ef5c7b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_04aaa8c069ef5c7b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_04aaa8c069ef5c7b PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_04aaa8c069ef5c7b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_04aaa8c069ef5c7b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c244e809d6055b87
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c244e809d6055b87.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c244e809d6055b87 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c244e809d6055b87 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c244e809d6055b87 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_93922bfb2fffe8e2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_93922bfb2fffe8e2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_93922bfb2fffe8e2 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_93922bfb2fffe8e2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_93922bfb2fffe8e2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f09d0fb6eb4ff960
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f09d0fb6eb4ff960.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_f09d0fb6eb4ff960 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f09d0fb6eb4ff960 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f09d0fb6eb4ff960 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3c049a750f1e2a06
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3c049a750f1e2a06.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_3c049a750f1e2a06 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3c049a750f1e2a06 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3c049a750f1e2a06 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_989031a8920cc5a6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_989031a8920cc5a6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_989031a8920cc5a6 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_989031a8920cc5a6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_989031a8920cc5a6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_75517c86215a5ff3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_75517c86215a5ff3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_75517c86215a5ff3 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_75517c86215a5ff3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_75517c86215a5ff3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fd6586527512cfba
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fd6586527512cfba.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_fd6586527512cfba PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fd6586527512cfba PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fd6586527512cfba PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ddb072a1d0443137
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ddb072a1d0443137.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_ddb072a1d0443137 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ddb072a1d0443137 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ddb072a1d0443137 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bac6a6c3317fde3b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bac6a6c3317fde3b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_bac6a6c3317fde3b PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bac6a6c3317fde3b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bac6a6c3317fde3b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_629568434320e5a8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_629568434320e5a8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_629568434320e5a8 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_629568434320e5a8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_629568434320e5a8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a70f13077d22e2fb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a70f13077d22e2fb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_a70f13077d22e2fb PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a70f13077d22e2fb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a70f13077d22e2fb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_61d277085d08f1ec
  ${CMAKE_SOURCE_DIR}/generated/fuzz_61d277085d08f1ec.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_61d277085d08f1ec PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_61d277085d08f1ec PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_61d277085d08f1ec PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a3f67db89cbb41ae
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a3f67db89cbb41ae.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_a3f67db89cbb41ae PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a3f67db89cbb41ae PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a3f67db89cbb41ae PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_77598a5213700108
  ${CMAKE_SOURCE_DIR}/generated/fuzz_77598a5213700108.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_77598a5213700108 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_77598a5213700108 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_77598a5213700108 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8144c35200d029c1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8144c35200d029c1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_8144c35200d029c1 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8144c35200d029c1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8144c35200d029c1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bd8a50ba3eb4f68d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bd8a50ba3eb4f68d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_bd8a50ba3eb4f68d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bd8a50ba3eb4f68d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bd8a50ba3eb4f68d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_645b7222d9da245b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_645b7222d9da245b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_645b7222d9da245b PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_645b7222d9da245b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_645b7222d9da245b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5521375841c5b5b1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5521375841c5b5b1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_5521375841c5b5b1 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5521375841c5b5b1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5521375841c5b5b1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3ce862612fc1ef76
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3ce862612fc1ef76.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_3ce862612fc1ef76 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3ce862612fc1ef76 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3ce862612fc1ef76 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2fadc4222a88efd5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2fadc4222a88efd5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2fadc4222a88efd5 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2fadc4222a88efd5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2fadc4222a88efd5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bacc74283c86e026
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bacc74283c86e026.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_bacc74283c86e026 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bacc74283c86e026 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bacc74283c86e026 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_820b4120408dc873
  ${CMAKE_SOURCE_DIR}/generated/fuzz_820b4120408dc873.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_820b4120408dc873 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_820b4120408dc873 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_820b4120408dc873 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_27bccae419e496ab
  ${CMAKE_SOURCE_DIR}/generated/fuzz_27bccae419e496ab.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_27bccae419e496ab PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_27bccae419e496ab PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_27bccae419e496ab PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c824ebfe34e52011
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c824ebfe34e52011.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c824ebfe34e52011 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c824ebfe34e52011 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c824ebfe34e52011 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_efb9f46ab123182c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_efb9f46ab123182c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_efb9f46ab123182c PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_efb9f46ab123182c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_efb9f46ab123182c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a73664edc9bb0f5f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a73664edc9bb0f5f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_a73664edc9bb0f5f PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a73664edc9bb0f5f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a73664edc9bb0f5f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c7c71520a563e221
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c7c71520a563e221.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c7c71520a563e221 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c7c71520a563e221 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c7c71520a563e221 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_78c3821ef93bd768
  ${CMAKE_SOURCE_DIR}/generated/fuzz_78c3821ef93bd768.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_78c3821ef93bd768 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_78c3821ef93bd768 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_78c3821ef93bd768 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_eda9e1be167a6baf
  ${CMAKE_SOURCE_DIR}/generated/fuzz_eda9e1be167a6baf.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_eda9e1be167a6baf PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_eda9e1be167a6baf PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_eda9e1be167a6baf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cadee6cc2a7ff706
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cadee6cc2a7ff706.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_cadee6cc2a7ff706 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cadee6cc2a7ff706 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cadee6cc2a7ff706 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e8e4afb32cd89610
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e8e4afb32cd89610.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_e8e4afb32cd89610 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e8e4afb32cd89610 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e8e4afb32cd89610 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8c658542caa87630
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8c658542caa87630.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_8c658542caa87630 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8c658542caa87630 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8c658542caa87630 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d1be8288206d4c50
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d1be8288206d4c50.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_d1be8288206d4c50 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d1be8288206d4c50 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d1be8288206d4c50 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dff4d860d23643ec
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dff4d860d23643ec.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_dff4d860d23643ec PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dff4d860d23643ec PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dff4d860d23643ec PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_eb2fdb2e3dde654f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_eb2fdb2e3dde654f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_eb2fdb2e3dde654f PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_eb2fdb2e3dde654f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_eb2fdb2e3dde654f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d60ff25bae886f2b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d60ff25bae886f2b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_d60ff25bae886f2b PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d60ff25bae886f2b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d60ff25bae886f2b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_978911aac0f3d97e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_978911aac0f3d97e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_978911aac0f3d97e PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_978911aac0f3d97e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_978911aac0f3d97e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_31337381b1bde14e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_31337381b1bde14e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_31337381b1bde14e PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_31337381b1bde14e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_31337381b1bde14e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8d597bc98693e230
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8d597bc98693e230.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_8d597bc98693e230 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8d597bc98693e230 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8d597bc98693e230 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5745cf60a18999e8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5745cf60a18999e8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_5745cf60a18999e8 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5745cf60a18999e8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5745cf60a18999e8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b90479c017971547
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b90479c017971547.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_b90479c017971547 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b90479c017971547 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b90479c017971547 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6718c828acc3be3a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6718c828acc3be3a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_6718c828acc3be3a PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6718c828acc3be3a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6718c828acc3be3a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5e7c36f3b7f2a8f7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5e7c36f3b7f2a8f7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_5e7c36f3b7f2a8f7 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5e7c36f3b7f2a8f7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5e7c36f3b7f2a8f7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_747fbcef9de4289d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_747fbcef9de4289d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_747fbcef9de4289d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_747fbcef9de4289d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_747fbcef9de4289d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_41c48a691df0f343
  ${CMAKE_SOURCE_DIR}/generated/fuzz_41c48a691df0f343.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_41c48a691df0f343 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_41c48a691df0f343 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_41c48a691df0f343 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6a857380647bd909
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6a857380647bd909.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_6a857380647bd909 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6a857380647bd909 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6a857380647bd909 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3d994c6a9341f6c6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3d994c6a9341f6c6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_3d994c6a9341f6c6 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3d994c6a9341f6c6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3d994c6a9341f6c6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c63973069e02d826
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c63973069e02d826.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c63973069e02d826 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c63973069e02d826 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c63973069e02d826 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4814ac11ce5611c7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4814ac11ce5611c7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_4814ac11ce5611c7 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4814ac11ce5611c7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4814ac11ce5611c7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fe8bebdd0198048c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fe8bebdd0198048c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_fe8bebdd0198048c PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fe8bebdd0198048c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fe8bebdd0198048c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0a3fcc07230a6f16
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0a3fcc07230a6f16.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_0a3fcc07230a6f16 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0a3fcc07230a6f16 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0a3fcc07230a6f16 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9580bd8941bc47c6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9580bd8941bc47c6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_9580bd8941bc47c6 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9580bd8941bc47c6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9580bd8941bc47c6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_628da917d600a681
  ${CMAKE_SOURCE_DIR}/generated/fuzz_628da917d600a681.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_628da917d600a681 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_628da917d600a681 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_628da917d600a681 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_92882246d163054f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_92882246d163054f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_92882246d163054f PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_92882246d163054f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_92882246d163054f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e5e94d46bf759e72
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e5e94d46bf759e72.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_e5e94d46bf759e72 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e5e94d46bf759e72 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e5e94d46bf759e72 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4e14b1a0afc70fe7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4e14b1a0afc70fe7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_4e14b1a0afc70fe7 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4e14b1a0afc70fe7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4e14b1a0afc70fe7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7f2c1a9fb3e314ba
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7f2c1a9fb3e314ba.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_7f2c1a9fb3e314ba PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7f2c1a9fb3e314ba PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7f2c1a9fb3e314ba PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_212370f8caea56d8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_212370f8caea56d8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_212370f8caea56d8 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_212370f8caea56d8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_212370f8caea56d8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_51afca78fe9099ab
  ${CMAKE_SOURCE_DIR}/generated/fuzz_51afca78fe9099ab.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_51afca78fe9099ab PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_51afca78fe9099ab PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_51afca78fe9099ab PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_52fb639bf10acb0a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_52fb639bf10acb0a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_52fb639bf10acb0a PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_52fb639bf10acb0a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_52fb639bf10acb0a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f6e14a07affced3b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f6e14a07affced3b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_f6e14a07affced3b PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f6e14a07affced3b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f6e14a07affced3b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f00e4987bc3ac2fb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f00e4987bc3ac2fb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_f00e4987bc3ac2fb PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f00e4987bc3ac2fb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f00e4987bc3ac2fb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1fcf8c52ad87f54b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1fcf8c52ad87f54b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_1fcf8c52ad87f54b PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1fcf8c52ad87f54b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1fcf8c52ad87f54b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8940aa90c2c911f7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8940aa90c2c911f7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_8940aa90c2c911f7 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8940aa90c2c911f7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8940aa90c2c911f7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_22b7cabd40f32ea3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_22b7cabd40f32ea3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_22b7cabd40f32ea3 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_22b7cabd40f32ea3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_22b7cabd40f32ea3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7d074dfd3e307b59
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7d074dfd3e307b59.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_7d074dfd3e307b59 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7d074dfd3e307b59 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7d074dfd3e307b59 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dfd8cd5e9de1ac55
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dfd8cd5e9de1ac55.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_dfd8cd5e9de1ac55 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dfd8cd5e9de1ac55 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dfd8cd5e9de1ac55 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5bcb233671fcbfd8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5bcb233671fcbfd8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_5bcb233671fcbfd8 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5bcb233671fcbfd8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5bcb233671fcbfd8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_623ac4b99aff596d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_623ac4b99aff596d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_623ac4b99aff596d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_623ac4b99aff596d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_623ac4b99aff596d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_33b651b3854d5f14
  ${CMAKE_SOURCE_DIR}/generated/fuzz_33b651b3854d5f14.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_33b651b3854d5f14 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_33b651b3854d5f14 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_33b651b3854d5f14 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7e7043cedc6998ed
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7e7043cedc6998ed.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_7e7043cedc6998ed PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7e7043cedc6998ed PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7e7043cedc6998ed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7887b52f2a163230
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7887b52f2a163230.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_7887b52f2a163230 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7887b52f2a163230 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7887b52f2a163230 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f8bf7c6429d8ead1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f8bf7c6429d8ead1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_f8bf7c6429d8ead1 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f8bf7c6429d8ead1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f8bf7c6429d8ead1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_11174f9668293c15
  ${CMAKE_SOURCE_DIR}/generated/fuzz_11174f9668293c15.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_11174f9668293c15 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_11174f9668293c15 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_11174f9668293c15 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_95369e8e1a2ac256
  ${CMAKE_SOURCE_DIR}/generated/fuzz_95369e8e1a2ac256.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_95369e8e1a2ac256 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_95369e8e1a2ac256 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_95369e8e1a2ac256 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a1ddebf38cf69d56
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a1ddebf38cf69d56.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_a1ddebf38cf69d56 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a1ddebf38cf69d56 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a1ddebf38cf69d56 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_08972aa980b75d80
  ${CMAKE_SOURCE_DIR}/generated/fuzz_08972aa980b75d80.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_08972aa980b75d80 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_08972aa980b75d80 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_08972aa980b75d80 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e1face508a0a310e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e1face508a0a310e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_e1face508a0a310e PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e1face508a0a310e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e1face508a0a310e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1a0d2eac7aae4458
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1a0d2eac7aae4458.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_1a0d2eac7aae4458 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1a0d2eac7aae4458 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1a0d2eac7aae4458 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dd8cccb97b03abdc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dd8cccb97b03abdc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_dd8cccb97b03abdc PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dd8cccb97b03abdc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dd8cccb97b03abdc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_03a2ad0b1b96b80e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_03a2ad0b1b96b80e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_03a2ad0b1b96b80e PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_03a2ad0b1b96b80e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_03a2ad0b1b96b80e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e91e5c6aacd1ac6f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e91e5c6aacd1ac6f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_e91e5c6aacd1ac6f PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e91e5c6aacd1ac6f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e91e5c6aacd1ac6f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2a5a0e3f09dd0767
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2a5a0e3f09dd0767.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2a5a0e3f09dd0767 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2a5a0e3f09dd0767 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2a5a0e3f09dd0767 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0e8cd510f1ea0ed1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0e8cd510f1ea0ed1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_0e8cd510f1ea0ed1 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0e8cd510f1ea0ed1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0e8cd510f1ea0ed1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9a49349dca01662d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9a49349dca01662d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_9a49349dca01662d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9a49349dca01662d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9a49349dca01662d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2c64ade748379b15
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2c64ade748379b15.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2c64ade748379b15 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2c64ade748379b15 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2c64ade748379b15 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2751722201601f74
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2751722201601f74.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2751722201601f74 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2751722201601f74 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2751722201601f74 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_49e6b4240ac58072
  ${CMAKE_SOURCE_DIR}/generated/fuzz_49e6b4240ac58072.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_49e6b4240ac58072 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_49e6b4240ac58072 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_49e6b4240ac58072 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8591c040035ffe53
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8591c040035ffe53.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_8591c040035ffe53 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8591c040035ffe53 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8591c040035ffe53 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d8658f6f2181da36
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d8658f6f2181da36.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_d8658f6f2181da36 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d8658f6f2181da36 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d8658f6f2181da36 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1742b985acc88064
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1742b985acc88064.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_1742b985acc88064 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1742b985acc88064 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1742b985acc88064 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_88215b7f5467d7bb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_88215b7f5467d7bb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_88215b7f5467d7bb PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_88215b7f5467d7bb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_88215b7f5467d7bb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_36003efd900c1bb3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_36003efd900c1bb3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_36003efd900c1bb3 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_36003efd900c1bb3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_36003efd900c1bb3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_45d6cf68e8557e95
  ${CMAKE_SOURCE_DIR}/generated/fuzz_45d6cf68e8557e95.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_45d6cf68e8557e95 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_45d6cf68e8557e95 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_45d6cf68e8557e95 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b789e17c51364d40
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b789e17c51364d40.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_b789e17c51364d40 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b789e17c51364d40 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b789e17c51364d40 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bd9976182019cd30
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bd9976182019cd30.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_bd9976182019cd30 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bd9976182019cd30 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bd9976182019cd30 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b554e76a6fcd389b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b554e76a6fcd389b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_b554e76a6fcd389b PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b554e76a6fcd389b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b554e76a6fcd389b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2b704a0a36893be9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2b704a0a36893be9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_2b704a0a36893be9 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2b704a0a36893be9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2b704a0a36893be9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3c3b0109768674b5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3c3b0109768674b5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_3c3b0109768674b5 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3c3b0109768674b5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3c3b0109768674b5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4f418e8e44663595
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4f418e8e44663595.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_4f418e8e44663595 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4f418e8e44663595 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4f418e8e44663595 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c25a2ac526970dd2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c25a2ac526970dd2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_c25a2ac526970dd2 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c25a2ac526970dd2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c25a2ac526970dd2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dfe028c3d59c489d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dfe028c3d59c489d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_dfe028c3d59c489d PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dfe028c3d59c489d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dfe028c3d59c489d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_11a64fc0bddd05fb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_11a64fc0bddd05fb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_11a64fc0bddd05fb PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_11a64fc0bddd05fb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_11a64fc0bddd05fb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9f830d60c19f13a3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9f830d60c19f13a3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appmanager)
  target_link_libraries(fuzz_9f830d60c19f13a3 PRIVATE appmanager_fuzz_appmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9f830d60c19f13a3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9f830d60c19f13a3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d6fa561c9e585c7a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d6fa561c9e585c7a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_d6fa561c9e585c7a PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d6fa561c9e585c7a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d6fa561c9e585c7a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5734e0c4dc438961
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5734e0c4dc438961.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_5734e0c4dc438961 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5734e0c4dc438961 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5734e0c4dc438961 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ad8ad494220b3e61
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ad8ad494220b3e61.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_ad8ad494220b3e61 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ad8ad494220b3e61 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ad8ad494220b3e61 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8ea883d4696d97bf
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8ea883d4696d97bf.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_8ea883d4696d97bf PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8ea883d4696d97bf PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8ea883d4696d97bf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7d2e98e98f10af85
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7d2e98e98f10af85.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_7d2e98e98f10af85 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7d2e98e98f10af85 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7d2e98e98f10af85 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_659740c4e01b4254
  ${CMAKE_SOURCE_DIR}/generated/fuzz_659740c4e01b4254.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_659740c4e01b4254 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_659740c4e01b4254 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_659740c4e01b4254 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3ac70fc1c804b80d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3ac70fc1c804b80d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_3ac70fc1c804b80d PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3ac70fc1c804b80d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3ac70fc1c804b80d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9a1a1d934903da1e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9a1a1d934903da1e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_9a1a1d934903da1e PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9a1a1d934903da1e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9a1a1d934903da1e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_668c9383ce319211
  ${CMAKE_SOURCE_DIR}/generated/fuzz_668c9383ce319211.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_668c9383ce319211 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_668c9383ce319211 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_668c9383ce319211 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2a7ed650c9eb2d55
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2a7ed650c9eb2d55.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_2a7ed650c9eb2d55 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2a7ed650c9eb2d55 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2a7ed650c9eb2d55 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_235d5848f35b9b07
  ${CMAKE_SOURCE_DIR}/generated/fuzz_235d5848f35b9b07.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_235d5848f35b9b07 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_235d5848f35b9b07 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_235d5848f35b9b07 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_613426d447a455a6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_613426d447a455a6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_613426d447a455a6 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_613426d447a455a6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_613426d447a455a6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8ce3183a42515180
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8ce3183a42515180.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_8ce3183a42515180 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8ce3183a42515180 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8ce3183a42515180 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_50ec216b7f9d2a85
  ${CMAKE_SOURCE_DIR}/generated/fuzz_50ec216b7f9d2a85.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_50ec216b7f9d2a85 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_50ec216b7f9d2a85 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_50ec216b7f9d2a85 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6410e63929d35176
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6410e63929d35176.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_6410e63929d35176 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6410e63929d35176 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6410e63929d35176 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d1cde35a602e5ae2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d1cde35a602e5ae2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_d1cde35a602e5ae2 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d1cde35a602e5ae2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d1cde35a602e5ae2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_859cbb3ba421ea05
  ${CMAKE_SOURCE_DIR}/generated/fuzz_859cbb3ba421ea05.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_859cbb3ba421ea05 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_859cbb3ba421ea05 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_859cbb3ba421ea05 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0820600553ed3c7c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0820600553ed3c7c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_0820600553ed3c7c PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0820600553ed3c7c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0820600553ed3c7c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0e4c09c68903d4bb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0e4c09c68903d4bb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_0e4c09c68903d4bb PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0e4c09c68903d4bb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0e4c09c68903d4bb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cf9d1d93d98fa58f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cf9d1d93d98fa58f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_cf9d1d93d98fa58f PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cf9d1d93d98fa58f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cf9d1d93d98fa58f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fb9ac3193d9ce90f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fb9ac3193d9ce90f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_fb9ac3193d9ce90f PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fb9ac3193d9ce90f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fb9ac3193d9ce90f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_91a6c5859b8afb36
  ${CMAKE_SOURCE_DIR}/generated/fuzz_91a6c5859b8afb36.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_91a6c5859b8afb36 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_91a6c5859b8afb36 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_91a6c5859b8afb36 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_18ca68308ac81b66
  ${CMAKE_SOURCE_DIR}/generated/fuzz_18ca68308ac81b66.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_18ca68308ac81b66 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_18ca68308ac81b66 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_18ca68308ac81b66 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0dc6169ab97baf67
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0dc6169ab97baf67.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_0dc6169ab97baf67 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0dc6169ab97baf67 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0dc6169ab97baf67 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7bf386798e59b9bb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7bf386798e59b9bb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_7bf386798e59b9bb PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7bf386798e59b9bb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7bf386798e59b9bb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_17b8dcff4860401b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_17b8dcff4860401b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_17b8dcff4860401b PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_17b8dcff4860401b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_17b8dcff4860401b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8ac0ae83a0f1ee70
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8ac0ae83a0f1ee70.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_8ac0ae83a0f1ee70 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8ac0ae83a0f1ee70 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8ac0ae83a0f1ee70 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ac4611fa33b30c2a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ac4611fa33b30c2a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_ac4611fa33b30c2a PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ac4611fa33b30c2a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ac4611fa33b30c2a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_859034c5358c3caa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_859034c5358c3caa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_859034c5358c3caa PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_859034c5358c3caa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_859034c5358c3caa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3122873ee7b7e97b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3122873ee7b7e97b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_3122873ee7b7e97b PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3122873ee7b7e97b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3122873ee7b7e97b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9b9fec08cd8ea68c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9b9fec08cd8ea68c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_9b9fec08cd8ea68c PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9b9fec08cd8ea68c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9b9fec08cd8ea68c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_304d5f3a2d318998
  ${CMAKE_SOURCE_DIR}/generated/fuzz_304d5f3a2d318998.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_304d5f3a2d318998 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_304d5f3a2d318998 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_304d5f3a2d318998 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6d600946d8699212
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6d600946d8699212.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_6d600946d8699212 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6d600946d8699212 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6d600946d8699212 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_87603e16c444f623
  ${CMAKE_SOURCE_DIR}/generated/fuzz_87603e16c444f623.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_87603e16c444f623 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_87603e16c444f623 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_87603e16c444f623 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5c5e7be8e3487e26
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5c5e7be8e3487e26.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_5c5e7be8e3487e26 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5c5e7be8e3487e26 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5c5e7be8e3487e26 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_89610f4818407bca
  ${CMAKE_SOURCE_DIR}/generated/fuzz_89610f4818407bca.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_89610f4818407bca PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_89610f4818407bca PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_89610f4818407bca PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7a2aee49c96dc936
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7a2aee49c96dc936.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_7a2aee49c96dc936 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7a2aee49c96dc936 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7a2aee49c96dc936 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d545c9790eb102e6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d545c9790eb102e6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_d545c9790eb102e6 PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d545c9790eb102e6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d545c9790eb102e6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_51e558fed9aeef2b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_51e558fed9aeef2b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_51e558fed9aeef2b PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_51e558fed9aeef2b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_51e558fed9aeef2b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_91003e38ad0396af
  ${CMAKE_SOURCE_DIR}/generated/fuzz_91003e38ad0396af.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_appstoragemanager)
  target_link_libraries(fuzz_91003e38ad0396af PRIVATE appmanager_fuzz_appstoragemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_91003e38ad0396af PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_91003e38ad0396af PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_eb247202e0fdb25a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_eb247202e0fdb25a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_eb247202e0fdb25a PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_eb247202e0fdb25a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_eb247202e0fdb25a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_be481ce4f9b0b2cd
  ${CMAKE_SOURCE_DIR}/generated/fuzz_be481ce4f9b0b2cd.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_be481ce4f9b0b2cd PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_be481ce4f9b0b2cd PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_be481ce4f9b0b2cd PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_74c2c308bb6c41df
  ${CMAKE_SOURCE_DIR}/generated/fuzz_74c2c308bb6c41df.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_74c2c308bb6c41df PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_74c2c308bb6c41df PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_74c2c308bb6c41df PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c73a4647ebbf2bf3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c73a4647ebbf2bf3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_c73a4647ebbf2bf3 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c73a4647ebbf2bf3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c73a4647ebbf2bf3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d9c30865c738c7b6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d9c30865c738c7b6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_d9c30865c738c7b6 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d9c30865c738c7b6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d9c30865c738c7b6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4f5681fc147c2988
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4f5681fc147c2988.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_4f5681fc147c2988 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4f5681fc147c2988 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4f5681fc147c2988 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_10e240d8104a31ae
  ${CMAKE_SOURCE_DIR}/generated/fuzz_10e240d8104a31ae.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_10e240d8104a31ae PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_10e240d8104a31ae PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_10e240d8104a31ae PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e58ffdb13cd7f8d2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e58ffdb13cd7f8d2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_e58ffdb13cd7f8d2 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e58ffdb13cd7f8d2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e58ffdb13cd7f8d2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fc15e2f8c2e2aec2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fc15e2f8c2e2aec2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_fc15e2f8c2e2aec2 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fc15e2f8c2e2aec2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fc15e2f8c2e2aec2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_57a92e44e6695bdd
  ${CMAKE_SOURCE_DIR}/generated/fuzz_57a92e44e6695bdd.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_57a92e44e6695bdd PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_57a92e44e6695bdd PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_57a92e44e6695bdd PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bc999c5be1843c23
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bc999c5be1843c23.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_bc999c5be1843c23 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bc999c5be1843c23 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bc999c5be1843c23 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c4a516de96982ac9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c4a516de96982ac9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_c4a516de96982ac9 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c4a516de96982ac9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c4a516de96982ac9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_649b187b0fd73067
  ${CMAKE_SOURCE_DIR}/generated/fuzz_649b187b0fd73067.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_649b187b0fd73067 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_649b187b0fd73067 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_649b187b0fd73067 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_beb048631e3c6b1e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_beb048631e3c6b1e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_beb048631e3c6b1e PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_beb048631e3c6b1e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_beb048631e3c6b1e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e929bb9988da5ebc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e929bb9988da5ebc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_e929bb9988da5ebc PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e929bb9988da5ebc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e929bb9988da5ebc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2dd77b49712d1676
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2dd77b49712d1676.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_2dd77b49712d1676 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2dd77b49712d1676 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2dd77b49712d1676 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f1f1a1deb5ea1a37
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f1f1a1deb5ea1a37.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_f1f1a1deb5ea1a37 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f1f1a1deb5ea1a37 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f1f1a1deb5ea1a37 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c17b07a76133a7aa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c17b07a76133a7aa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_c17b07a76133a7aa PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c17b07a76133a7aa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c17b07a76133a7aa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_89f39219b856c548
  ${CMAKE_SOURCE_DIR}/generated/fuzz_89f39219b856c548.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_89f39219b856c548 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_89f39219b856c548 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_89f39219b856c548 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_be886e5e1bf63995
  ${CMAKE_SOURCE_DIR}/generated/fuzz_be886e5e1bf63995.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_be886e5e1bf63995 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_be886e5e1bf63995 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_be886e5e1bf63995 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1b99763ebe3eed33
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1b99763ebe3eed33.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_1b99763ebe3eed33 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1b99763ebe3eed33 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1b99763ebe3eed33 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f3228a488a73a06a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f3228a488a73a06a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_f3228a488a73a06a PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f3228a488a73a06a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f3228a488a73a06a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_aaed509e5052c55b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_aaed509e5052c55b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_aaed509e5052c55b PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_aaed509e5052c55b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_aaed509e5052c55b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c2c80389330c09eb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c2c80389330c09eb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_c2c80389330c09eb PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c2c80389330c09eb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c2c80389330c09eb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8717954e7513cd87
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8717954e7513cd87.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_8717954e7513cd87 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8717954e7513cd87 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8717954e7513cd87 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_85dc91823e60647b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_85dc91823e60647b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_85dc91823e60647b PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_85dc91823e60647b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_85dc91823e60647b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ae44b26621ff2d4c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ae44b26621ff2d4c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_ae44b26621ff2d4c PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ae44b26621ff2d4c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ae44b26621ff2d4c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_296ff8eaa6700528
  ${CMAKE_SOURCE_DIR}/generated/fuzz_296ff8eaa6700528.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_296ff8eaa6700528 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_296ff8eaa6700528 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_296ff8eaa6700528 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2a5edf067135c8e4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2a5edf067135c8e4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_2a5edf067135c8e4 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2a5edf067135c8e4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2a5edf067135c8e4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dc0036a595dfafc8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dc0036a595dfafc8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_dc0036a595dfafc8 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dc0036a595dfafc8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dc0036a595dfafc8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d37a45635b189a6d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d37a45635b189a6d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_d37a45635b189a6d PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d37a45635b189a6d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d37a45635b189a6d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_712461d67c5f654d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_712461d67c5f654d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_712461d67c5f654d PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_712461d67c5f654d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_712461d67c5f654d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_07f0362cb75a39c7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_07f0362cb75a39c7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_07f0362cb75a39c7 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_07f0362cb75a39c7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_07f0362cb75a39c7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0e532640115d9543
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0e532640115d9543.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_0e532640115d9543 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0e532640115d9543 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0e532640115d9543 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_aadbbefd06d1df8c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_aadbbefd06d1df8c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_aadbbefd06d1df8c PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_aadbbefd06d1df8c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_aadbbefd06d1df8c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e93c59fe581e172b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e93c59fe581e172b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_e93c59fe581e172b PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e93c59fe581e172b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e93c59fe581e172b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0734007e127785f3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0734007e127785f3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_0734007e127785f3 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0734007e127785f3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0734007e127785f3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_58c8086d1987dda3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_58c8086d1987dda3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_58c8086d1987dda3 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_58c8086d1987dda3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_58c8086d1987dda3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_579e97363bf49565
  ${CMAKE_SOURCE_DIR}/generated/fuzz_579e97363bf49565.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_579e97363bf49565 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_579e97363bf49565 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_579e97363bf49565 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7e8ef0ab35f567d6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7e8ef0ab35f567d6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_7e8ef0ab35f567d6 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7e8ef0ab35f567d6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7e8ef0ab35f567d6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_88241e7f8ed720d7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_88241e7f8ed720d7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_88241e7f8ed720d7 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_88241e7f8ed720d7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_88241e7f8ed720d7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3555dfbe488226f3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3555dfbe488226f3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_3555dfbe488226f3 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3555dfbe488226f3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3555dfbe488226f3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bbf0ce50c371ad4a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bbf0ce50c371ad4a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_bbf0ce50c371ad4a PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bbf0ce50c371ad4a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bbf0ce50c371ad4a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cc5708eb2946e012
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cc5708eb2946e012.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_cc5708eb2946e012 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cc5708eb2946e012 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cc5708eb2946e012 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0e8af133370f426d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0e8af133370f426d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_0e8af133370f426d PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0e8af133370f426d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0e8af133370f426d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2639a5e5a65b2c64
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2639a5e5a65b2c64.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_2639a5e5a65b2c64 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2639a5e5a65b2c64 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2639a5e5a65b2c64 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fc8b17b12e08a470
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fc8b17b12e08a470.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_fc8b17b12e08a470 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fc8b17b12e08a470 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fc8b17b12e08a470 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1e994f2311dc098c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1e994f2311dc098c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_1e994f2311dc098c PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1e994f2311dc098c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1e994f2311dc098c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_607a751a4ec20d54
  ${CMAKE_SOURCE_DIR}/generated/fuzz_607a751a4ec20d54.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_607a751a4ec20d54 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_607a751a4ec20d54 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_607a751a4ec20d54 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_559e4f79b3c354e3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_559e4f79b3c354e3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_559e4f79b3c354e3 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_559e4f79b3c354e3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_559e4f79b3c354e3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e0aae446e4d55978
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e0aae446e4d55978.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_e0aae446e4d55978 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e0aae446e4d55978 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e0aae446e4d55978 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d40e7bb12ac23ca6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d40e7bb12ac23ca6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_d40e7bb12ac23ca6 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d40e7bb12ac23ca6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d40e7bb12ac23ca6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9db12ff36ead78e0
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9db12ff36ead78e0.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_9db12ff36ead78e0 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9db12ff36ead78e0 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9db12ff36ead78e0 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6a260fd3dbbda151
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6a260fd3dbbda151.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_6a260fd3dbbda151 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6a260fd3dbbda151 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6a260fd3dbbda151 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f84a5ac5a59d57f9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f84a5ac5a59d57f9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_f84a5ac5a59d57f9 PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f84a5ac5a59d57f9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f84a5ac5a59d57f9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4dd9db26a1f339bf
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4dd9db26a1f339bf.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_downloadmanager)
  target_link_libraries(fuzz_4dd9db26a1f339bf PRIVATE appmanager_fuzz_downloadmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4dd9db26a1f339bf PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4dd9db26a1f339bf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bf08e17bf5c987fc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bf08e17bf5c987fc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_bf08e17bf5c987fc PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bf08e17bf5c987fc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bf08e17bf5c987fc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_40858e1ea60d4aa8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_40858e1ea60d4aa8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_40858e1ea60d4aa8 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_40858e1ea60d4aa8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_40858e1ea60d4aa8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_248a47d7815aeee8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_248a47d7815aeee8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_248a47d7815aeee8 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_248a47d7815aeee8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_248a47d7815aeee8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c0460df2af7d46d9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c0460df2af7d46d9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_c0460df2af7d46d9 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c0460df2af7d46d9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c0460df2af7d46d9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f304a88e7f9dda48
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f304a88e7f9dda48.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f304a88e7f9dda48 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f304a88e7f9dda48 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f304a88e7f9dda48 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b0c3c9ead1fbf2c6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b0c3c9ead1fbf2c6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_b0c3c9ead1fbf2c6 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b0c3c9ead1fbf2c6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b0c3c9ead1fbf2c6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7b4ac1a2de075d1b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7b4ac1a2de075d1b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_7b4ac1a2de075d1b PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7b4ac1a2de075d1b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7b4ac1a2de075d1b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_86900a47a4969ec4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_86900a47a4969ec4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_86900a47a4969ec4 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_86900a47a4969ec4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_86900a47a4969ec4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1361435f7ab22f11
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1361435f7ab22f11.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_1361435f7ab22f11 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1361435f7ab22f11 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1361435f7ab22f11 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e787a9081e1ebf08
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e787a9081e1ebf08.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_e787a9081e1ebf08 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e787a9081e1ebf08 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e787a9081e1ebf08 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_eea3e646d3ceb0a4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_eea3e646d3ceb0a4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_eea3e646d3ceb0a4 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_eea3e646d3ceb0a4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_eea3e646d3ceb0a4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7a9f7ce7e318f7df
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7a9f7ce7e318f7df.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_7a9f7ce7e318f7df PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7a9f7ce7e318f7df PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7a9f7ce7e318f7df PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_15a6c854912ccf84
  ${CMAKE_SOURCE_DIR}/generated/fuzz_15a6c854912ccf84.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_15a6c854912ccf84 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_15a6c854912ccf84 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_15a6c854912ccf84 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_119704d59e272f31
  ${CMAKE_SOURCE_DIR}/generated/fuzz_119704d59e272f31.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_119704d59e272f31 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_119704d59e272f31 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_119704d59e272f31 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c744d297ea42d11b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c744d297ea42d11b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_c744d297ea42d11b PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c744d297ea42d11b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c744d297ea42d11b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_419a164d22e178ab
  ${CMAKE_SOURCE_DIR}/generated/fuzz_419a164d22e178ab.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_419a164d22e178ab PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_419a164d22e178ab PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_419a164d22e178ab PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_507bdf86e7146c08
  ${CMAKE_SOURCE_DIR}/generated/fuzz_507bdf86e7146c08.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_507bdf86e7146c08 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_507bdf86e7146c08 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_507bdf86e7146c08 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9d25737fd25d4fe4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9d25737fd25d4fe4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_9d25737fd25d4fe4 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9d25737fd25d4fe4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9d25737fd25d4fe4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_064d382ec46c7b0f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_064d382ec46c7b0f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_064d382ec46c7b0f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_064d382ec46c7b0f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_064d382ec46c7b0f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fea64255e5fc243e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fea64255e5fc243e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_fea64255e5fc243e PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fea64255e5fc243e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fea64255e5fc243e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f64161de3772d7c7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f64161de3772d7c7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f64161de3772d7c7 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f64161de3772d7c7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f64161de3772d7c7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_da3841ecb40e7ed1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_da3841ecb40e7ed1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_da3841ecb40e7ed1 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_da3841ecb40e7ed1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_da3841ecb40e7ed1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b2406b1da4567dba
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b2406b1da4567dba.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_b2406b1da4567dba PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b2406b1da4567dba PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b2406b1da4567dba PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fdb0a7f98f41688c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fdb0a7f98f41688c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_fdb0a7f98f41688c PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fdb0a7f98f41688c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fdb0a7f98f41688c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5f8eae90af63635c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5f8eae90af63635c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_5f8eae90af63635c PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5f8eae90af63635c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5f8eae90af63635c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cf6f8e83f39cc4ed
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cf6f8e83f39cc4ed.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_cf6f8e83f39cc4ed PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cf6f8e83f39cc4ed PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cf6f8e83f39cc4ed PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1185d6f01dcf1ffa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1185d6f01dcf1ffa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_1185d6f01dcf1ffa PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1185d6f01dcf1ffa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1185d6f01dcf1ffa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1fe627c575fdc992
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1fe627c575fdc992.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_1fe627c575fdc992 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1fe627c575fdc992 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1fe627c575fdc992 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_833e41d512166729
  ${CMAKE_SOURCE_DIR}/generated/fuzz_833e41d512166729.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_833e41d512166729 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_833e41d512166729 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_833e41d512166729 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_52f7c89dc04efab3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_52f7c89dc04efab3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_52f7c89dc04efab3 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_52f7c89dc04efab3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_52f7c89dc04efab3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ccdebf09b4e644ad
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ccdebf09b4e644ad.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_ccdebf09b4e644ad PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ccdebf09b4e644ad PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ccdebf09b4e644ad PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ba2b2219c63e68aa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ba2b2219c63e68aa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_ba2b2219c63e68aa PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ba2b2219c63e68aa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ba2b2219c63e68aa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6d177e5d575ac262
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6d177e5d575ac262.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_6d177e5d575ac262 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6d177e5d575ac262 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6d177e5d575ac262 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_07235a55970967b6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_07235a55970967b6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_07235a55970967b6 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_07235a55970967b6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_07235a55970967b6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_eb42fad78299851d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_eb42fad78299851d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_eb42fad78299851d PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_eb42fad78299851d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_eb42fad78299851d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0c6076699f4f0c83
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0c6076699f4f0c83.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_0c6076699f4f0c83 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0c6076699f4f0c83 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0c6076699f4f0c83 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5ccb8c1f339a6bf9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5ccb8c1f339a6bf9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_5ccb8c1f339a6bf9 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5ccb8c1f339a6bf9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5ccb8c1f339a6bf9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0c2ed648aacd1217
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0c2ed648aacd1217.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_0c2ed648aacd1217 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0c2ed648aacd1217 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0c2ed648aacd1217 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f37ef7f1c117e7ba
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f37ef7f1c117e7ba.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f37ef7f1c117e7ba PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f37ef7f1c117e7ba PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f37ef7f1c117e7ba PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1489186fcb94e25d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1489186fcb94e25d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_1489186fcb94e25d PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1489186fcb94e25d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1489186fcb94e25d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7ebda478e9d8ab18
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7ebda478e9d8ab18.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_7ebda478e9d8ab18 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7ebda478e9d8ab18 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7ebda478e9d8ab18 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f2e5ea2e97de8b4f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f2e5ea2e97de8b4f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f2e5ea2e97de8b4f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f2e5ea2e97de8b4f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f2e5ea2e97de8b4f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e25ee6ed5de81872
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e25ee6ed5de81872.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_e25ee6ed5de81872 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e25ee6ed5de81872 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e25ee6ed5de81872 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6d91fba1abad6270
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6d91fba1abad6270.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_6d91fba1abad6270 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6d91fba1abad6270 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6d91fba1abad6270 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c7f53bffaf3392f3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c7f53bffaf3392f3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_c7f53bffaf3392f3 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c7f53bffaf3392f3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c7f53bffaf3392f3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f6f3902c78acb40d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f6f3902c78acb40d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f6f3902c78acb40d PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f6f3902c78acb40d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f6f3902c78acb40d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d7cd19b098914887
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d7cd19b098914887.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_d7cd19b098914887 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d7cd19b098914887 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d7cd19b098914887 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f10e6e343bb40590
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f10e6e343bb40590.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f10e6e343bb40590 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f10e6e343bb40590 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f10e6e343bb40590 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_da86536cc99d94c5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_da86536cc99d94c5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_da86536cc99d94c5 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_da86536cc99d94c5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_da86536cc99d94c5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f5fe85c1d91b7a3c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f5fe85c1d91b7a3c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f5fe85c1d91b7a3c PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f5fe85c1d91b7a3c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f5fe85c1d91b7a3c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_69b7063e74d66e2f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_69b7063e74d66e2f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_69b7063e74d66e2f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_69b7063e74d66e2f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_69b7063e74d66e2f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0822b23e4223f7d4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0822b23e4223f7d4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_0822b23e4223f7d4 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0822b23e4223f7d4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0822b23e4223f7d4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f7a1f8a0387debaf
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f7a1f8a0387debaf.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f7a1f8a0387debaf PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f7a1f8a0387debaf PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f7a1f8a0387debaf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0a57fe0d5cd180ff
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0a57fe0d5cd180ff.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_0a57fe0d5cd180ff PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0a57fe0d5cd180ff PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0a57fe0d5cd180ff PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_684362f2750b7c0d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_684362f2750b7c0d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_684362f2750b7c0d PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_684362f2750b7c0d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_684362f2750b7c0d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3fd8256d32c57b46
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3fd8256d32c57b46.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_3fd8256d32c57b46 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3fd8256d32c57b46 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3fd8256d32c57b46 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_56b14a0716a09639
  ${CMAKE_SOURCE_DIR}/generated/fuzz_56b14a0716a09639.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_56b14a0716a09639 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_56b14a0716a09639 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_56b14a0716a09639 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bfb285fb81818174
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bfb285fb81818174.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_bfb285fb81818174 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bfb285fb81818174 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bfb285fb81818174 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a92145c4ed392fe6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a92145c4ed392fe6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_a92145c4ed392fe6 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a92145c4ed392fe6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a92145c4ed392fe6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_25d2321070b12392
  ${CMAKE_SOURCE_DIR}/generated/fuzz_25d2321070b12392.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_25d2321070b12392 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_25d2321070b12392 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_25d2321070b12392 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ba426768247b9667
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ba426768247b9667.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_ba426768247b9667 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ba426768247b9667 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ba426768247b9667 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1351c5cb2b0b191f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1351c5cb2b0b191f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_1351c5cb2b0b191f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1351c5cb2b0b191f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1351c5cb2b0b191f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d6eda7b2a62c8857
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d6eda7b2a62c8857.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_d6eda7b2a62c8857 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d6eda7b2a62c8857 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d6eda7b2a62c8857 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1244da659eb2d3f4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1244da659eb2d3f4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_1244da659eb2d3f4 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1244da659eb2d3f4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1244da659eb2d3f4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7dfa73bbc686d436
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7dfa73bbc686d436.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_7dfa73bbc686d436 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7dfa73bbc686d436 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7dfa73bbc686d436 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_849a351e995081e1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_849a351e995081e1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_849a351e995081e1 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_849a351e995081e1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_849a351e995081e1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4582784b411c1559
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4582784b411c1559.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_4582784b411c1559 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4582784b411c1559 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4582784b411c1559 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_976f8fe860c39e3e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_976f8fe860c39e3e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_976f8fe860c39e3e PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_976f8fe860c39e3e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_976f8fe860c39e3e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f0ca64a02289253f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f0ca64a02289253f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f0ca64a02289253f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f0ca64a02289253f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f0ca64a02289253f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e1449596a5dbea3b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e1449596a5dbea3b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_e1449596a5dbea3b PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e1449596a5dbea3b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e1449596a5dbea3b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c2e04daf441cd51a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c2e04daf441cd51a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_c2e04daf441cd51a PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c2e04daf441cd51a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c2e04daf441cd51a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_05a5be8074ee5456
  ${CMAKE_SOURCE_DIR}/generated/fuzz_05a5be8074ee5456.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_05a5be8074ee5456 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_05a5be8074ee5456 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_05a5be8074ee5456 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_956cdf74e4f8ffaf
  ${CMAKE_SOURCE_DIR}/generated/fuzz_956cdf74e4f8ffaf.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_956cdf74e4f8ffaf PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_956cdf74e4f8ffaf PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_956cdf74e4f8ffaf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e6e839b111c68b3d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e6e839b111c68b3d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_e6e839b111c68b3d PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e6e839b111c68b3d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e6e839b111c68b3d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2af47fc78b9a6353
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2af47fc78b9a6353.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_2af47fc78b9a6353 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2af47fc78b9a6353 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2af47fc78b9a6353 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_495f3d7ca0fb481e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_495f3d7ca0fb481e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_495f3d7ca0fb481e PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_495f3d7ca0fb481e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_495f3d7ca0fb481e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_440fffdf7644036e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_440fffdf7644036e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_440fffdf7644036e PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_440fffdf7644036e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_440fffdf7644036e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_854d1b5951db3786
  ${CMAKE_SOURCE_DIR}/generated/fuzz_854d1b5951db3786.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_854d1b5951db3786 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_854d1b5951db3786 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_854d1b5951db3786 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a275f223cea25185
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a275f223cea25185.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_a275f223cea25185 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a275f223cea25185 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a275f223cea25185 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b4342be47d5fdbdd
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b4342be47d5fdbdd.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_b4342be47d5fdbdd PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b4342be47d5fdbdd PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b4342be47d5fdbdd PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2b69e8a5964f1352
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2b69e8a5964f1352.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_2b69e8a5964f1352 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2b69e8a5964f1352 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2b69e8a5964f1352 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_797f178bb0f239ef
  ${CMAKE_SOURCE_DIR}/generated/fuzz_797f178bb0f239ef.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_797f178bb0f239ef PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_797f178bb0f239ef PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_797f178bb0f239ef PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f4be5a820c4c3faa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f4be5a820c4c3faa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f4be5a820c4c3faa PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f4be5a820c4c3faa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f4be5a820c4c3faa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f85989038e3036bc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f85989038e3036bc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f85989038e3036bc PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f85989038e3036bc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f85989038e3036bc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_44fe00fca958fafa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_44fe00fca958fafa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_44fe00fca958fafa PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_44fe00fca958fafa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_44fe00fca958fafa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b6b7f04fb37dd6d6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b6b7f04fb37dd6d6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_b6b7f04fb37dd6d6 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b6b7f04fb37dd6d6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b6b7f04fb37dd6d6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_924273e428994322
  ${CMAKE_SOURCE_DIR}/generated/fuzz_924273e428994322.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_924273e428994322 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_924273e428994322 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_924273e428994322 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9319ef2ec04f9df3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9319ef2ec04f9df3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_9319ef2ec04f9df3 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9319ef2ec04f9df3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9319ef2ec04f9df3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_de83f68f2df6273e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_de83f68f2df6273e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_de83f68f2df6273e PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_de83f68f2df6273e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_de83f68f2df6273e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a0e32c04ded04cea
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a0e32c04ded04cea.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_a0e32c04ded04cea PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a0e32c04ded04cea PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a0e32c04ded04cea PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fb0a4230c7f125c9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fb0a4230c7f125c9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_fb0a4230c7f125c9 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fb0a4230c7f125c9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fb0a4230c7f125c9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fa806afc7dd620e6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fa806afc7dd620e6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_fa806afc7dd620e6 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fa806afc7dd620e6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fa806afc7dd620e6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_53660eccba8250f1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_53660eccba8250f1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_53660eccba8250f1 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_53660eccba8250f1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_53660eccba8250f1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f899bfd5174605b4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f899bfd5174605b4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f899bfd5174605b4 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f899bfd5174605b4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f899bfd5174605b4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8590147af1aa8b05
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8590147af1aa8b05.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_8590147af1aa8b05 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8590147af1aa8b05 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8590147af1aa8b05 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_01ff7b16fdcf79b8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_01ff7b16fdcf79b8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_01ff7b16fdcf79b8 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_01ff7b16fdcf79b8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_01ff7b16fdcf79b8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b5f6801da9dd3b0d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b5f6801da9dd3b0d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_b5f6801da9dd3b0d PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b5f6801da9dd3b0d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b5f6801da9dd3b0d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2d6060e54f77743a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2d6060e54f77743a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_2d6060e54f77743a PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2d6060e54f77743a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2d6060e54f77743a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6bfd6336563644aa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6bfd6336563644aa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_6bfd6336563644aa PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6bfd6336563644aa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6bfd6336563644aa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_30e2b38e10033407
  ${CMAKE_SOURCE_DIR}/generated/fuzz_30e2b38e10033407.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_30e2b38e10033407 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_30e2b38e10033407 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_30e2b38e10033407 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0dc5b48dcd16f436
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0dc5b48dcd16f436.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_0dc5b48dcd16f436 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0dc5b48dcd16f436 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0dc5b48dcd16f436 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_25eb262f6212205a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_25eb262f6212205a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_25eb262f6212205a PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_25eb262f6212205a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_25eb262f6212205a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5958fb234812f9c4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5958fb234812f9c4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_5958fb234812f9c4 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5958fb234812f9c4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5958fb234812f9c4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e2aa3275be6d6145
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e2aa3275be6d6145.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_e2aa3275be6d6145 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e2aa3275be6d6145 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e2aa3275be6d6145 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c47089444ba3f880
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c47089444ba3f880.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_c47089444ba3f880 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c47089444ba3f880 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c47089444ba3f880 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_345c6cd177d5a8f2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_345c6cd177d5a8f2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_345c6cd177d5a8f2 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_345c6cd177d5a8f2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_345c6cd177d5a8f2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cbca543f61ae04f6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cbca543f61ae04f6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_cbca543f61ae04f6 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cbca543f61ae04f6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cbca543f61ae04f6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5ae18f80365cfcc2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5ae18f80365cfcc2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_5ae18f80365cfcc2 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5ae18f80365cfcc2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5ae18f80365cfcc2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e57d5eb8c026f171
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e57d5eb8c026f171.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_e57d5eb8c026f171 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e57d5eb8c026f171 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e57d5eb8c026f171 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0ab65bbcd9388d9d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0ab65bbcd9388d9d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_0ab65bbcd9388d9d PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0ab65bbcd9388d9d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0ab65bbcd9388d9d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_281f25d985fd53b5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_281f25d985fd53b5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_281f25d985fd53b5 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_281f25d985fd53b5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_281f25d985fd53b5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1d72f632d78b588f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1d72f632d78b588f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_1d72f632d78b588f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1d72f632d78b588f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1d72f632d78b588f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f1572cec8d109f77
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f1572cec8d109f77.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f1572cec8d109f77 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f1572cec8d109f77 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f1572cec8d109f77 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b2c4dad1cd1ad0d7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b2c4dad1cd1ad0d7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_b2c4dad1cd1ad0d7 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b2c4dad1cd1ad0d7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b2c4dad1cd1ad0d7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e3321afb586840b7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e3321afb586840b7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_e3321afb586840b7 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e3321afb586840b7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e3321afb586840b7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4e26056b5d958e93
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4e26056b5d958e93.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_4e26056b5d958e93 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4e26056b5d958e93 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4e26056b5d958e93 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a74c0d816e8b5b3a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a74c0d816e8b5b3a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_a74c0d816e8b5b3a PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a74c0d816e8b5b3a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a74c0d816e8b5b3a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bbcdd83af7e31441
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bbcdd83af7e31441.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_bbcdd83af7e31441 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bbcdd83af7e31441 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bbcdd83af7e31441 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f8e2e674e077458d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f8e2e674e077458d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f8e2e674e077458d PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f8e2e674e077458d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f8e2e674e077458d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_10a2a58089993d95
  ${CMAKE_SOURCE_DIR}/generated/fuzz_10a2a58089993d95.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_10a2a58089993d95 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_10a2a58089993d95 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_10a2a58089993d95 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_653518b0cd3b8371
  ${CMAKE_SOURCE_DIR}/generated/fuzz_653518b0cd3b8371.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_653518b0cd3b8371 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_653518b0cd3b8371 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_653518b0cd3b8371 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4dd0b621ff1f4966
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4dd0b621ff1f4966.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_4dd0b621ff1f4966 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4dd0b621ff1f4966 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4dd0b621ff1f4966 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b7979d6f01ae76f6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b7979d6f01ae76f6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_b7979d6f01ae76f6 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b7979d6f01ae76f6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b7979d6f01ae76f6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bace6aade2d15ea6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bace6aade2d15ea6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_bace6aade2d15ea6 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bace6aade2d15ea6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bace6aade2d15ea6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d75fb0b0e18d2a66
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d75fb0b0e18d2a66.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_d75fb0b0e18d2a66 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d75fb0b0e18d2a66 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d75fb0b0e18d2a66 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_675d72a5f64392ff
  ${CMAKE_SOURCE_DIR}/generated/fuzz_675d72a5f64392ff.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_675d72a5f64392ff PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_675d72a5f64392ff PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_675d72a5f64392ff PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4039e520ddfcc4f3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4039e520ddfcc4f3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_4039e520ddfcc4f3 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4039e520ddfcc4f3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4039e520ddfcc4f3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c0f956d461f927bd
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c0f956d461f927bd.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_c0f956d461f927bd PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c0f956d461f927bd PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c0f956d461f927bd PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_13a83f227bb7a332
  ${CMAKE_SOURCE_DIR}/generated/fuzz_13a83f227bb7a332.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_13a83f227bb7a332 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_13a83f227bb7a332 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_13a83f227bb7a332 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dac34850854ba29f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dac34850854ba29f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_dac34850854ba29f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dac34850854ba29f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dac34850854ba29f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c09b18d6ce896682
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c09b18d6ce896682.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_c09b18d6ce896682 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c09b18d6ce896682 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c09b18d6ce896682 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3234c3e1c37c300b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3234c3e1c37c300b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_3234c3e1c37c300b PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3234c3e1c37c300b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3234c3e1c37c300b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_23faaf08060ad767
  ${CMAKE_SOURCE_DIR}/generated/fuzz_23faaf08060ad767.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_23faaf08060ad767 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_23faaf08060ad767 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_23faaf08060ad767 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_df21f4ee22b7a4bb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_df21f4ee22b7a4bb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_df21f4ee22b7a4bb PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_df21f4ee22b7a4bb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_df21f4ee22b7a4bb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_56885015a06de764
  ${CMAKE_SOURCE_DIR}/generated/fuzz_56885015a06de764.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_56885015a06de764 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_56885015a06de764 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_56885015a06de764 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ecf9c164e10e4731
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ecf9c164e10e4731.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_ecf9c164e10e4731 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ecf9c164e10e4731 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ecf9c164e10e4731 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7c5ea18a38848915
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7c5ea18a38848915.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_7c5ea18a38848915 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7c5ea18a38848915 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7c5ea18a38848915 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_434d32421fd5d976
  ${CMAKE_SOURCE_DIR}/generated/fuzz_434d32421fd5d976.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_434d32421fd5d976 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_434d32421fd5d976 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_434d32421fd5d976 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_25f81f5c4eaff59b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_25f81f5c4eaff59b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_25f81f5c4eaff59b PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_25f81f5c4eaff59b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_25f81f5c4eaff59b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ec13d666dd08430f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ec13d666dd08430f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_ec13d666dd08430f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ec13d666dd08430f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ec13d666dd08430f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7f5d9d031aa57466
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7f5d9d031aa57466.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_7f5d9d031aa57466 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7f5d9d031aa57466 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7f5d9d031aa57466 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5f36e8d829499c0e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5f36e8d829499c0e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_5f36e8d829499c0e PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5f36e8d829499c0e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5f36e8d829499c0e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f5f26b14767882a5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f5f26b14767882a5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f5f26b14767882a5 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f5f26b14767882a5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f5f26b14767882a5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9963679c1dd3026b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9963679c1dd3026b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_9963679c1dd3026b PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9963679c1dd3026b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9963679c1dd3026b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9ebaa19f1a87ba93
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9ebaa19f1a87ba93.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_9ebaa19f1a87ba93 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9ebaa19f1a87ba93 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9ebaa19f1a87ba93 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cf80625f6e8da0ac
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cf80625f6e8da0ac.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_cf80625f6e8da0ac PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cf80625f6e8da0ac PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cf80625f6e8da0ac PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f3fad676ee1293a8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f3fad676ee1293a8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_f3fad676ee1293a8 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f3fad676ee1293a8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f3fad676ee1293a8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c1f010ced57d13f5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c1f010ced57d13f5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_c1f010ced57d13f5 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c1f010ced57d13f5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c1f010ced57d13f5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_59543aff4f0d0ba9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_59543aff4f0d0ba9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_59543aff4f0d0ba9 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_59543aff4f0d0ba9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_59543aff4f0d0ba9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cae70bd22b76d4d3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cae70bd22b76d4d3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_cae70bd22b76d4d3 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cae70bd22b76d4d3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cae70bd22b76d4d3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d0c3b9f6f6e6a505
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d0c3b9f6f6e6a505.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_d0c3b9f6f6e6a505 PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d0c3b9f6f6e6a505 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d0c3b9f6f6e6a505 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2af645f3dd4de01f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2af645f3dd4de01f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_2af645f3dd4de01f PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2af645f3dd4de01f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2af645f3dd4de01f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_068479dcbe591e8a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_068479dcbe591e8a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_lifecyclemanager)
  target_link_libraries(fuzz_068479dcbe591e8a PRIVATE appmanager_fuzz_lifecyclemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_068479dcbe591e8a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_068479dcbe591e8a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d74d03b514e07aec
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d74d03b514e07aec.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_d74d03b514e07aec PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d74d03b514e07aec PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d74d03b514e07aec PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7cdcafa98bc9d839
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7cdcafa98bc9d839.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_7cdcafa98bc9d839 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7cdcafa98bc9d839 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7cdcafa98bc9d839 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_88f70df16503103d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_88f70df16503103d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_88f70df16503103d PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_88f70df16503103d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_88f70df16503103d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8061e923a9e3dc74
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8061e923a9e3dc74.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_8061e923a9e3dc74 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8061e923a9e3dc74 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8061e923a9e3dc74 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f011387bd4ba56c5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f011387bd4ba56c5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_f011387bd4ba56c5 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f011387bd4ba56c5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f011387bd4ba56c5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ebdcd925ef6b9f21
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ebdcd925ef6b9f21.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_ebdcd925ef6b9f21 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ebdcd925ef6b9f21 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ebdcd925ef6b9f21 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cb80d4deeca947ad
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cb80d4deeca947ad.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_cb80d4deeca947ad PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cb80d4deeca947ad PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cb80d4deeca947ad PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_976b310a36f4cc6d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_976b310a36f4cc6d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_976b310a36f4cc6d PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_976b310a36f4cc6d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_976b310a36f4cc6d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a6e535f1c22a95eb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a6e535f1c22a95eb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_a6e535f1c22a95eb PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a6e535f1c22a95eb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a6e535f1c22a95eb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_63b6206af9904d39
  ${CMAKE_SOURCE_DIR}/generated/fuzz_63b6206af9904d39.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_63b6206af9904d39 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_63b6206af9904d39 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_63b6206af9904d39 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6e6c3bded33f32b5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6e6c3bded33f32b5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_6e6c3bded33f32b5 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6e6c3bded33f32b5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6e6c3bded33f32b5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9e75e82df69eb640
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9e75e82df69eb640.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_9e75e82df69eb640 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9e75e82df69eb640 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9e75e82df69eb640 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cf9cc29dc4717e2b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cf9cc29dc4717e2b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_cf9cc29dc4717e2b PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cf9cc29dc4717e2b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cf9cc29dc4717e2b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5b6b268874dd32d5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5b6b268874dd32d5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_5b6b268874dd32d5 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5b6b268874dd32d5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5b6b268874dd32d5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8e44750d80ec0460
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8e44750d80ec0460.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_8e44750d80ec0460 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8e44750d80ec0460 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8e44750d80ec0460 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_34d14a64f1054c5f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_34d14a64f1054c5f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_34d14a64f1054c5f PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_34d14a64f1054c5f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_34d14a64f1054c5f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_05ecd1cfd0f3fc12
  ${CMAKE_SOURCE_DIR}/generated/fuzz_05ecd1cfd0f3fc12.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_05ecd1cfd0f3fc12 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_05ecd1cfd0f3fc12 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_05ecd1cfd0f3fc12 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f443c738eb70d035
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f443c738eb70d035.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_f443c738eb70d035 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f443c738eb70d035 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f443c738eb70d035 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8d90dfe40990a50d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8d90dfe40990a50d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_8d90dfe40990a50d PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8d90dfe40990a50d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8d90dfe40990a50d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3cb4475daabc4fb7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3cb4475daabc4fb7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_3cb4475daabc4fb7 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3cb4475daabc4fb7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3cb4475daabc4fb7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_106e7250945f622d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_106e7250945f622d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_106e7250945f622d PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_106e7250945f622d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_106e7250945f622d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_00eadbe2b2aba4f5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_00eadbe2b2aba4f5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_00eadbe2b2aba4f5 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_00eadbe2b2aba4f5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_00eadbe2b2aba4f5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b602a5130be4bd34
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b602a5130be4bd34.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_b602a5130be4bd34 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b602a5130be4bd34 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b602a5130be4bd34 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c06dc2c804543fce
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c06dc2c804543fce.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_c06dc2c804543fce PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c06dc2c804543fce PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c06dc2c804543fce PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_26318fb65645007c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_26318fb65645007c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_26318fb65645007c PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_26318fb65645007c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_26318fb65645007c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fe8ece1a22d8fd0a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fe8ece1a22d8fd0a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_fe8ece1a22d8fd0a PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fe8ece1a22d8fd0a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fe8ece1a22d8fd0a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_54bbe691bcacf50f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_54bbe691bcacf50f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_54bbe691bcacf50f PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_54bbe691bcacf50f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_54bbe691bcacf50f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3cfa9f9d28df3206
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3cfa9f9d28df3206.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_3cfa9f9d28df3206 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3cfa9f9d28df3206 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3cfa9f9d28df3206 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e917735071527db4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e917735071527db4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_e917735071527db4 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e917735071527db4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e917735071527db4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_72bbf8e6829dc210
  ${CMAKE_SOURCE_DIR}/generated/fuzz_72bbf8e6829dc210.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_72bbf8e6829dc210 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_72bbf8e6829dc210 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_72bbf8e6829dc210 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_eded3db3134dcf48
  ${CMAKE_SOURCE_DIR}/generated/fuzz_eded3db3134dcf48.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_eded3db3134dcf48 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_eded3db3134dcf48 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_eded3db3134dcf48 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ac455cb07e0be5e5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ac455cb07e0be5e5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_ac455cb07e0be5e5 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ac455cb07e0be5e5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ac455cb07e0be5e5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fa5398954b89035f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fa5398954b89035f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_fa5398954b89035f PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fa5398954b89035f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fa5398954b89035f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3451dfa7982d1f0b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3451dfa7982d1f0b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_3451dfa7982d1f0b PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3451dfa7982d1f0b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3451dfa7982d1f0b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bcf6337a61585da4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bcf6337a61585da4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_bcf6337a61585da4 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bcf6337a61585da4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bcf6337a61585da4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_21235c889b96d746
  ${CMAKE_SOURCE_DIR}/generated/fuzz_21235c889b96d746.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_21235c889b96d746 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_21235c889b96d746 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_21235c889b96d746 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b4042f3b5ed3b7d5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b4042f3b5ed3b7d5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_b4042f3b5ed3b7d5 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b4042f3b5ed3b7d5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b4042f3b5ed3b7d5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3cd4d652db14fa16
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3cd4d652db14fa16.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_3cd4d652db14fa16 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3cd4d652db14fa16 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3cd4d652db14fa16 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_40e94c214b29efe9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_40e94c214b29efe9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_40e94c214b29efe9 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_40e94c214b29efe9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_40e94c214b29efe9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_07442c3dd7fe4401
  ${CMAKE_SOURCE_DIR}/generated/fuzz_07442c3dd7fe4401.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_07442c3dd7fe4401 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_07442c3dd7fe4401 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_07442c3dd7fe4401 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ebaf684dc3367b11
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ebaf684dc3367b11.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_ebaf684dc3367b11 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ebaf684dc3367b11 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ebaf684dc3367b11 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4b880885972aa7d4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4b880885972aa7d4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_4b880885972aa7d4 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4b880885972aa7d4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4b880885972aa7d4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_63d812b5638f820e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_63d812b5638f820e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_63d812b5638f820e PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_63d812b5638f820e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_63d812b5638f820e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4d8b390cc0293b16
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4d8b390cc0293b16.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_4d8b390cc0293b16 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4d8b390cc0293b16 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4d8b390cc0293b16 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b517e41f40984291
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b517e41f40984291.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_b517e41f40984291 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b517e41f40984291 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b517e41f40984291 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_aa39b53d122b9454
  ${CMAKE_SOURCE_DIR}/generated/fuzz_aa39b53d122b9454.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_aa39b53d122b9454 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_aa39b53d122b9454 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_aa39b53d122b9454 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ccc0dfacf4e446d3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ccc0dfacf4e446d3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_ccc0dfacf4e446d3 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ccc0dfacf4e446d3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ccc0dfacf4e446d3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1009a80e7f8973d4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1009a80e7f8973d4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_1009a80e7f8973d4 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1009a80e7f8973d4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1009a80e7f8973d4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_26826bb758ae70b0
  ${CMAKE_SOURCE_DIR}/generated/fuzz_26826bb758ae70b0.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_26826bb758ae70b0 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_26826bb758ae70b0 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_26826bb758ae70b0 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f60ce5565da4a1c1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f60ce5565da4a1c1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_f60ce5565da4a1c1 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f60ce5565da4a1c1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f60ce5565da4a1c1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dc81c02510ad2e42
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dc81c02510ad2e42.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_dc81c02510ad2e42 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dc81c02510ad2e42 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dc81c02510ad2e42 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_69172a62cd7cf876
  ${CMAKE_SOURCE_DIR}/generated/fuzz_69172a62cd7cf876.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_69172a62cd7cf876 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_69172a62cd7cf876 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_69172a62cd7cf876 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8f92aac30723f944
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8f92aac30723f944.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_8f92aac30723f944 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8f92aac30723f944 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8f92aac30723f944 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ff3482c93300e064
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ff3482c93300e064.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_ff3482c93300e064 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ff3482c93300e064 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ff3482c93300e064 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c764a6f280b56c7e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c764a6f280b56c7e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_c764a6f280b56c7e PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c764a6f280b56c7e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c764a6f280b56c7e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_614a175422e4d98d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_614a175422e4d98d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_614a175422e4d98d PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_614a175422e4d98d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_614a175422e4d98d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0ff1f257e2a16fa1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0ff1f257e2a16fa1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_0ff1f257e2a16fa1 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0ff1f257e2a16fa1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0ff1f257e2a16fa1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f1a3924f932e8827
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f1a3924f932e8827.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_f1a3924f932e8827 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f1a3924f932e8827 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f1a3924f932e8827 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b51f022d001f8c8f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b51f022d001f8c8f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_b51f022d001f8c8f PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b51f022d001f8c8f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b51f022d001f8c8f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e28355010c167e82
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e28355010c167e82.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_e28355010c167e82 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e28355010c167e82 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e28355010c167e82 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a9e475035cb70f8a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a9e475035cb70f8a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_a9e475035cb70f8a PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a9e475035cb70f8a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a9e475035cb70f8a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_addaa6712fa30f7e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_addaa6712fa30f7e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_addaa6712fa30f7e PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_addaa6712fa30f7e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_addaa6712fa30f7e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e68b1b07b8a9c735
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e68b1b07b8a9c735.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_e68b1b07b8a9c735 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e68b1b07b8a9c735 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e68b1b07b8a9c735 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fce7ee4a83e898e7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fce7ee4a83e898e7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_fce7ee4a83e898e7 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fce7ee4a83e898e7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fce7ee4a83e898e7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4a31765f07d1af56
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4a31765f07d1af56.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_4a31765f07d1af56 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4a31765f07d1af56 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4a31765f07d1af56 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9acf639fa3128151
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9acf639fa3128151.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_9acf639fa3128151 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9acf639fa3128151 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9acf639fa3128151 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a1ffbd9dc8ba2d1d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a1ffbd9dc8ba2d1d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_a1ffbd9dc8ba2d1d PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a1ffbd9dc8ba2d1d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a1ffbd9dc8ba2d1d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3b83f1a03a32517b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3b83f1a03a32517b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_3b83f1a03a32517b PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3b83f1a03a32517b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3b83f1a03a32517b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d3e61143edb7bb5e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d3e61143edb7bb5e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_d3e61143edb7bb5e PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d3e61143edb7bb5e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d3e61143edb7bb5e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9364cbd1fde200fa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9364cbd1fde200fa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_9364cbd1fde200fa PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9364cbd1fde200fa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9364cbd1fde200fa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b9a4848f8be63695
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b9a4848f8be63695.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_b9a4848f8be63695 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b9a4848f8be63695 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b9a4848f8be63695 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a489ead1c24b0fc1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a489ead1c24b0fc1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_a489ead1c24b0fc1 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a489ead1c24b0fc1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a489ead1c24b0fc1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0b052cddea7ade79
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0b052cddea7ade79.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_0b052cddea7ade79 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0b052cddea7ade79 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0b052cddea7ade79 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_144a7107486f5a30
  ${CMAKE_SOURCE_DIR}/generated/fuzz_144a7107486f5a30.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_144a7107486f5a30 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_144a7107486f5a30 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_144a7107486f5a30 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f83f286a20d15d0f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f83f286a20d15d0f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_f83f286a20d15d0f PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f83f286a20d15d0f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f83f286a20d15d0f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fdeb3ccba40f48a7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fdeb3ccba40f48a7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_fdeb3ccba40f48a7 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fdeb3ccba40f48a7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fdeb3ccba40f48a7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8c2c7b04a914e195
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8c2c7b04a914e195.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_8c2c7b04a914e195 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8c2c7b04a914e195 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8c2c7b04a914e195 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_192147736e94d6d0
  ${CMAKE_SOURCE_DIR}/generated/fuzz_192147736e94d6d0.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_192147736e94d6d0 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_192147736e94d6d0 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_192147736e94d6d0 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_02427f79dc30e319
  ${CMAKE_SOURCE_DIR}/generated/fuzz_02427f79dc30e319.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_02427f79dc30e319 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_02427f79dc30e319 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_02427f79dc30e319 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_119fe3754e4429d6
  ${CMAKE_SOURCE_DIR}/generated/fuzz_119fe3754e4429d6.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_119fe3754e4429d6 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_119fe3754e4429d6 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_119fe3754e4429d6 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_34925d4ab4785279
  ${CMAKE_SOURCE_DIR}/generated/fuzz_34925d4ab4785279.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_34925d4ab4785279 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_34925d4ab4785279 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_34925d4ab4785279 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cab7310ec5a60754
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cab7310ec5a60754.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_cab7310ec5a60754 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cab7310ec5a60754 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cab7310ec5a60754 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e0b17a0115e4130f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e0b17a0115e4130f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_e0b17a0115e4130f PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e0b17a0115e4130f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e0b17a0115e4130f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d3de6eb6d1504762
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d3de6eb6d1504762.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_d3de6eb6d1504762 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d3de6eb6d1504762 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d3de6eb6d1504762 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_4e0bf61900eecdc4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_4e0bf61900eecdc4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_4e0bf61900eecdc4 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_4e0bf61900eecdc4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_4e0bf61900eecdc4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_00303857fa252dcd
  ${CMAKE_SOURCE_DIR}/generated/fuzz_00303857fa252dcd.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_00303857fa252dcd PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_00303857fa252dcd PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_00303857fa252dcd PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6209e01eade431df
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6209e01eade431df.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_6209e01eade431df PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6209e01eade431df PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6209e01eade431df PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_deebddec8da90c26
  ${CMAKE_SOURCE_DIR}/generated/fuzz_deebddec8da90c26.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_deebddec8da90c26 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_deebddec8da90c26 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_deebddec8da90c26 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_50e778805f2fae51
  ${CMAKE_SOURCE_DIR}/generated/fuzz_50e778805f2fae51.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_packagemanager)
  target_link_libraries(fuzz_50e778805f2fae51 PRIVATE appmanager_fuzz_packagemanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_50e778805f2fae51 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_50e778805f2fae51 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a4063488c7590a52
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a4063488c7590a52.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_a4063488c7590a52 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a4063488c7590a52 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a4063488c7590a52 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5dcf90c8f59d078d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5dcf90c8f59d078d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_5dcf90c8f59d078d PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5dcf90c8f59d078d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5dcf90c8f59d078d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9ddfb0cf72e956c2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9ddfb0cf72e956c2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_9ddfb0cf72e956c2 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9ddfb0cf72e956c2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9ddfb0cf72e956c2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_973c14b8d6d828ea
  ${CMAKE_SOURCE_DIR}/generated/fuzz_973c14b8d6d828ea.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_973c14b8d6d828ea PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_973c14b8d6d828ea PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_973c14b8d6d828ea PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7d30e2d4149d46f9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7d30e2d4149d46f9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_7d30e2d4149d46f9 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7d30e2d4149d46f9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7d30e2d4149d46f9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cf8d59827f8c7660
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cf8d59827f8c7660.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_cf8d59827f8c7660 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cf8d59827f8c7660 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cf8d59827f8c7660 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_682f38dabdc7db7b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_682f38dabdc7db7b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_682f38dabdc7db7b PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_682f38dabdc7db7b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_682f38dabdc7db7b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7b9842eb5ae629aa
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7b9842eb5ae629aa.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_7b9842eb5ae629aa PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7b9842eb5ae629aa PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7b9842eb5ae629aa PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bed9c16779b47fbc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bed9c16779b47fbc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_bed9c16779b47fbc PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bed9c16779b47fbc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bed9c16779b47fbc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7866000de0b3d515
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7866000de0b3d515.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_7866000de0b3d515 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7866000de0b3d515 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7866000de0b3d515 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_db902d2fedc034f7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_db902d2fedc034f7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_db902d2fedc034f7 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_db902d2fedc034f7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_db902d2fedc034f7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_99954a01af9ad9f3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_99954a01af9ad9f3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_99954a01af9ad9f3 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_99954a01af9ad9f3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_99954a01af9ad9f3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_211a9895664b7ed5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_211a9895664b7ed5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_211a9895664b7ed5 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_211a9895664b7ed5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_211a9895664b7ed5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2757e362ac83045c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2757e362ac83045c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_2757e362ac83045c PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2757e362ac83045c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2757e362ac83045c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a28d0fe7e7c90511
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a28d0fe7e7c90511.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_a28d0fe7e7c90511 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a28d0fe7e7c90511 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a28d0fe7e7c90511 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_50e023120c3ee087
  ${CMAKE_SOURCE_DIR}/generated/fuzz_50e023120c3ee087.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_50e023120c3ee087 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_50e023120c3ee087 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_50e023120c3ee087 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b2c7358c90223d38
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b2c7358c90223d38.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_b2c7358c90223d38 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b2c7358c90223d38 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b2c7358c90223d38 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_536c92c6417082fc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_536c92c6417082fc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_536c92c6417082fc PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_536c92c6417082fc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_536c92c6417082fc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_01abb25b5dbdc7a8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_01abb25b5dbdc7a8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_01abb25b5dbdc7a8 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_01abb25b5dbdc7a8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_01abb25b5dbdc7a8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_59bc0911a60cee0b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_59bc0911a60cee0b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_59bc0911a60cee0b PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_59bc0911a60cee0b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_59bc0911a60cee0b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ec5a4af8a5245b48
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ec5a4af8a5245b48.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_ec5a4af8a5245b48 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ec5a4af8a5245b48 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ec5a4af8a5245b48 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d14e18b2dd6c3dc4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d14e18b2dd6c3dc4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_d14e18b2dd6c3dc4 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d14e18b2dd6c3dc4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d14e18b2dd6c3dc4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_18888b60e457f01a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_18888b60e457f01a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_18888b60e457f01a PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_18888b60e457f01a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_18888b60e457f01a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dd8e13d0587b86e3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dd8e13d0587b86e3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_dd8e13d0587b86e3 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dd8e13d0587b86e3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dd8e13d0587b86e3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e87882c1a92dbc50
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e87882c1a92dbc50.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_e87882c1a92dbc50 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e87882c1a92dbc50 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e87882c1a92dbc50 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8bde3ca8691dc044
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8bde3ca8691dc044.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_8bde3ca8691dc044 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8bde3ca8691dc044 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8bde3ca8691dc044 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_59d2f055e459d182
  ${CMAKE_SOURCE_DIR}/generated/fuzz_59d2f055e459d182.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_59d2f055e459d182 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_59d2f055e459d182 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_59d2f055e459d182 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d731e33874c1ddbc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d731e33874c1ddbc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_d731e33874c1ddbc PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d731e33874c1ddbc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d731e33874c1ddbc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_418f50b0e401a602
  ${CMAKE_SOURCE_DIR}/generated/fuzz_418f50b0e401a602.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_418f50b0e401a602 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_418f50b0e401a602 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_418f50b0e401a602 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_97051cf05a483ac2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_97051cf05a483ac2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_97051cf05a483ac2 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_97051cf05a483ac2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_97051cf05a483ac2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_062a4e4bc80fa5a9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_062a4e4bc80fa5a9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_062a4e4bc80fa5a9 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_062a4e4bc80fa5a9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_062a4e4bc80fa5a9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7298ddd09eb875c1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7298ddd09eb875c1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_preinstallmanager)
  target_link_libraries(fuzz_7298ddd09eb875c1 PRIVATE appmanager_fuzz_preinstallmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7298ddd09eb875c1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7298ddd09eb875c1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b74d899dfbb1f89c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b74d899dfbb1f89c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_b74d899dfbb1f89c PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b74d899dfbb1f89c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b74d899dfbb1f89c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7b2f31b16ab1c905
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7b2f31b16ab1c905.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_7b2f31b16ab1c905 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7b2f31b16ab1c905 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7b2f31b16ab1c905 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8dfc1b71022ed710
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8dfc1b71022ed710.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_8dfc1b71022ed710 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8dfc1b71022ed710 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8dfc1b71022ed710 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fdc7878f43600ae3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fdc7878f43600ae3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_fdc7878f43600ae3 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fdc7878f43600ae3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fdc7878f43600ae3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c73cbc7533fd8975
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c73cbc7533fd8975.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_c73cbc7533fd8975 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c73cbc7533fd8975 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c73cbc7533fd8975 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_590dd03e5fbcc1fc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_590dd03e5fbcc1fc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_590dd03e5fbcc1fc PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_590dd03e5fbcc1fc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_590dd03e5fbcc1fc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_80393390894b3ba8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_80393390894b3ba8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_80393390894b3ba8 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_80393390894b3ba8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_80393390894b3ba8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e919f94be3693f92
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e919f94be3693f92.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_e919f94be3693f92 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e919f94be3693f92 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e919f94be3693f92 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2528ed0c983c477d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2528ed0c983c477d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_2528ed0c983c477d PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2528ed0c983c477d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2528ed0c983c477d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f7d7ec950aa7f54d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f7d7ec950aa7f54d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_f7d7ec950aa7f54d PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f7d7ec950aa7f54d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f7d7ec950aa7f54d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_92fd8b47bff864ca
  ${CMAKE_SOURCE_DIR}/generated/fuzz_92fd8b47bff864ca.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_92fd8b47bff864ca PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_92fd8b47bff864ca PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_92fd8b47bff864ca PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_64aa1860fae2333b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_64aa1860fae2333b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_64aa1860fae2333b PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_64aa1860fae2333b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_64aa1860fae2333b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0a355e5c15c05441
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0a355e5c15c05441.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_0a355e5c15c05441 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0a355e5c15c05441 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0a355e5c15c05441 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7d883a50795999ad
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7d883a50795999ad.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_7d883a50795999ad PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7d883a50795999ad PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7d883a50795999ad PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a8e8b8490a9b4aa9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a8e8b8490a9b4aa9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_a8e8b8490a9b4aa9 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a8e8b8490a9b4aa9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a8e8b8490a9b4aa9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_41c1a921bce58c28
  ${CMAKE_SOURCE_DIR}/generated/fuzz_41c1a921bce58c28.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_41c1a921bce58c28 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_41c1a921bce58c28 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_41c1a921bce58c28 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f355ada36f4cf257
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f355ada36f4cf257.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_f355ada36f4cf257 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f355ada36f4cf257 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f355ada36f4cf257 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_dcc16062d43e7f57
  ${CMAKE_SOURCE_DIR}/generated/fuzz_dcc16062d43e7f57.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_dcc16062d43e7f57 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_dcc16062d43e7f57 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_dcc16062d43e7f57 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_95d26c86153b5671
  ${CMAKE_SOURCE_DIR}/generated/fuzz_95d26c86153b5671.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_95d26c86153b5671 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_95d26c86153b5671 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_95d26c86153b5671 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_105b8cd02b8919dc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_105b8cd02b8919dc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_105b8cd02b8919dc PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_105b8cd02b8919dc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_105b8cd02b8919dc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5741764c864d511a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5741764c864d511a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_5741764c864d511a PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5741764c864d511a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5741764c864d511a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_7f0b648dcfbbb11c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_7f0b648dcfbbb11c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_7f0b648dcfbbb11c PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_7f0b648dcfbbb11c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_7f0b648dcfbbb11c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e44730621d14e82f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e44730621d14e82f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_e44730621d14e82f PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e44730621d14e82f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e44730621d14e82f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_de2cb0f01c001cd1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_de2cb0f01c001cd1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_de2cb0f01c001cd1 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_de2cb0f01c001cd1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_de2cb0f01c001cd1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f6b74e054e3136dc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f6b74e054e3136dc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_f6b74e054e3136dc PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f6b74e054e3136dc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f6b74e054e3136dc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_3022c94486338a89
  ${CMAKE_SOURCE_DIR}/generated/fuzz_3022c94486338a89.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_3022c94486338a89 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_3022c94486338a89 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_3022c94486338a89 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5f0d4e20b2aa0562
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5f0d4e20b2aa0562.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_5f0d4e20b2aa0562 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5f0d4e20b2aa0562 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5f0d4e20b2aa0562 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_784de65a3094c31a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_784de65a3094c31a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_784de65a3094c31a PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_784de65a3094c31a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_784de65a3094c31a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1a8c07798277f899
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1a8c07798277f899.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_1a8c07798277f899 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1a8c07798277f899 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1a8c07798277f899 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1c840d751d872cff
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1c840d751d872cff.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_1c840d751d872cff PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1c840d751d872cff PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1c840d751d872cff PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_34c65996be6f0ea9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_34c65996be6f0ea9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_34c65996be6f0ea9 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_34c65996be6f0ea9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_34c65996be6f0ea9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c45738c72e2a08fc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c45738c72e2a08fc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_c45738c72e2a08fc PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c45738c72e2a08fc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c45738c72e2a08fc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a08c9e79d1bb3360
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a08c9e79d1bb3360.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_a08c9e79d1bb3360 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a08c9e79d1bb3360 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a08c9e79d1bb3360 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b3d4b2cb1575f2b3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b3d4b2cb1575f2b3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_b3d4b2cb1575f2b3 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b3d4b2cb1575f2b3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b3d4b2cb1575f2b3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5edfdad180b012e2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5edfdad180b012e2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_5edfdad180b012e2 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5edfdad180b012e2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5edfdad180b012e2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_cd6a5cd587eb678f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_cd6a5cd587eb678f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_cd6a5cd587eb678f PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_cd6a5cd587eb678f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_cd6a5cd587eb678f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_17123631d2962a22
  ${CMAKE_SOURCE_DIR}/generated/fuzz_17123631d2962a22.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_17123631d2962a22 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_17123631d2962a22 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_17123631d2962a22 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_886c9d1dcc92471b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_886c9d1dcc92471b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_886c9d1dcc92471b PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_886c9d1dcc92471b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_886c9d1dcc92471b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6e8777f1be88526d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6e8777f1be88526d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_6e8777f1be88526d PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6e8777f1be88526d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6e8777f1be88526d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_39b318ad499f6f3b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_39b318ad499f6f3b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_39b318ad499f6f3b PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_39b318ad499f6f3b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_39b318ad499f6f3b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_03a315447ed003a7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_03a315447ed003a7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_03a315447ed003a7 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_03a315447ed003a7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_03a315447ed003a7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_16ec484e696f5414
  ${CMAKE_SOURCE_DIR}/generated/fuzz_16ec484e696f5414.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_16ec484e696f5414 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_16ec484e696f5414 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_16ec484e696f5414 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f15ea56b0cec423e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f15ea56b0cec423e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_f15ea56b0cec423e PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f15ea56b0cec423e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f15ea56b0cec423e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d0d5116d0fa766f9
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d0d5116d0fa766f9.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_d0d5116d0fa766f9 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d0d5116d0fa766f9 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d0d5116d0fa766f9 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_16edd13b4d2a9e77
  ${CMAKE_SOURCE_DIR}/generated/fuzz_16edd13b4d2a9e77.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_16edd13b4d2a9e77 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_16edd13b4d2a9e77 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_16edd13b4d2a9e77 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2258c24faf7093e7
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2258c24faf7093e7.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_2258c24faf7093e7 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2258c24faf7093e7 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2258c24faf7093e7 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5a15b5f606637451
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5a15b5f606637451.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_5a15b5f606637451 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5a15b5f606637451 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5a15b5f606637451 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_def12768432ce98d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_def12768432ce98d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_def12768432ce98d PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_def12768432ce98d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_def12768432ce98d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_178c9525e4e4be85
  ${CMAKE_SOURCE_DIR}/generated/fuzz_178c9525e4e4be85.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_178c9525e4e4be85 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_178c9525e4e4be85 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_178c9525e4e4be85 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a7c3b288ee12a3ba
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a7c3b288ee12a3ba.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_a7c3b288ee12a3ba PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a7c3b288ee12a3ba PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a7c3b288ee12a3ba PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_73aaf387cfd8d732
  ${CMAKE_SOURCE_DIR}/generated/fuzz_73aaf387cfd8d732.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_73aaf387cfd8d732 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_73aaf387cfd8d732 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_73aaf387cfd8d732 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8387b8e1e29af842
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8387b8e1e29af842.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_8387b8e1e29af842 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8387b8e1e29af842 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8387b8e1e29af842 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0c445135db1c976c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0c445135db1c976c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_0c445135db1c976c PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0c445135db1c976c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0c445135db1c976c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ec8e94ff41c223b8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ec8e94ff41c223b8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_ec8e94ff41c223b8 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ec8e94ff41c223b8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ec8e94ff41c223b8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_48c57287f273ed5b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_48c57287f273ed5b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_48c57287f273ed5b PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_48c57287f273ed5b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_48c57287f273ed5b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_c82abffd39eab85c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_c82abffd39eab85c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_c82abffd39eab85c PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_c82abffd39eab85c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_c82abffd39eab85c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d14d3132e4d0df97
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d14d3132e4d0df97.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_d14d3132e4d0df97 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d14d3132e4d0df97 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d14d3132e4d0df97 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d9c73a71767541a2
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d9c73a71767541a2.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_d9c73a71767541a2 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d9c73a71767541a2 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d9c73a71767541a2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_56c572a915d07a3d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_56c572a915d07a3d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_56c572a915d07a3d PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_56c572a915d07a3d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_56c572a915d07a3d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8ff1c8a9d613bad5
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8ff1c8a9d613bad5.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_8ff1c8a9d613bad5 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8ff1c8a9d613bad5 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8ff1c8a9d613bad5 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5ae4e969ef204f0d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5ae4e969ef204f0d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_5ae4e969ef204f0d PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5ae4e969ef204f0d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5ae4e969ef204f0d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_987e740bc9a5ee39
  ${CMAKE_SOURCE_DIR}/generated/fuzz_987e740bc9a5ee39.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_987e740bc9a5ee39 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_987e740bc9a5ee39 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_987e740bc9a5ee39 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a5ed3fe0462dae82
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a5ed3fe0462dae82.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_a5ed3fe0462dae82 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a5ed3fe0462dae82 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a5ed3fe0462dae82 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_09c449acd8ec7431
  ${CMAKE_SOURCE_DIR}/generated/fuzz_09c449acd8ec7431.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_09c449acd8ec7431 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_09c449acd8ec7431 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_09c449acd8ec7431 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d908ca21de8b0052
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d908ca21de8b0052.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_d908ca21de8b0052 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d908ca21de8b0052 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d908ca21de8b0052 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_25b8028ac0e83532
  ${CMAKE_SOURCE_DIR}/generated/fuzz_25b8028ac0e83532.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_25b8028ac0e83532 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_25b8028ac0e83532 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_25b8028ac0e83532 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5efe4314466f477f
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5efe4314466f477f.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_5efe4314466f477f PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5efe4314466f477f PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5efe4314466f477f PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_6f0729fc7066a8fd
  ${CMAKE_SOURCE_DIR}/generated/fuzz_6f0729fc7066a8fd.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_6f0729fc7066a8fd PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_6f0729fc7066a8fd PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_6f0729fc7066a8fd PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_d7c26c9deeb6a39c
  ${CMAKE_SOURCE_DIR}/generated/fuzz_d7c26c9deeb6a39c.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_d7c26c9deeb6a39c PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_d7c26c9deeb6a39c PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_d7c26c9deeb6a39c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_f10c696137b3a056
  ${CMAKE_SOURCE_DIR}/generated/fuzz_f10c696137b3a056.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_f10c696137b3a056 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_f10c696137b3a056 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_f10c696137b3a056 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_080af7b39a48d6b1
  ${CMAKE_SOURCE_DIR}/generated/fuzz_080af7b39a48d6b1.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_080af7b39a48d6b1 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_080af7b39a48d6b1 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_080af7b39a48d6b1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_a260c9d0cff3a9bb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_a260c9d0cff3a9bb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_a260c9d0cff3a9bb PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_a260c9d0cff3a9bb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_a260c9d0cff3a9bb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bc57500f9ca72b52
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bc57500f9ca72b52.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_bc57500f9ca72b52 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bc57500f9ca72b52 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bc57500f9ca72b52 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e6b35e79f2359079
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e6b35e79f2359079.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_e6b35e79f2359079 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e6b35e79f2359079 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e6b35e79f2359079 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_2370f94a103ab103
  ${CMAKE_SOURCE_DIR}/generated/fuzz_2370f94a103ab103.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_2370f94a103ab103 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_2370f94a103ab103 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_2370f94a103ab103 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b39b753bfb3f2c9d
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b39b753bfb3f2c9d.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_b39b753bfb3f2c9d PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b39b753bfb3f2c9d PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b39b753bfb3f2c9d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_fdd768132c38b1e8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_fdd768132c38b1e8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_fdd768132c38b1e8 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_fdd768132c38b1e8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_fdd768132c38b1e8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_bdaa31755bd92217
  ${CMAKE_SOURCE_DIR}/generated/fuzz_bdaa31755bd92217.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_bdaa31755bd92217 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_bdaa31755bd92217 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_bdaa31755bd92217 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b9eb3bf331b49bb4
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b9eb3bf331b49bb4.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_b9eb3bf331b49bb4 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b9eb3bf331b49bb4 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b9eb3bf331b49bb4 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8138c314619d6079
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8138c314619d6079.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_8138c314619d6079 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8138c314619d6079 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8138c314619d6079 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e5b9c1234da84ccc
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e5b9c1234da84ccc.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_e5b9c1234da84ccc PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e5b9c1234da84ccc PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e5b9c1234da84ccc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_5431a7746481332a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_5431a7746481332a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_5431a7746481332a PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_5431a7746481332a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_5431a7746481332a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e04ba18c7f2c0a62
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e04ba18c7f2c0a62.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_e04ba18c7f2c0a62 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e04ba18c7f2c0a62 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e04ba18c7f2c0a62 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_b6837b1b71611e18
  ${CMAKE_SOURCE_DIR}/generated/fuzz_b6837b1b71611e18.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_b6837b1b71611e18 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_b6837b1b71611e18 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_b6837b1b71611e18 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_31205c08744dd35a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_31205c08744dd35a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_31205c08744dd35a PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_31205c08744dd35a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_31205c08744dd35a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_ba79ade564b78fb0
  ${CMAKE_SOURCE_DIR}/generated/fuzz_ba79ade564b78fb0.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_ba79ade564b78fb0 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_ba79ade564b78fb0 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_ba79ade564b78fb0 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_0c66c012952d67b3
  ${CMAKE_SOURCE_DIR}/generated/fuzz_0c66c012952d67b3.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_rdkwindowmanager)
  target_link_libraries(fuzz_0c66c012952d67b3 PRIVATE appmanager_fuzz_rdkwindowmanager)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_0c66c012952d67b3 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_0c66c012952d67b3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8edb6a71001cf85b
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8edb6a71001cf85b.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_8edb6a71001cf85b PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8edb6a71001cf85b PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8edb6a71001cf85b PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_aca1780608ba3b6e
  ${CMAKE_SOURCE_DIR}/generated/fuzz_aca1780608ba3b6e.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_aca1780608ba3b6e PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_aca1780608ba3b6e PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_aca1780608ba3b6e PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_34e0f9d65d701960
  ${CMAKE_SOURCE_DIR}/generated/fuzz_34e0f9d65d701960.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_34e0f9d65d701960 PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_34e0f9d65d701960 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_34e0f9d65d701960 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_11edb81d4ba7dba8
  ${CMAKE_SOURCE_DIR}/generated/fuzz_11edb81d4ba7dba8.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_11edb81d4ba7dba8 PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_11edb81d4ba7dba8 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_11edb81d4ba7dba8 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_1516cede9d9dfa22
  ${CMAKE_SOURCE_DIR}/generated/fuzz_1516cede9d9dfa22.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_1516cede9d9dfa22 PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_1516cede9d9dfa22 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_1516cede9d9dfa22 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_18b55bc64bd32efb
  ${CMAKE_SOURCE_DIR}/generated/fuzz_18b55bc64bd32efb.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_18b55bc64bd32efb PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_18b55bc64bd32efb PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_18b55bc64bd32efb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_8d48214ee386fb76
  ${CMAKE_SOURCE_DIR}/generated/fuzz_8d48214ee386fb76.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_8d48214ee386fb76 PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_8d48214ee386fb76 PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_8d48214ee386fb76 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_e3751c1dff40c48a
  ${CMAKE_SOURCE_DIR}/generated/fuzz_e3751c1dff40c48a.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_e3751c1dff40c48a PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_e3751c1dff40c48a PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_e3751c1dff40c48a PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(fuzz_9ee63f896d1100ac
  ${CMAKE_SOURCE_DIR}/generated/fuzz_9ee63f896d1100ac.cpp
  $<TARGET_OBJECTS:appmanager_fuzz_stub_support>
)
if(FUZZ_INCLUDE_APP_SOURCES AND TARGET appmanager_fuzz_telemetrymetrics)
  target_link_libraries(fuzz_9ee63f896d1100ac PRIVATE appmanager_fuzz_telemetrymetrics)
endif()
if(FUZZ_OPTIONAL_LIBS)
  target_link_libraries(fuzz_9ee63f896d1100ac PRIVATE ${FUZZ_OPTIONAL_LIBS})
endif()
set_target_properties(fuzz_9ee63f896d1100ac PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

