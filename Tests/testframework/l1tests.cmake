###
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
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
###

message("Building for unit tests...")

message("Generating empty headers to suppress compiler errors")

set(BASEDIR ${CMAKE_CURRENT_SOURCE_DIR}/headers)
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
        ${BASEDIR}/rdk/iarmmgrs-hal/pwrMgr.h
        ${BASEDIR}/rdk/iarmmgrs-hal/sysMgr.h
        ${BASEDIR}/libudev.h
        ${BASEDIR}/libusb/libusb.h
        ${BASEDIR}/rfcapi.h
        ${BASEDIR}/rbus.h
        ${BASEDIR}/telemetry_busmessage_sender.h
        ${BASEDIR}/maintenanceMGR.h
        ${BASEDIR}/pkg.h
        ${BASEDIR}/secure_wrapper.h
        ${BASEDIR}/wpa_ctrl.h
        ${BASEDIR}/edid-parser.hpp
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

set(BASEDIR ${CMAKE_CURRENT_SOURCE_DIR}/mocks)
set(FAKE_HEADERS
        ${BASEDIR}/devicesettings.h
        ${BASEDIR}/Iarm.h
        ${BASEDIR}/Rfc.h
        ${BASEDIR}/RBus.h
        ${BASEDIR}/Telemetry.h
        ${BASEDIR}/Udev.h
        ${BASEDIR}/MotionDetection.h
        ${BASEDIR}/Dobby.h
        ${BASEDIR}/secure_wrappermock.h
        ${BASEDIR}/readprocMockInterface.h
        ${BASEDIR}/Wraps.h
        )

foreach (file ${FAKE_HEADERS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include ${file}")
endforeach ()

add_compile_options(-Wall -Werror)

add_link_options(-Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog -Wl,-wrap,v_secure_popen -Wl,-wrap,v_secure_pclose -Wl,-wrap,unlink -Wl,-wrap,v_secure_system -Wl,-wrap,pclose)

add_definitions(
        -DENABLE_TELEMETRY_LOGGING
        -DUSE_IARMBUS
        -DHAS_API_SYSTEM
        -DHAS_RBUS
        -DDISABLE_SECURITY_TOKEN
        -DENABLE_DEVICE_MANUFACTURER_INFO
        -DRFC_ENABLED
        -DUNIT_TESTING
)

message("Setting build options")

set(CMAKE_DISABLE_FIND_PACKAGE_IARMBus ON)
set(CMAKE_DISABLE_FIND_PACKAGE_Udev ON)
set(CMAKE_DISABLE_FIND_PACKAGE_RFC ON)
set(CMAKE_DISABLE_FIND_PACKAGE_RBus ON)
set(LIBOPKG_LIBRARIES ${LIBOPKG_LIBRARIES} CACHE PATH "Path to LIBOPKG library")
set(LIBOPKG_INCLUDE_DIRS ${LIBOPKG_INCLUDE_DIRS} CACHE PATH "Path to LIBOPKG include")

