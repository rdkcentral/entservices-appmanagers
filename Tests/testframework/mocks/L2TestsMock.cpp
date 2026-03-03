/* If not stated otherwise in this file or this component's LICENSE file the
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
*/

#include <string>
#include <sstream>

#include "L2TestsMock.h"
#ifdef L2_TEST_OOP_RPC
#include "MockAccessor.h"
#endif /* L2_TEST_OOP_RPC */

#define TEST_CALLSIGN _T("org.rdk.L2Tests.1")  /* Test module callsign. */
#define INVOKE_TIMEOUT 3000                        /* Method invoke timeout in milliseconds. */
#define THUNDER_ADDRESS _T("127.0.0.1:")
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

#ifndef THUNDER_PORT
#define THUNDER_PORT "9998"
#endif

using namespace WPEFramework;

/* L2TestMock consturctor */
L2TestMocks::L2TestMocks()
{
    p_rfcApiImplMock    = new NiceMock <RfcApiImplMock>;
    p_iarmBusImplMock   = new NiceMock <IarmBusImplMock>;
    p_readprocImplMock  = new NiceMock <readprocImplMock>;
    p_wrapsImplMock     = new NiceMock <WrapsImplMock>;
    p_hostImplMock      = new NiceMock <HostImplMock>;
    p_videoOutputPortConfigImplMock = new NiceMock <VideoOutputPortConfigImplMock>;
    p_managerImplMock   = new NiceMock <ManagerImplMock>;
    p_videoOutputPortMock = new NiceMock <VideoOutputPortMock>;
    p_udevImplMock      = new NiceMock <UdevImplMock>;
    p_rBusApiImplMock   = new NiceMock <RBusApiImplMock>;
    p_telemetryApiImplMock  = new NiceMock <TelemetryApiImplMock>;
    p_audioOutputPortMock  = new NiceMock <AudioOutputPortMock>;
    p_audioStereoModeMock  = new NiceMock <AudioStereoModeMock>;
    p_btmgrImplMock     = new NiceMock <BtmgrImplMock>;
    p_hdmiInputImplMock  = new NiceMock <HdmiInputImplMock>;
    p_libUSBApiImplMock  = new NiceMock <libUSBImplMock>;
    p_tvSettingsImplMock  = new NiceMock <TvSettingsImplMock>;
    p_tr181ApiImplMock = new NiceMock <Tr181ApiImplMock>;
    p_videoDeviceMock  = new NiceMock <VideoDeviceMock>;
    p_videoDFCMock = new NiceMock <VideoDFCMock>;
    p_displayMock  = new testing::NiceMock <DisplayMock>;
    p_videoResolutionMock  = new NiceMock <VideoResolutionMock>;
    p_frontPanelIndicatorMock  = new NiceMock <FrontPanelIndicatorMock>;
    p_frontPanelConfigImplMock  = new testing::NiceMock <FrontPanelConfigMock>;
   //  p_hostImplMock  = new testing::NiceMock <HostImplMock>;
   //  p_frontPanelIndicatorMock  = new testing::NiceMock <FrontPanelIndicatorMock>;
   //  testing::NiceMock<FrontPanelIndicatorMock> frontPanelIndicatorMock;
   p_audioOutputPortTypeMock  = new NiceMock <AudioOutputPortTypeMock>;
   p_videoOutputPortTypeMock  = new NiceMock <VideoOutputPortTypeMock>;
   p_essRMgrMock       = new NiceMock <EssRMgrMock>;
   p_rdkloggerImplMock = new NiceMock <RdkLoggerMilestoneImplMock>;
   p_drmScreenCaptureApiImplMock = new NiceMock<DRMScreenCaptureApiImplMock>;
   p_dsFPDMock = new NiceMock <dsFPDMock>;
   p_systemAudioPlatformAPIMock = new NiceMock<SystemAudioPlatformAPIMock>;
   p_renderSessionMock = new NiceMock <RenderSessionMock>;
   p_dobbyProxyMock = new NiceMock <DobbyProxyMock>;
   p_ipcservicemock  = new NiceMock <IpcServiceMock>;
   p_mockOmiProxy = new NiceMock<MockOmiProxy>;
   p_powerManagerHalMock = new NiceMock<PowerManagerHalMock>;
   p_mfrMock = new NiceMock<mfrMock>;
   p_connectionMock = new NiceMock<ConnectionImplMock>;
   p_libCCECMock = new NiceMock<LibCCECImplMock>;
   p_messageEncoderMock = new NiceMock<MessageEncoderMock>;
   p_messageDecoderMock = new NiceMock<MessageDecoderMock>;
   p_colorImplMock  = new NiceMock <ColorMock>;

#ifdef RDK_SERVICE_CPC_L2_TEST    
    p_secSecurityApiImplMock      = new NiceMock <SecSecurityApiImplMock>;
    p_keyProvisionObjectImplMock      = new NiceMock <KeyProvisionObjectImplMock>;
    p_keyProvisionDirectorySerializerImplMock = new NiceMock<secclient::KeyProvisionDirectorySerializerImplMock>;
    p_credentialUtilsImplMock = new NiceMock<secclient::CredentialUtilsImplMock>;
    p_keyProvisionClientImplMock = new NiceMock<secclient::KeyProvisionClientImplMock>;
    p_secApiProvisionerImplMock = new NiceMock<secclient::SecApiProvisionerImplMock>;
#endif    

   TEST_LOG("Inside L2TestMocks constructor");

#ifdef L2_TEST_OOP_RPC
    MockAccessor<IarmBusImpl>::setPtr(p_iarmBusImplMock);
    MockAccessor<readprocImpl>::setPtr(p_readprocImplMock);
    MockAccessor<RBusApiImpl>::setPtr(p_rBusApiImplMock);
    MockAccessor<RfcApiImpl>::setPtr(p_rfcApiImplMock);
    MockAccessor<PowerManagerImpl>::setPtr(p_powerManagerHalMock);
    MockAccessor<mfrImpl>::setPtr(p_mfrMock);
#else
    IarmBus::setImpl(p_iarmBusImplMock);
    RfcApi::setImpl(p_rfcApiImplMock);
    ProcImpl::setImpl(p_readprocImplMock);
    Wraps::setImpl(p_wrapsImplMock);
    RBusApi::setImpl(p_rBusApiImplMock);
    TelemetryApi::setImpl(p_telemetryApiImplMock);
    device::Host::setImpl(p_hostImplMock);
    device::VideoOutputPortConfig::setImpl(p_videoOutputPortConfigImplMock);
    device::Manager::setImpl(p_managerImplMock);
    device::VideoOutputPort::setImpl(p_videoOutputPortMock);
    Udev::setImpl(p_udevImplMock);
    Btmgr::setImpl(p_btmgrImplMock);
    device::HdmiInput::setImpl(p_hdmiInputImplMock);
    libusbApi::setImpl(p_libUSBApiImplMock);
    TvSettings::setImpl(p_tvSettingsImplMock);
    Tr181Api::setImpl(p_tr181ApiImplMock);
    EssRMgrApi::setImpl(p_essRMgrMock);
    DRMScreenCaptureApi::getInstance().impl = p_drmScreenCaptureApiImplMock;
    dsFPDApi::setImpl(p_dsFPDMock);
    SystemAudioPlatformMockImpl::setImpl(p_systemAudioPlatformAPIMock);

    device::AudioOutputPort::setImpl(p_audioOutputPortMock);
    device::AudioStereoMode::setImpl(p_audioStereoModeMock);
    device::VideoDevice::setImpl(p_videoDeviceMock);
    device::VideoDFC::setImpl(p_videoDFCMock);
    device::VideoResolution::setImpl(p_videoResolutionMock);

    device::Display::setImpl(p_displayMock);
    device::FrontPanelConfig::setImpl(p_frontPanelConfigImplMock);
    device::FrontPanelIndicator::setImpl(p_frontPanelIndicatorMock);
    device::AudioOutputPortType::setImpl(p_audioOutputPortTypeMock);
    device::VideoOutputPortType::setImpl(p_videoOutputPortTypeMock);
    device::FrontPanelIndicator::Color::setImpl(p_colorImplMock);
    RenderSession::setImpl(p_renderSessionMock);
    DobbyProxy::setImpl(p_dobbyProxyMock);
    IpcService::setImpl(p_ipcservicemock);
    omi::OmiProxy::setImpl(p_mockOmiProxy);
    PowerManagerAPI::setImpl(p_powerManagerHalMock);
    mfr::setImpl(p_mfrMock);
    LibCCEC::setImpl(p_libCCECMock);
    Connection::setImpl(p_connectionMock);
    MessageEncoder::setImpl(p_messageEncoderMock);
    MessageDecoder::setImpl(p_messageDecoderMock);
#ifdef RDK_SERVICE_CPC_L2_TEST    
    SecSecurityApi::setImpl(p_secSecurityApiImplMock);
    secclient::KeyProvisionObject::setImpl(p_keyProvisionObjectImplMock);
    secclient::KeyProvisionDirectorySerializer::setImpl(p_keyProvisionDirectorySerializerImplMock);
    secclient::CredentialUtils::setImpl(p_credentialUtilsImplMock);
    secclient::KeyProvisionClient::setImpl(p_keyProvisionClientImplMock);
    secclient::SecApiProvisioner::setImpl(p_secApiProvisionerImplMock);
#endif
#endif /* L2_TEST_OOP_RPC */

    thunder_address = THUNDER_ADDRESS + std::string(THUNDER_PORT);
    (void)Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), thunder_address);

#ifdef L2_TEST_OOP_RPC
    ActivateService("org.rdk.MockPlugin");
#endif /* L2_TEST_OOP_RPC */

}

/* L2TestMock Destructor */
L2TestMocks::~L2TestMocks()
{

#ifdef L2_TEST_OOP_RPC
   DeactivateService("org.rdk.MockPlugin");
   MockAccessor<IarmBusImpl>::setPtr(nullptr);
   MockAccessor<readprocImpl>::setPtr(nullptr);
   MockAccessor<RBusApiImpl>::setPtr(nullptr);
   MockAccessor<RfcApiImpl>::setPtr(nullptr);
   MockAccessor<PowerManagerImpl>::setPtr(nullptr);
   MockAccessor<mfrImpl>::setPtr(nullptr);
#else
   IarmBus::setImpl(nullptr);
   RfcApi::setImpl(nullptr);
   ProcImpl::setImpl(nullptr);
   Wraps::setImpl(nullptr);
   RBusApi::setImpl(nullptr);
   TelemetryApi::setImpl(nullptr);
   device::Host::setImpl(nullptr);
   device::VideoOutputPortConfig::setImpl(nullptr);
   device::Manager::setImpl(nullptr);
   device::VideoOutputPort::setImpl(nullptr);
   Udev::setImpl(nullptr);
   Btmgr::setImpl(nullptr);
   device::HdmiInput::setImpl(nullptr);
   libusbApi::setImpl(nullptr);
   TvSettings::setImpl(nullptr);
   Tr181Api::setImpl(nullptr);
   EssRMgrApi::setImpl(nullptr);
   DRMScreenCaptureApi::getInstance().impl = nullptr;
   dsFPDApi::setImpl(nullptr);
   SystemAudioPlatformMockImpl::setImpl(nullptr);
   device::AudioOutputPort::setImpl(nullptr);
   device::AudioStereoMode::setImpl(nullptr);
   device::AudioOutputPort::setImpl(nullptr);
   device::AudioStereoMode::setImpl(nullptr);
   device::Manager::setImpl(nullptr);
   device::VideoDevice::setImpl(nullptr);
   device::VideoDFC::setImpl(nullptr);
   device::Display::setImpl(nullptr);
   device::VideoResolution::setImpl(nullptr);
   device::FrontPanelConfig::setImpl(nullptr);
   device::FrontPanelIndicator::setImpl(nullptr);
   device::FrontPanelIndicator::Color::setImpl(nullptr);
   device::AudioOutputPortType::setImpl(nullptr);
   device::VideoOutputPortType::setImpl(nullptr);
   RenderSession::setImpl(nullptr);
   DobbyProxy::setImpl(nullptr);
   IpcService::setImpl(nullptr);
   omi::OmiProxy::setImpl(nullptr);
   PowerManagerAPI::setImpl(nullptr);
   mfr::setImpl(nullptr);
   Connection::setImpl(nullptr);
   LibCCEC::setImpl(nullptr);
   MessageEncoder::setImpl(nullptr);
   MessageDecoder::setImpl(nullptr);

#ifdef RDK_SERVICE_CPC_L2_TEST   
   SecSecurityApi::setImpl(nullptr);
   secclient::KeyProvisionObject::setImpl(nullptr);
   secclient::KeyProvisionDirectorySerializer::setImpl(nullptr);
   secclient::CredentialUtils::setImpl(nullptr);
   secclient::KeyProvisionClient::setImpl(nullptr);
   secclient::SecApiProvisioner::setImpl(nullptr);
#endif   
 
#endif /* L2_TEST_OOP_RPC */

   TEST_LOG("Inside L2TestMocks destructor");

   if (p_iarmBusImplMock != nullptr)
   {
        delete p_iarmBusImplMock;
        p_iarmBusImplMock = nullptr;
   }

   if (p_essRMgrMock != nullptr)
   {
         delete p_essRMgrMock;
         p_essRMgrMock = nullptr;
   }

   if (p_dsFPDMock != nullptr)
   {
         delete p_dsFPDMock;
         p_dsFPDMock = nullptr;
   }

   if (p_rfcApiImplMock != nullptr)
   {
        delete p_rfcApiImplMock;
        p_rfcApiImplMock = nullptr;
   }

   if (p_readprocImplMock != nullptr)
   {
        delete p_readprocImplMock;
        p_readprocImplMock = nullptr;
   }

   if (p_wrapsImplMock != nullptr)
   {
        delete p_wrapsImplMock;
        p_wrapsImplMock = nullptr;
   }


   if (p_rBusApiImplMock != nullptr)
   {
        delete p_rBusApiImplMock;
        p_rBusApiImplMock = nullptr;
   }


   if (p_telemetryApiImplMock != nullptr)
   {
        delete p_telemetryApiImplMock;
        p_telemetryApiImplMock = nullptr;
   }


   if (p_hostImplMock != nullptr)
   {
      delete p_hostImplMock;
      p_hostImplMock = nullptr;
   }

   if (p_videoOutputPortConfigImplMock != nullptr)
   {
      delete p_videoOutputPortConfigImplMock;
      p_videoOutputPortConfigImplMock = nullptr;
   }

   // device::AudiodeoOutputPortConfig::setImpl(nullptr);
   // if (p_audioOutputPortConfigImplMock != nullptr)
   // {
   //    delete p_audioOutputPortConfigImplMock;
   //    p_audioOutputPortConfigImplMock = nullptr;
   // }

   if (p_audioOutputPortMock != nullptr)
   {
      delete p_audioOutputPortMock;
      p_audioOutputPortMock = nullptr;
   }

   if (p_audioStereoModeMock != nullptr)
   {
      delete p_audioStereoModeMock;
      p_audioStereoModeMock = nullptr;
   }


   // device::AudiodeoOutputPortConfig::setImpl(nullptr);
   // if (p_audioOutputPortConfigImplMock != nullptr)
   // {
   //    delete p_audioOutputPortConfigImplMock;
   //    p_audioOutputPortConfigImplMock = nullptr;
   // }


   if (p_audioOutputPortMock != nullptr)
   {
      delete p_audioOutputPortMock;
      p_audioOutputPortMock = nullptr;
   }


   if (p_audioStereoModeMock != nullptr)
   {
      delete p_audioStereoModeMock;
      p_audioStereoModeMock = nullptr;
   }

   if (p_managerImplMock != nullptr)
   {
      delete p_managerImplMock;
      p_managerImplMock = nullptr;
   }

   if (p_videoOutputPortMock != nullptr)
   {
      delete p_videoOutputPortMock;
      p_videoOutputPortMock = nullptr;
   }


   if (p_udevImplMock != nullptr)
   {
        delete p_udevImplMock;
        p_udevImplMock = nullptr;
   }


   if (p_btmgrImplMock != nullptr)
   {
        delete p_btmgrImplMock;
        p_btmgrImplMock = nullptr;
   }

   if (p_hdmiInputImplMock != nullptr)
   {
      delete p_hdmiInputImplMock;
      p_hdmiInputImplMock = nullptr;
   }


   if (p_libUSBApiImplMock != nullptr)
   {
        delete p_libUSBApiImplMock;
        p_libUSBApiImplMock = nullptr;
   }

   if (p_tvSettingsImplMock != nullptr)
   {
      delete p_tvSettingsImplMock;
      p_tvSettingsImplMock = nullptr;
   }

   if (p_tr181ApiImplMock != nullptr)
   {
      delete p_tr181ApiImplMock;
      p_tr181ApiImplMock = nullptr;
   }

   if (p_videoDeviceMock != nullptr)
   {
      delete p_videoDeviceMock;
      p_videoDeviceMock = nullptr;
   }

   if (p_videoDFCMock != nullptr)
   {
      delete p_videoDFCMock;
      p_videoDFCMock = nullptr;
   }

   if (p_displayMock != nullptr)
   {
         delete p_displayMock;
         p_displayMock = nullptr;
   }
   
   if (p_videoResolutionMock != nullptr)
   {
      delete p_videoResolutionMock;
      p_videoResolutionMock = nullptr;
   }
    
   if (p_frontPanelConfigImplMock != nullptr)
   {
      delete p_frontPanelConfigImplMock;
      p_frontPanelConfigImplMock = nullptr;
   }

   if (p_audioOutputPortTypeMock != nullptr)
   {
      delete p_audioOutputPortTypeMock;
      p_audioOutputPortTypeMock = nullptr;
   }

   if (p_videoOutputPortTypeMock != nullptr)
   {
      delete p_videoOutputPortTypeMock;
      p_videoOutputPortTypeMock = nullptr;
   }

   if (p_drmScreenCaptureApiImplMock != nullptr)
   {
      delete p_drmScreenCaptureApiImplMock;
      p_drmScreenCaptureApiImplMock = nullptr;
   }

   if (p_systemAudioPlatformAPIMock != nullptr)
   {
      delete p_systemAudioPlatformAPIMock;
      p_systemAudioPlatformAPIMock = nullptr;
   }

   if (p_renderSessionMock != nullptr)
   {
      delete p_renderSessionMock;
      p_renderSessionMock = nullptr;
   }

//    device::FrontPanelIndicator::setImpl(nullptr);
//   // device::FrontPanelIndicator::getInstance().impl = &frontPanelIndicatorMock;

   if (p_frontPanelIndicatorMock!= nullptr)
   {
      delete p_frontPanelIndicatorMock;
      p_frontPanelIndicatorMock = nullptr;
   }

   if (p_colorImplMock != nullptr)
   {
      delete p_colorImplMock;
      p_colorImplMock = nullptr;
   }

   if (p_dobbyProxyMock != nullptr)
   {
      delete p_dobbyProxyMock;
      p_dobbyProxyMock = nullptr;
   }

   if (p_ipcservicemock != nullptr)
   {
      delete p_ipcservicemock;
      p_ipcservicemock = nullptr;
   }

   if(p_mockOmiProxy != nullptr)
   {
      delete p_mockOmiProxy;
      p_mockOmiProxy = nullptr;
   }

   if(p_powerManagerHalMock != nullptr)
   {
      delete p_powerManagerHalMock;
      p_powerManagerHalMock = nullptr;
   }

   if(p_mfrMock != nullptr)
   {
      delete p_mfrMock;
      p_mfrMock = nullptr;
   }

   if (p_connectionMock != nullptr)
   {
      delete p_connectionMock;
      p_connectionMock = nullptr;
   }

   if (p_libCCECMock != nullptr)
   {
      delete p_libCCECMock;
      p_libCCECMock = nullptr;
   }

   if (p_messageEncoderMock != nullptr)
   {
      delete p_messageEncoderMock;
      p_messageEncoderMock = nullptr;
   }

   if (p_messageDecoderMock != nullptr)
   {
      delete p_messageDecoderMock;
      p_messageDecoderMock = nullptr;
   }

#ifdef RDK_SERVICE_CPC_L2_TEST
   if (p_secSecurityApiImplMock != nullptr)
   {
        delete p_secSecurityApiImplMock;
        p_secSecurityApiImplMock = nullptr;
   }

   if (p_keyProvisionObjectImplMock != nullptr)
   {
        delete p_keyProvisionObjectImplMock;
        p_keyProvisionObjectImplMock = nullptr;
   }

   if (p_keyProvisionDirectorySerializerImplMock != nullptr) {
       delete p_keyProvisionDirectorySerializerImplMock;
       p_keyProvisionDirectorySerializerImplMock = nullptr;
   }

   if (p_credentialUtilsImplMock != nullptr) {
       delete p_credentialUtilsImplMock;
       p_credentialUtilsImplMock = nullptr;
   }

   if (p_keyProvisionClientImplMock != nullptr) {
       delete p_keyProvisionClientImplMock;
       p_keyProvisionClientImplMock = nullptr;
   }

   if (p_secApiProvisionerImplMock != nullptr) {
       delete p_secApiProvisionerImplMock;
       p_secApiProvisionerImplMock = nullptr;
   }
#endif


}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[in] params Method parameters
 * @param[out] results Method results
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, JsonObject &params, JsonObject &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string message;
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   params.ToString(message);
   TEST_LOG("Invoking %s.%s, parameters %s\n", callsign, method, message.c_str());

   results = JsonObject();
   status = jsonrpc.Invoke<JsonObject, JsonObject>(INVOKE_TIMEOUT, std::string(method), params, results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());
   if(status == 11) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep for 1 second 
        status = jsonrpc.Invoke<JsonObject, JsonObject>(INVOKE_TIMEOUT, std::string(method), params, results);
        results.ToString(reply);
        TEST_LOG("Retry Status %u, results %s", status, reply.c_str());
    }   

   return status;
}

/**
 * @brief Invoke a service method (jsonObject Return Type)
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[out] results Method results (JsonObject)
 * @return Zero (Core::ERROR_NONE) on success or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, JsonObject &results)
{
   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   TEST_LOG("Invoking %s.%s \n", callsign, method);

   results = JsonObject();
   status = jsonrpc.Invoke<void, JsonObject>(INVOKE_TIMEOUT, std::string(method), results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;
}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[in] params Method parameters
 * @param[out] results Method results
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, JsonObject &params, Core::JSON::String &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string message;
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   params.ToString(message);
   TEST_LOG("Invoking %s.%s, parameters %s\n", callsign, method, message.c_str());

   status = jsonrpc.Invoke<JsonObject, Core::JSON::String>(INVOKE_TIMEOUT, std::string(method), params, results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;

}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[in] params Method parameters
 * @param[out] results Method results with string format
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, JsonObject &params, Core::JSON::Boolean &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string message;
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   params.ToString(message);
   TEST_LOG("Invoking %s.%s, parameters %s\n", callsign, method, message.c_str());

   status = jsonrpc.Invoke<JsonObject, Core::JSON::Boolean>(INVOKE_TIMEOUT, std::string(method), params, results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;

}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[out] results Method results
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, Core::JSON::Boolean &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   TEST_LOG("Invoking %s.%s \n", callsign, method);

   status = jsonrpc.Invoke<void, Core::JSON::Boolean>(INVOKE_TIMEOUT, std::string(method), results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;
}

/**
 * @brief Invoke a service method
 *
 * @param[in] callsign Service callsign
 * @param[in] method Method name
 * @param[out] results Method results
 * @return Zero (Core::ERROR_NONE) on succes or another value on error
 */
uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, Core::JSON::String &results)
{

   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   TEST_LOG("Invoking %s.%s \n", callsign, method);

   status = jsonrpc.Invoke<void, Core::JSON::String>(INVOKE_TIMEOUT, std::string(method), results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;

}

uint32_t L2TestMocks::InvokeServiceMethod(const char *callsign, const char *method, Core::JSON::Double &results)
{
   JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(std::string(callsign), TEST_CALLSIGN);
   std::string reply;
   uint32_t status = Core::ERROR_NONE;

   TEST_LOG("Invoking %s.%s \n", callsign, method);

   status = jsonrpc.Invoke<void, Core::JSON::Double>(INVOKE_TIMEOUT, std::string(method), results);

   results.ToString(reply);
   TEST_LOG("Status %u, results %s", status, reply.c_str());

   return status;
}

/**
* @brief Activate a service plugin
*
* @param[in] callsign Service callsign
* @return Zero (Core::ERROR_NONE) on succes or another value on error
*/
uint32_t L2TestMocks::ActivateService(const char *callsign)
{
   JsonObject params;
   JsonObject result;
   uint32_t status = Core::ERROR_GENERAL;

   if(callsign != NULL)
   {
     params["callsign"] = callsign;
     status = InvokeServiceMethod("Controller.1", "activate", params, result);
   }

   return status;
}

/**
* @brief Deactivate a service plugin
*
* @param[in] callsign Service callsign
* @return Zero (Core::ERROR_NONE) on succes or another value on error
*/
uint32_t L2TestMocks::DeactivateService(const char *callsign)
{
   JsonObject params;
   JsonObject result;
   uint32_t status = Core::ERROR_GENERAL;

   if(callsign != NULL)
   {
      params["callsign"] = callsign;
      status = InvokeServiceMethod("Controller.1", "deactivate", params, result);
   }
   return status;
}


