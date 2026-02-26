# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2025 RDK Management
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

cmake_minimum_required(VERSION 3.8)

if(L2_TEST_OOP_RPC)
        add_library(MockAccessor SHARED
            MockAccessor.cpp
        )

    # Include directories for the new MockAccessor
        target_include_directories(MockAccessor PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_INSTALL_PREFIX}/include/WPEFramework/core
            ${CMAKE_SOURCE_DIR}/helpers
            ${CMAKE_SOURCE_DIR}/Tests/mocks
            ${CMAKE_SOURCE_DIR}/Tests/mocks/thunder
            ${CMAKE_INSTALL_PREFIX}/include
            ${CMAKE_INSTALL_PREFIX}/include/WPEFramework
        )

    # Set target properties for MockAccessor
        set_target_properties(MockAccessor PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED YES)
    
    # Install MockAccessor
        install(TARGETS MockAccessor
            DESTINATION lib)
    
        add_subdirectory(MockProxy)

else(L2_TEST_OOP_RPC)

    set(MODULE_NAME TestMocklib)

    find_package(Protobuf REQUIRED)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

        set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/secure_storage)
        set(PROTO_FILE ${PROTO_DIR}/secure_storage.proto)
        set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})

        set(GENERATED_PROTO_SRC ${GENERATED_DIR}/secure_storage.pb.cc)
        set(GENERATED_PROTO_HDR ${GENERATED_DIR}/secure_storage.pb.h)
        set(GENERATED_GRPC_SRC ${GENERATED_DIR}/secure_storage.grpc.pb.cc)
        set(GENERATED_GRPC_HDR ${GENERATED_DIR}/secure_storage.grpc.pb.h)

        add_custom_command(
            OUTPUT ${GENERATED_PROTO_SRC} ${GENERATED_PROTO_HDR}
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                --proto_path=${PROTO_DIR}
                --cpp_out=${GENERATED_DIR}
                ${PROTO_FILE}
            DEPENDS ${PROTO_FILE}
            COMMENT "Generating Protobuf source files..."
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${GENERATED_GRPC_SRC} ${GENERATED_GRPC_HDR}
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                --proto_path=${PROTO_DIR}
                --grpc_out=${GENERATED_DIR}
                --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
                ${PROTO_FILE}
            DEPENDS ${PROTO_FILE}
            COMMENT "Generating gRPC source files..."
            VERBATIM
        )

    add_custom_target(generate_secure_storage_proto
        DEPENDS ${GENERATED_PROTO_SRC} ${GENERATED_GRPC_SRC}
    )

    set(MODULE_SOURCES 
        Rfc.cpp
        Iarm.cpp
        Wraps.cpp
        RBus.cpp
        Telemetry.cpp
        readprocMockInterface.cpp
        Udev.cpp
        libusb.cpp
        DRMScreenCapture.cpp
        systemaudioplatform.cpp
        dsFPD.cpp
        MfrMock.cpp
        PowerManagerHalMock.cpp
        RenderSession.cpp
        gdialservice.cpp
        )

    if(RDK_SERVICES_L1_TEST)
        list(APPEND MODULE_SOURCES
            MotionDetection.cpp
            devicesettings.cpp
            HdmiCec.cpp
            thunder/Module.cpp
            Dobby.cpp
            rdkshell.cpp
            opkgMock.cpp
            PowerManagerMock.cpp
			tr181api.cpp
        )

    else(RDK_SERVICES_L1_TEST)
        list(APPEND MODULE_SOURCES
	    devicesettings.cpp
            systemaudioplatform.cpp
            btmgr.cpp
	    tvSettings.cpp
            tr181api.cpp
            EssRMgr.cpp
            ${GENERATED_PROTO_SRC}
            ${GENERATED_GRPC_SRC}
            SecureStorageServerMock.cpp
            Dobby.cpp
            Omi.cpp
	    HdmiCec.cpp
	    )
    if(RDK_SERVICE_CPC_L2_TEST)
        list(APPEND MODULE_SOURCES
            sec_security.cpp
            KeyProvisionObject.cpp
            KeyProvisionResult.cpp
            KeyProvisionDirectorySerializer.cpp
            CredentialUtils.cpp
            KeyProvisionClient.cpp
            SecApiProvisioner.cpp
            )
    endif()

    endif()

    add_library(${MODULE_NAME} SHARED ${MODULE_SOURCES})

    add_dependencies(${MODULE_NAME} generate_secure_storage_proto)

    set_source_files_properties(
        ${GENERATED_PROTO_SRC} ${GENERATED_GRPC_SRC}
        PROPERTIES GENERATED TRUE
    )

    target_include_directories(${MODULE_NAME} PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
    )

    target_link_libraries(${MODULE_NAME} PRIVATE
        grpc++
        protobuf
    )

    target_include_directories(${MODULE_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/Tests/mocks
            ${CMAKE_SOURCE_DIR}/Tests/mocks/thunder
            ${CMAKE_SOURCE_DIR}/Tests/mocks/devicesettings
            ${CMAKE_SOURCE_DIR}/Tests/mocks/libusb
            ${CMAKE_INSTALL_PREFIX}/include
            ${CMAKE_INSTALL_PREFIX}/include/WPEFramework
            ${CMAKE_CURRENT_SOURCE_DIR}/../../OCIContainer/stubs
             ../../../Thunder/Source/plugins
        )

    # Set target properties for TestMocklib
    set_target_properties(${MODULE_NAME} PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED YES)

    # Install TestMocklib
    install(TARGETS ${MODULE_NAME}
        DESTINATION lib)

endif(L2_TEST_OOP_RPC)
