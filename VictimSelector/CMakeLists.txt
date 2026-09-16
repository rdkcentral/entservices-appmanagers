set(PLUGIN_NAME VictimSelector)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

set(PLUGIN_VICTIM_SELECTOR_MODE "Off" CACHE STRING "Controls if the VictimSelector plugin should run in its own process, in process or remote")
set(PLUGIN_VICTIM_SELECTOR_AUTOSTART false CACHE STRING "Automatically start VictimSelector plugin")
set(PLUGIN_VICTIM_SELECTOR_STARTUPORDER "" CACHE STRING "Automatically start VictimSelector plugin")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${PLUGIN_IMPLEMENTATION} SHARED
    VictimSelectorImplementation.cpp
    Module.cpp
)

target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../helpers)
set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES)
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

add_library(${MODULE_NAME} SHARED
    VictimSelector.cpp
    Module.cpp
)

set_target_properties(${MODULE_NAME} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES)
target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../helpers)
target_link_libraries(${MODULE_NAME} PRIVATE
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    ${NAMESPACE}Definitions::${NAMESPACE}Definitions
    ${PLUGIN_IMPLEMENTATION})

install(TARGETS ${MODULE_NAME} ${PLUGIN_IMPLEMENTATION}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
