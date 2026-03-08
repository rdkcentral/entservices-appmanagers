cmake_minimum_required(VERSION 3.8)
project(RdkServicesL1Test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define namespace (replace WPEFramework with your actual namespace if needed)
set(NAMESPACE WPEFramework)

# Find required packages
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(GTest REQUIRED)

# -----------------------------
# Plugin Test Library (L1TestsIN)
# -----------------------------
set(PLUGIN_NAME L1TestsIN)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

# Base plugin test sources, libs, and includes
set(TEST_SRC
    tests/test_UtilsFile.cpp
)

set(TEST_LIB
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

set(TEST_INC ../../helpers)

# Macro to append plugin test sources, includes, libs
macro(add_plugin_test_ex plugin_opt plugin_test_sources_str plugin_includes_str plugin_libs_str)
    if(${plugin_opt})
        message(STATUS "${plugin_opt}=ON")
        string(REPLACE ";" ";" srclist "${plugin_test_sources_str}")
        string(REPLACE ";" ";" inclist "${plugin_includes_str}")
        string(REPLACE ";" ";" liblist "${plugin_libs_str}")

        foreach(item IN LISTS srclist)
            list(APPEND TEST_SRC ${item})
        endforeach()
        foreach(item IN LISTS inclist)
            list(APPEND TEST_INC ${item})
        endforeach()
        foreach(item IN LISTS liblist)
            list(APPEND TEST_LIB ${item})
        endforeach()
    else()
        message(STATUS "${plugin_opt}=OFF")
    endif()
endmacro()

# Helper to add plugin test
macro(add_plugin_test plugin_name test_files)
    string(TOUPPER "${plugin_name}" plugin_option)
    set(plugin_opt "PLUGIN_${plugin_option}")
    add_plugin_test_ex(${plugin_opt} "${test_files}" "../../${plugin_name}" "${NAMESPACE}${plugin_name}")
endmacro()

# Example: Add AppManager plugin test
set(APPMANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/AppManager ${CMAKE_SOURCE_DIR}/../entservices-appmanagers/helpers)
set(APPMANAGER_LIBS ${NAMESPACE}AppManager ${NAMESPACE}AppManagerImplementation)
add_plugin_test_ex(PLUGIN_APPMANAGER tests/test_AppManager.cpp "${APPMANAGER_INC}" "${APPMANAGER_LIBS}")

# Add other plugins similarly...
# e.g., RuntimeManager, AppStorageManager, LifecycleManager, etc.

# Build shared library for L1TestsIN
add_library(${MODULE_NAME} SHARED ${TEST_SRC})
target_include_directories(${MODULE_NAME} PUBLIC ${TEST_INC} ${CMAKE_CURRENT_SOURCE_DIR}/../mocks)
target_link_libraries(${MODULE_NAME} ${TEST_LIB})

# Optional linker wrap flag
if(RDK_SERVICES_L1_TEST)
    target_link_options(${MODULE_NAME} PRIVATE "-Wl,-wrap,opendir")
endif()

# Install library
install(TARGETS ${MODULE_NAME} DESTINATION lib)

# -----------------------------
# RdkServicesL1Test Executable
# -----------------------------
add_executable(RdkServicesL1Test
    test_JSON.cpp  # Add any other test sources
)

# Include paths for executable
target_include_directories(RdkServicesL1Test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../../helpers
    ${CMAKE_INSTALL_PREFIX}/include
)

# Link plugin library and required test libs
target_link_libraries(RdkServicesL1Test
    ${MODULE_NAME}   # L1TestsIN shared library
    ${CMAKE_INSTALL_PREFIX}/lib/libTestMocklib.so
    gtest_main
    gmock
    gtest
)

# Install executable
install(TARGETS RdkServicesL1Test DESTINATION bin)