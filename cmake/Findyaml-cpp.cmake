# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
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

# - Try to find the yaml-cpp library.
#
# Debian/Ubuntu's libyaml-cpp-dev package does not install a yaml-cpp CMake
# package configuration file (yaml-cppConfig.cmake / yaml-cpp-config.cmake),
# so a plain `find_package(yaml-cpp CONFIG REQUIRED)` fails on those
# distributions even though the library and headers are present. This module
# locates yaml-cpp the same way the other third-party Find modules in this
# directory do (find_path/find_library) so it works regardless of whether
# the installed yaml-cpp ships CMake config files.
#
# The following are set after configuration is done:
#  YAML_CPP_FOUND
#  YAML_CPP_INCLUDE_DIRS
#  YAML_CPP_LIBRARIES
#
# An imported target named `yaml-cpp` is created for consumers that already
# do `target_link_libraries(<target> PRIVATE yaml-cpp)`.

# Use pkg-config (reliably installed by the apt package via its yaml-cpp.pc
# file) as a hint source. This covers runners/distros where the library
# isn't under one of CMake's default find_library() search paths (e.g. some
# multiarch layouts), without requiring pkg-config to be the only lookup.
find_package( PkgConfig QUIET )
if( PkgConfig_FOUND )
    pkg_check_modules( PC_YAML_CPP QUIET yaml-cpp )
endif()

find_path( YAML_CPP_INCLUDE_DIR NAMES yaml-cpp/yaml.h
        HINTS ${PC_YAML_CPP_INCLUDEDIR} ${PC_YAML_CPP_INCLUDE_DIRS} )
find_library( YAML_CPP_LIBRARY NAMES yaml-cpp libyaml-cpp.so
        HINTS ${PC_YAML_CPP_LIBDIR} ${PC_YAML_CPP_LIBRARY_DIRS} )

include( FindPackageHandleStandardArgs )

# Handle the QUIETLY and REQUIRED arguments and set yaml-cpp_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( yaml-cpp DEFAULT_MSG
        YAML_CPP_LIBRARY YAML_CPP_INCLUDE_DIR )

mark_as_advanced( YAML_CPP_INCLUDE_DIR YAML_CPP_LIBRARY )

if( yaml-cpp_FOUND )
    set( YAML_CPP_LIBRARIES ${YAML_CPP_LIBRARY} )
    set( YAML_CPP_INCLUDE_DIRS ${YAML_CPP_INCLUDE_DIR} )
endif()

if( yaml-cpp_FOUND AND NOT TARGET yaml-cpp )
    add_library( yaml-cpp SHARED IMPORTED )
    set_target_properties( yaml-cpp PROPERTIES
            IMPORTED_LOCATION "${YAML_CPP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${YAML_CPP_INCLUDE_DIR}" )
endif()
