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
        ${BASEDIR}/rdk/ds
        ${BASEDIR}/rdk/iarmbus
        ${BASEDIR}/rdk/iarmmgrs-hal
        ${BASEDIR}/systemservices
        ${BASEDIR}/systemservices/proc
        )

set(EMPTY_HEADERS
         ${BASEDIR}/rdk/ds/host.hpp
         ${BASEDIR}/rdk/ds/videoOutputPort.hpp
         ${BASEDIR}/rdk/ds/audioOutputPort.hpp
         ${BASEDIR}/rdk/ds/dsMgr.h
         ${BASEDIR}/rdk/ds/manager.hpp
         ${BASEDIR}/rdk/ds/audioOutputPortType.hpp
         ${BASEDIR}/rdk/ds/audioOutputPortConfig.hpp
         ${BASEDIR}/rdk/ds/pixelResolution.hpp
         ${BASEDIR}/rdk/iarmbus/libIARM.h
         ${BASEDIR}/rdk/iarmbus/libIBus.h
         ${BASEDIR}/rdk/iarmbus/libIBusDaemon.h
         ${BASEDIR}/rdk/iarmmgrs-hal/pwrMgr.h
         ${BASEDIR}/rdk/iarmmgrs-hal/mfrMgr.h
         ${BASEDIR}/rdk/iarmmgrs-hal/sysMgr.h
         ${BASEDIR}/rdk_logger_milestone.h
         ${BASEDIR}/rdk/ds/videoOutputPortConfig.hpp
         ${BASEDIR}/rdk/ds/sleepMode.hpp
         ${BASEDIR}/rdk/ds/frontPanelConfig.hpp
         ${BASEDIR}/rdk/ds/frontPanelTextDisplay.hpp
         ${BASEDIR}/rfcapi.h
         ${BASEDIR}/rbus.h
         ${BASEDIR}/systemservices/proc/readproc.h
         ${BASEDIR}/systemservices/secure_wrapper.h
         ${BASEDIR}/libudev.h
         ${BASEDIR}/systemaudioplatform.h
         ${BASEDIR}/rdk/ds/dsTypes.h
         ${BASEDIR}/rdk/ds/hdmiIn.hpp
         ${BASEDIR}/rdk/ds/compositeIn.hpp
         ${BASEDIR}/rdk/ds/exception.hpp
         ${BASEDIR}/rdk/iarmbus/iarmUtil.h
         ${BASEDIR}/rdk/ds/dsError.h
         ${BASEDIR}/tvTypes.h
         ${BASEDIR}/tvSettingsExtODM.h 
         ${BASEDIR}/tvError.h
         ${BASEDIR}/tvSettingsODM.h
         ${BASEDIR}/tr181api.h
         ${BASEDIR}/dsRpc.h
         ${BASEDIR}/edid-parser.hpp
         ${BASEDIR}/videoOutputPortType.hpp
         ${BASEDIR}/videoResolution.hpp
         ${BASEDIR}/audioOutputPortType.hpp
         ${BASEDIR}/audioOutputPortConfig.hpp
         ${BASEDIR}/dsUtl.h
         ${BASEDIR}/dsError.h
         ${BASEDIR}/list.hpp
         ${BASEDIR}/dsDisplay.h
         ${BASEDIR}/Wraps.h
         ${BASEDIR}/rdk/ds/AudioStereoMode.hpp
         ${BASEDIR}/rdk/ds/VideoDFC.hpp
         )
     if(RDK_SERVICE_CPC_L2_TEST)
         set(EMPTY_HEADERS
             ${BASEDIR}/sec_security_comcastids.h
             ${BASEDIR}/KeyProvisionClient.h
             ${BASEDIR}/KeyProvisionResult.h
             ${BASEDIR}/KeyProvisionObject.h
             ${BASEDIR}/KeyProvisionDirectorySerializer.h
             ${BASEDIR}/SecApiProvisioner.h
             ${BASEDIR}/CredentialUtils.h
             ${BASEDIR}/fkpsb.h
             ${BASEDIR}/sec_security.h
             )
     endif()


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
        ${BASEDIR}/devicesettings.h
        ${BASEDIR}/Iarm.h
        ${BASEDIR}/Rfc.h
        ${BASEDIR}/RBus.h
        ${BASEDIR}/Udev.h
        ${BASEDIR}/libusb/libusb.h
        ${BASEDIR}/secure_wrappermock.h
        ${BASEDIR}/readprocMockInterface.h
        ${BASEDIR}/btmgr.h
        ${BASEDIR}/tvSettings.h
        ${BASEDIR}/tr181api.h
        ${BASEDIR}/Wraps.h
	${BASEDIR}/essos-resmgr.h
	${BASEDIR}/rdk_logger_milestone.h
	${BASEDIR}/dsFPD.h
        )
    if(RDK_SERVICE_CPC_L2_TEST)
        set(FAKE_HEADERS
            ${BASEDIR}/sec_security_comcastids.h
            ${BASEDIR}/KeyProvisionClient.h
            ${BASEDIR}/KeyProvisionResult.h
            ${BASEDIR}/KeyProvisionObject.h
            ${BASEDIR}/KeyProvisionDirectorySerializer.h
            ${BASEDIR}/SecApiProvisioner.h
            ${BASEDIR}/CredentialUtils.h
            ${BASEDIR}/fkpsb.h
            ${BASEDIR}/sec_security.h
            )
    endif()


foreach (file ${FAKE_HEADERS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include ${file}")
endforeach ()

add_compile_options(-Wall -Werror)
add_link_options(-Wl,-wrap,setmntent -Wl,-wrap,getmntent -Wl,-wrap,v_secure_popen -Wl,-wrap,v_secure_pclose -Wl,-wrap,v_secure_system -Wl,-wrap,readlink)

add_definitions(
         -DUSE_IARMBUS
         -DRFC_ENABLED
         -DUSE_DS
         -DUSE_DRM_SCREENCAPTURE
         -DENABLE_THERMAL_PROTECTION
         -DHAS_API_SYSTEM
         -DHAS_API_POWERSTATE
         -DHAS_RBUS
	 -DENABLE_ERM
         -DDISABLE_SECURITY_TOKEN
         -DBUILD_ENABLE_NOT_ACTIVATED
         -DAUTHTOKEN_FETCH_BINARY="/opt/www/authService/authservice"         
         -DUNIT_TESTING
)

message("Setting build options")
set(CMAKE_DISABLE_FIND_PACKAGE_DS ON)
set(CMAKE_DISABLE_FIND_PACKAGE_IARMBus ON)
set(CMAKE_DISABLE_FIND_PACKAGE_Udev ON)
set(CMAKE_DISABLE_FIND_PACKAGE_RFC ON)
set(CMAKE_DISABLE_FIND_PACKAGE_RBus ON)
set(CMAKE_DISABLE_FIND_PACKAGE_CEC ON)
set(CMAKE_DISABLE_FIND_PACKAGE_Dobby ON)
set(CMAKE_DISABLE_FIND_PACKAGE_CEC ON)
set(BUILD_SHARED_LIBS ON)
set(DS_FOUND ON)
