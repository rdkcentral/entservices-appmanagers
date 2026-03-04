# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 RDK Management
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


message("Building for L2 tests...")

message("Generating empty headers to suppress compiler errors")

file(GLOB BASEDIR Tests)
set(BASEDIR ${BASEDIR}/headers)
set(EMPTY_HEADERS_DIRS
        ${BASEDIR}
        ${BASEDIR}/rdk/iarmbus
        ${BASEDIR}/rdk/iarmmgrs-hal
        ${BASEDIR}/libusb
        )

set(EMPTY_HEADERS
         ${BASEDIR}/rdk/iarmbus/libIARM.h
         ${BASEDIR}/rdk/iarmbus/libIBus.h
         ${BASEDIR}/rdk/iarmbus/libIBusDaemon.h
         ${BASEDIR}/rdk/iarmmgrs-hal/mfrMgr.h
         ${BASEDIR}/rdk/iarmmgrs-hal/sysMgr.h
         ${BASEDIR}/rfcapi.h
         ${BASEDIR}/rbus.h
         ${BASEDIR}/libudev.h
         ${BASEDIR}/libusb/libusb.h
         ${BASEDIR}/secure_wrapper.h
         ${BASEDIR}/wpa_ctrl.h
         ${BASEDIR}/maintenanceMGR.h
         ${BASEDIR}/pkg.h
         ${BASEDIR}/edid-parser.hpp
         ${BASEDIR}/Wraps.h
         ${BASEDIR}/telemetry_busmessage_sender.h
         ${BASEDIR}/gdialservice.h
         ${BASEDIR}/gdialservicecommon.h
         )


file(MAKE_DIRECTORY ${EMPTY_HEADERS_DIRS})

file(GLOB_RECURSE EMPTY_HEADERS_AVAILABLE "${BASEDIR}/*")
if (EMPTY_HEADERS_AVAILABLE)
    message("Skip already generated headers to avoid rebuild")
    list(REMOVE_ITEM EMPTY_HEADERS ${EMPTY_HEADERS_AVAILABLE})
endif ()
if (EMPTY_HEADERS)
    file(TOUCH ${EMPTY_HEADERS})
endif ()

include_directories(${EMPTY_HEADERS_DIRS})

message("Adding compiler and linker options for all targets")

file(GLOB BASEDIR Tests/mocks)
set(FAKE_HEADERS
        ${BASEDIR}/Iarm.h
        ${BASEDIR}/Rfc.h
        ${BASEDIR}/RBus.h
        ${BASEDIR}/Telemetry.h
        ${BASEDIR}/Udev.h
        ${BASEDIR}/libusb/libusb.h
        ${BASEDIR}/secure_wrappermock.h
        ${BASEDIR}/wpa_ctrl_mock.h
        ${BASEDIR}/readprocMockInterface.h
        ${BASEDIR}/maintenanceMGR.h
        ${BASEDIR}/pkg.h
        ${BASEDIR}/gdialservice.h
        ${BASEDIR}/Wraps.h
        )


foreach (file ${FAKE_HEADERS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include ${file}")
endforeach ()

add_compile_options(-Wall -Werror)
add_link_options(-Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog -Wl,-wrap,v_secure_popen -Wl,-wrap,v_secure_pclose -Wl,-wrap,v_secure_system -Wl,-wrap,pclose)

add_definitions(
         -DUSE_IARMBUS
         -DRFC_ENABLED
         -DHAS_API_SYSTEM
         -DHAS_RBUS
         -DDISABLE_SECURITY_TOKEN
         -DENABLE_TELEMETRY_LOGGING
         -DENABLE_DEVICE_MANUFACTURER_INFO
         -DUNIT_TESTING
)

message("Setting build options")
set(CMAKE_DISABLE_FIND_PACKAGE_IARMBus ON)
set(CMAKE_DISABLE_FIND_PACKAGE_Udev ON)
set(CMAKE_DISABLE_FIND_PACKAGE_RFC ON)
set(CMAKE_DISABLE_FIND_PACKAGE_RBus ON)
set(BUILD_SHARED_LIBS ON)
set(LIBOPKG_LIBRARIES ${LIBOPKG_LIBRARIES} CACHE PATH "Path to LIBOPKG library")
set(LIBOPKG_INCLUDE_DIRS ${LIBOPKG_INCLUDE_DIRS} CACHE PATH "Path to LIBOPKG include")
