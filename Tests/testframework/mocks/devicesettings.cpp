/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "devicesettings.h"
#include <gmock/gmock.h>

namespace device
{

const int AudioStereoMode::kMono = dsAUDIO_STEREO_MONO;
const int AudioStereoMode::kStereo = dsAUDIO_STEREO_STEREO;
const int AudioStereoMode::kSurround = dsAUDIO_STEREO_SURROUND;
const int AudioStereoMode::kPassThru = dsAUDIO_STEREO_PASSTHRU;
const int AudioStereoMode::kDD = dsAUDIO_STEREO_DD;
const int AudioStereoMode::kDDPlus = dsAUDIO_STEREO_DDPLUS;
const int AudioStereoMode::kMax = dsAUDIO_STEREO_MAX;

AudioStereoModeImpl* AudioStereoMode::impl = nullptr;

AudioStereoMode::AudioStereoMode() {}
AudioStereoMode::AudioStereoMode(int id) {
    audioMode = id;
}

void AudioStereoMode::setImpl(AudioStereoModeImpl* newImpl) {
        // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

std::string AudioStereoMode::toString()
{
    EXPECT_NE(impl, nullptr);
    return impl->toString();
}
const std::string& AudioStereoMode::getName() const {
    EXPECT_NE(impl, nullptr);
    return impl->getName();
}

const int AudioOutputPortType::kHDMI 			= dsAUDIOPORT_TYPE_HDMI;
const int AudioOutputPortType::kSPDIF 			= dsAUDIOPORT_TYPE_SPDIF;
const int AudioOutputPortType::kARC             = dsAUDIOPORT_TYPE_HDMI_ARC;
const int AudioOutputPortType::kSPEAKER         = dsAUDIOPORT_TYPE_SPEAKER;
const int AudioOutputPortType::kHEADPHONE       = dsAUDIOPORT_TYPE_HEADPHONE;

AudioOutputPortTypeImpl* AudioOutputPortType::impl = nullptr;

AudioOutputPortType::AudioOutputPortType() {}

void AudioOutputPortType::setImpl(AudioOutputPortTypeImpl* newImpl) {
        // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}
int AudioOutputPortType::getId() const {
    EXPECT_NE(impl, nullptr);
    return impl->getId();
}

AudioOutputPortImpl* AudioOutputPort::impl = nullptr;

AudioOutputPort::AudioOutputPort() {}

void AudioOutputPort::setImpl(AudioOutputPortImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

AudioOutputPort& AudioOutputPort::getInstance() {
    static AudioOutputPort instance;
    return instance;
}

const std::string& AudioOutputPort::getName() const {
    EXPECT_NE(impl, nullptr);
    return impl->getName();
}

const AudioOutputPortType& AudioOutputPort::getType() const {
    EXPECT_NE(impl, nullptr);
    return impl->getType();
}

std::vector<std::string> AudioOutputPort::getMS12AudioProfileList() const {
    EXPECT_NE(impl, nullptr);
    return impl->getMS12AudioProfileList();
}

void AudioOutputPort::getAudioCapabilities(int* capabilities) {
    EXPECT_NE(impl, nullptr);
    impl->getAudioCapabilities(capabilities);
}

void AudioOutputPort::getMS12Capabilities(int* capabilities) {
    EXPECT_NE(impl, nullptr);
    impl->getMS12Capabilities(capabilities);
}

bool AudioOutputPort::isAudioMSDecode() {
    EXPECT_NE(impl, nullptr);
    return impl->isAudioMSDecode();
}
bool AudioOutputPort::getEnablePersist() {
    EXPECT_NE(impl, nullptr);
    return impl->getEnablePersist();
}
void AudioOutputPort::getHdmiArcPortId(int *portId)
{
    EXPECT_NE(impl, nullptr);
    return impl->getHdmiArcPortId(portId);
}
bool AudioOutputPort::isConnected() {
    EXPECT_NE(impl, nullptr);
    return impl->isConnected();
}
void AudioOutputPort::setDRCMode(int DRCMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->setDRCMode(DRCMode);
}
void AudioOutputPort::setCompression(int compresionLevel)
{
    EXPECT_NE(impl, nullptr);
    return impl->setCompression(compresionLevel);
}
int AudioOutputPort::getCompression() {
    EXPECT_NE(impl, nullptr);
    return impl->getCompression();
}
void AudioOutputPort::setDolbyVolumeMode(bool dolbyVolumeMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->setDolbyVolumeMode(dolbyVolumeMode);
}
void AudioOutputPort::setDialogEnhancement(int enhancerlevel)
{
    EXPECT_NE(impl, nullptr);
    return impl->setDialogEnhancement(enhancerlevel);
}
int AudioOutputPort::getDialogEnhancement()
{
    EXPECT_NE(impl, nullptr);
    return impl->getDialogEnhancement();
}
void AudioOutputPort::setIntelligentEqualizerMode (int intelligentEqualizerMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->setIntelligentEqualizerMode (intelligentEqualizerMode);
}
int AudioOutputPort::getIntelligentEqualizerMode()
{
    EXPECT_NE(impl, nullptr);
    return impl->getIntelligentEqualizerMode();
}
void AudioOutputPort::setGraphicEqualizerMode (int graphicEqualizerMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->setGraphicEqualizerMode (graphicEqualizerMode);
}
int AudioOutputPort::getGraphicEqualizerMode()
{
    EXPECT_NE(impl, nullptr);
    return impl->getGraphicEqualizerMode();
}
void AudioOutputPort::setMS12AudioProfile(std::string audioProfileName)
{
    EXPECT_NE(impl, nullptr);
    return impl->setMS12AudioProfile(audioProfileName);
}
void AudioOutputPort::resetVolumeLeveller()
{
    EXPECT_NE(impl, nullptr);
    return impl->resetVolumeLeveller();
}
void AudioOutputPort::resetSurroundVirtualizer()
{
    EXPECT_NE(impl, nullptr);
    return impl->resetSurroundVirtualizer();
}
void AudioOutputPort::resetBassEnhancer()
{
    EXPECT_NE(impl, nullptr);
    return impl->resetBassEnhancer();
}
void AudioOutputPort::resetDialogEnhancement()
{
    EXPECT_NE(impl, nullptr);
    return impl->resetDialogEnhancement();
}
void AudioOutputPort::getSupportedARCTypes(int *types)
{
    EXPECT_NE(impl, nullptr);
    return impl->getSupportedARCTypes(types);
}
void AudioOutputPort::setEnablePersist(bool pEnable)
{
    EXPECT_NE(impl, nullptr);
    return impl->setEnablePersist(pEnable);
}
dsError_t AudioOutputPort::setEnablePort(bool pEnable)
{
    EXPECT_NE(impl, nullptr);
    return impl->setEnablePort(pEnable);
}
bool AudioOutputPort::isMuted()
{
    EXPECT_NE(impl, nullptr);
    return impl->isMuted();
}
void AudioOutputPort::setAudioAtmosOutputMode (int enable)
{
    EXPECT_NE(impl, nullptr);
    return impl->setAudioAtmosOutputMode(enable);
}
bool AudioOutputPort::getSinkDeviceAtmosCapability(dsATMOSCapability_t& atmosCapability)
{
    EXPECT_NE(impl, nullptr);
    return impl->getSinkDeviceAtmosCapability(atmosCapability);
}
void AudioOutputPort::setAudioDelayOffset(int audioDelayOffsetMs)
{
    EXPECT_NE(impl, nullptr);
    return impl->setAudioDelayOffset(audioDelayOffsetMs);
}
void AudioOutputPort::setAudioDelay(int audioDelayMs)
{
    EXPECT_NE(impl, nullptr);
    return impl->setAudioDelay(audioDelayMs);
}
void AudioOutputPort::getAudioDelay(uint32_t audioDelayMs)
{
    EXPECT_NE(impl, nullptr);
    return impl->getAudioDelay(audioDelayMs);
}
void AudioOutputPort::getAudioDelayOffset(uint32_t audioDelayOffsetMs)
{
    EXPECT_NE(impl, nullptr);
    return impl->getAudioDelayOffset(audioDelayOffsetMs);
}
void AudioOutputPort::getSecondaryLanguage(std::string secondaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->getSecondaryLanguage(secondaryLanguage);
}
void AudioOutputPort::setSecondaryLanguage(std::string secondaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->setSecondaryLanguage(secondaryLanguage);
}
void AudioOutputPort::setPrimaryLanguage(std::string primaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->setPrimaryLanguage(primaryLanguage);
}
void AudioOutputPort::getPrimaryLanguage(std::string primaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->getPrimaryLanguage(primaryLanguage);
}
void AudioOutputPort::getFaderControl(int *mixerBalance)
{
    EXPECT_NE(impl, nullptr);
    return impl->getFaderControl(mixerBalance);
}
void AudioOutputPort::setFaderControl(int mixerBalance)
{
    EXPECT_NE(impl, nullptr);
    return impl->setFaderControl(mixerBalance);
}
void AudioOutputPort::setAssociatedAudioMixing(bool mixing)
{
    EXPECT_NE(impl, nullptr);
    return impl->setAssociatedAudioMixing(mixing);
}
void AudioOutputPort::setGain(float newGain)
{
    EXPECT_NE(impl, nullptr);
    return impl->setGain(newGain);
}
void AudioOutputPort::setMISteering(bool MISteering)
{
    EXPECT_NE(impl, nullptr);
    return impl->setMISteering(MISteering);
}
void AudioOutputPort::setSurroundVirtualizer(dsSurroundVirtualizer_t surroundVirtualizer)
{
    EXPECT_NE(impl, nullptr);
    return impl->setSurroundVirtualizer(surroundVirtualizer);
}
void AudioOutputPort::setBassEnhancer(int bassBoost)
{
    EXPECT_NE(impl, nullptr);
    return impl->setBassEnhancer(bassBoost);
}
void AudioOutputPort::enableSurroundDecoder(bool enableSurroundDecoder)
{
    EXPECT_NE(impl, nullptr);
    return impl->enableSurroundDecoder(enableSurroundDecoder);
}
void AudioOutputPort::setVolumeLeveller(dsVolumeLeveller_t volumeLeveller)
{
    EXPECT_NE(impl, nullptr);
    return impl->setVolumeLeveller(volumeLeveller);
}
bool AudioOutputPort::getMISteering()
{
    EXPECT_NE(impl, nullptr);
    return impl->getMISteering();
}
dsSurroundVirtualizer_t AudioOutputPort::getSurroundVirtualizer()
{
    EXPECT_NE(impl, nullptr);
    return impl->getSurroundVirtualizer();
}
int AudioOutputPort::getDRCMode()
{
    EXPECT_NE(impl, nullptr);
    return impl->getDRCMode();
}
float AudioOutputPort::getLevel()
{
    EXPECT_NE(impl, nullptr);
    return impl->getLevel();
}
float AudioOutputPort::getGain()
{
    EXPECT_NE(impl, nullptr);
    return impl->getGain();
}
bool AudioOutputPort::isSurroundDecoderEnabled()
{
    EXPECT_NE(impl, nullptr);
    return impl->isSurroundDecoderEnabled();
}
int AudioOutputPort::getBassEnhancer()
{
    EXPECT_NE(impl, nullptr);
    return impl->getBassEnhancer();
}
dsVolumeLeveller_t AudioOutputPort::getVolumeLeveller()
{
    EXPECT_NE(impl, nullptr);
    return impl->getVolumeLeveller();
}
bool AudioOutputPort::getStereoAuto()
{
    EXPECT_NE(impl, nullptr);
    return impl->getStereoAuto();
}
void AudioOutputPort::setStereoAuto(bool stereoAuto, bool persist)
{
    EXPECT_NE(impl, nullptr);
    return impl->setStereoAuto(stereoAuto, persist);
}
std::string AudioOutputPort::getMS12AudioProfile()
{
    EXPECT_NE(impl, nullptr);
    return impl->getMS12AudioProfile();
}
void AudioOutputPort::setMS12AudioProfileSetttingsOverride(std::string audioProfileState,std::string audioProfileName,std::string audioProfileSettingsName, std::string audioProfileSettingValue)
{
    EXPECT_NE(impl, nullptr);
    return impl->setMS12AudioProfileSetttingsOverride(audioProfileState,audioProfileName,audioProfileSettingsName,audioProfileSettingValue);
}
void AudioOutputPort::setLevel(float level)
{
    EXPECT_NE(impl, nullptr);
    return impl->setLevel(level);
}
void AudioOutputPort::setMuted(bool muted)
{
    EXPECT_NE(impl, nullptr);
    return impl->setMuted(muted);
}
void AudioOutputPort::getAssociatedAudioMixing(bool *mixing)
{
    EXPECT_NE(impl, nullptr);
    return impl->getAssociatedAudioMixing(mixing);
}
bool AudioOutputPort::isEnabled()
{
    EXPECT_NE(impl, nullptr);
    return impl->isEnabled();
}
void AudioOutputPort::setSAD(std::vector<int> sad_list)
{
    EXPECT_NE(impl, nullptr);
    return impl->setSAD(sad_list);
}
void AudioOutputPort::reInitializeAudioOutputPort()
{
    EXPECT_NE(impl, nullptr);
    return impl->reInitializeAudioOutputPort();
}
void AudioOutputPort::enableARC(dsAudioARCTypes_t type, bool enable)
{
    EXPECT_NE(impl, nullptr);
    return impl->enableARC(type, enable);
}
uint32_t AudioOutputPort::getDolbyVolumeMode() const
{
    EXPECT_NE(impl, nullptr);
    return impl->getDolbyVolumeMode();
}
const std::vector<device::AudioStereoMode> AudioOutputPort::getSupportedStereoModes() const
{
    EXPECT_NE(impl, nullptr);
    return impl->getSupportedStereoModes();  // Ensure this also returns a std::vector
}

AudioStereoMode AudioOutputPort::getStereoMode(const bool toPersist) const{
    EXPECT_NE(impl, nullptr);
    return impl->getStereoMode(toPersist);
}

AudioStereoMode AudioOutputPort::getStereoMode() const{
    EXPECT_NE(impl, nullptr);
    return impl->getStereoMode();
}

void AudioOutputPort::setStereoMode(const std::string &mode, bool persist)
{
    EXPECT_NE(impl, nullptr);
    return impl->setStereoMode(mode, persist);
}

CompositeInputImpl* CompositeInput::impl = nullptr;

CompositeInput::CompositeInput() {}

void CompositeInput::setImpl(CompositeInputImpl* newImpl) {
        // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

CompositeInput& CompositeInput::getInstance() {
    static CompositeInput instance;
    return instance;
}

uint8_t CompositeInput::getNumberOfInputs() const {
    EXPECT_NE(impl, nullptr);
    return impl->getNumberOfInputs();
}

bool CompositeInput::isPortConnected(int8_t port) const {
    EXPECT_NE(impl, nullptr);
    return impl->isPortConnected(port);
}

void CompositeInput::selectPort(int8_t port) const {
    EXPECT_NE(impl, nullptr);
    impl->selectPort(port);
}

void CompositeInput::scaleVideo(int32_t x, int32_t y, int32_t width, int32_t height) const {
    EXPECT_NE(impl, nullptr);
    impl->scaleVideo(x, y, width, height);
}


HdmiInputImpl* HdmiInput::impl = nullptr;

HdmiInput::HdmiInput() {}

void HdmiInput::setImpl(HdmiInputImpl* newImpl) {
        // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

HdmiInput& HdmiInput::getInstance() {
    static HdmiInput instance;
    return instance;
}

uint8_t HdmiInput::getNumberOfInputs() const {
    EXPECT_NE(impl, nullptr);
    return impl->getNumberOfInputs();
}

bool HdmiInput::isPortConnected(int8_t port) const {
    EXPECT_NE(impl, nullptr);
    return impl->isPortConnected(port);
}

std::string HdmiInput::getCurrentVideoMode() const {
    EXPECT_NE(impl, nullptr);
    return impl->getCurrentVideoMode();
}

void HdmiInput::getCurrentVideoModeObj(dsVideoPortResolution_t& resolution) const {
    EXPECT_NE(impl, nullptr);
    impl->getCurrentVideoModeObj(resolution);
}

/*void HdmiInput::selectPort(int8_t port) const {
    EXPECT_NE(impl, nullptr);
    impl->selectPort(port);
}*/

void HdmiInput::selectPort(int8_t Port,bool audioMix , int videoPlane ,bool topMost ) const {
    EXPECT_NE(impl, nullptr);
    impl->selectPort(Port,audioMix,videoPlane,topMost);
}

void HdmiInput::scaleVideo(int32_t x, int32_t y, int32_t width, int32_t height) const {
    EXPECT_NE(impl, nullptr);
    impl->scaleVideo(x, y, width, height);
}

void HdmiInput::getEDIDBytesInfo(int iHdmiPort, std::vector<uint8_t>& edid) const {
    EXPECT_NE(impl, nullptr);
    impl->getEDIDBytesInfo(iHdmiPort, edid);
}

void HdmiInput::getHDMISPDInfo(int iHdmiPort, std::vector<uint8_t>& data) const {
    EXPECT_NE(impl, nullptr);
    impl->getHDMISPDInfo(iHdmiPort, data);
}

void HdmiInput::setEdidVersion(int iHdmiPort, int iEdidVersion) const {
    EXPECT_NE(impl, nullptr);
    impl->setEdidVersion(iHdmiPort, iEdidVersion);
}

void HdmiInput::getEdidVersion(int iHdmiPort, int* iEdidVersion) const {
    EXPECT_NE(impl, nullptr);
    impl->getEdidVersion(iHdmiPort, iEdidVersion);
}

void HdmiInput::getHdmiALLMStatus(int iHdmiPort, bool* allmStatus) const {
    EXPECT_NE(impl, nullptr);
    impl->getHdmiALLMStatus(iHdmiPort, allmStatus);
}

void HdmiInput::getVRRStatus(int iHdmiPort, dsHdmiInVrrStatus_t *vrrStatus) const {
    EXPECT_NE(impl, nullptr);
    impl->getVRRStatus(iHdmiPort, vrrStatus);
}

void HdmiInput::getSupportedGameFeatures(std::vector<std::string>& featureList) const {
    EXPECT_NE(impl, nullptr);
    impl->getSupportedGameFeatures(featureList);
}

    void HdmiInput::setEdid2AllmSupport(int iport, bool allmSupport) const {
    EXPECT_NE(impl, nullptr);
    impl->setEdid2AllmSupport(iport,allmSupport);
}

    void HdmiInput::setVRRSupport(int iport, bool vrrSupport) const {
    EXPECT_NE(impl, nullptr);
    impl->setVRRSupport(iport,vrrSupport);
}

    void HdmiInput::getAVLatency(int *audio_output_delay, int *video_latency) const {
    EXPECT_NE(impl, nullptr);
    impl->getAVLatency(audio_output_delay,video_latency);
}


    void HdmiInput::getEdid2AllmSupport(int iport, bool *allmSupport) const {
    EXPECT_NE(impl, nullptr);
    impl->getEdid2AllmSupport(iport,allmSupport);
}

    void HdmiInput::getVRRSupport(int iport, bool *vrrSupport) const {
    EXPECT_NE(impl, nullptr);
    impl->getVRRSupport(iport,vrrSupport);
}

void HdmiInput::getHdmiVersion(int iport, dsHdmiMaxCapabilityVersion_t *capVersion) const {
     EXPECT_NE(impl, nullptr);
     impl->getHdmiVersion(iport,capVersion);
}

dsError_t HdmiInput::getHDMIARCPortId(int &portId) const {
     EXPECT_NE(impl, nullptr);
     return impl->getHDMIARCPortId(portId);
}

SleepModeImpl* SleepMode::impl = nullptr;

SleepMode::SleepMode() {}

void SleepMode::setImpl(SleepModeImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

SleepMode& SleepMode::getInstance() {
    static SleepMode instance;
    return instance;
}

SleepMode& SleepMode::getInstance(int id) {
    EXPECT_NE(impl, nullptr);
    return impl->getInstanceById(id);
}

SleepMode& SleepMode::getInstance(const std::string &name) {
    EXPECT_NE(impl, nullptr);
    return impl->getInstanceByName(name);
}

List<SleepMode> SleepMode::getSleepModes() {
    EXPECT_NE(impl, nullptr);
    return impl->getSleepModes();
}

const std::string& SleepMode::toString() const {
    EXPECT_NE(impl, nullptr);
    return impl->toString();
}

VideoDFCImpl* VideoDFC::impl = nullptr;

VideoDFC::VideoDFC() {}

void VideoDFC::setImpl(VideoDFCImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

const std::string& VideoDFC::getName() const {
    EXPECT_NE(impl, nullptr);
    return impl->getName();
}


VideoDeviceImpl* VideoDevice::impl = nullptr;

VideoDevice::VideoDevice() {}

void VideoDevice::setImpl(VideoDeviceImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

VideoDevice& VideoDevice::getInstance() {
    static VideoDevice instance;
    return instance;
}

int VideoDevice::getFRFMode(int* frfmode) const {
    EXPECT_NE(impl, nullptr);
    return impl->getFRFMode(frfmode);
}

int VideoDevice::setFRFMode(int frfmode) const {
    EXPECT_NE(impl, nullptr);
    return impl->setFRFMode(frfmode);
}

int VideoDevice::getCurrentDisframerate(char* framerate) const {
    EXPECT_NE(impl, nullptr);
    return impl->getCurrentDisframerate(framerate);
}

int VideoDevice::setDisplayframerate(const char* framerate) const {
    EXPECT_NE(impl, nullptr);
    return impl->setDisplayframerate(framerate);
}
void VideoDevice::getHDRCapabilities(int* capabilities)
{
    EXPECT_NE(impl, nullptr);
    return impl->getHDRCapabilities(capabilities);
}
void VideoDevice::getSettopSupportedResolutions(std::list<std::string>& resolutions)
{
    EXPECT_NE(impl, nullptr);
    return impl->getSettopSupportedResolutions(resolutions);
}

const int FrameRate::kUnknown 		= dsVIDEO_FRAMERATE_UNKNOWN;
const int FrameRate::k24 			= dsVIDEO_FRAMERATE_24;
const int FrameRate::k25 			= dsVIDEO_FRAMERATE_25;
const int FrameRate::k30 			= dsVIDEO_FRAMERATE_30;
const int FrameRate::k60 			= dsVIDEO_FRAMERATE_60;
const int FrameRate::k23dot98 		= dsVIDEO_FRAMERATE_23dot98;
const int FrameRate::k29dot97 		= dsVIDEO_FRAMERATE_29dot97;
const int FrameRate::k50 			= dsVIDEO_FRAMERATE_50;
const int FrameRate::k59dot94 		= dsVIDEO_FRAMERATE_59dot94;
const int FrameRate::kMax 			= dsVIDEO_FRAMERATE_MAX;

FrameRate::FrameRate(int id){
            _id = id;
        }
FrameRate::FrameRate(float value){
            _value = value;
        }

VideoResolutionImpl* VideoResolution::impl = nullptr;

void VideoResolution::setImpl(VideoResolutionImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}
const std::string& VideoResolution::getName() const {
    return impl->getName();
}

const PixelResolution& VideoResolution::getPixelResolution() const{
    EXPECT_NE(impl, nullptr);
    return impl->getPixelResolution();
}

const FrameRate& VideoResolution::getFrameRate() const{
    EXPECT_NE(impl, nullptr);
    return impl->getFrameRate();
}

const int VideoOutputPortType::kRF 			= dsVIDEOPORT_TYPE_RF;
const int VideoOutputPortType::kBaseband 	= dsVIDEOPORT_TYPE_BB;
const int VideoOutputPortType::kSVideo 		= dsVIDEOPORT_TYPE_SVIDEO;
const int VideoOutputPortType::k1394 		= dsVIDEOPORT_TYPE_1394;
const int VideoOutputPortType::kDVI 		= dsVIDEOPORT_TYPE_DVI;
const int VideoOutputPortType::kComponent 	= dsVIDEOPORT_TYPE_COMPONENT;
const int VideoOutputPortType::kHDMI 		= dsVIDEOPORT_TYPE_HDMI;
const int VideoOutputPortType::kInternal 	= dsVIDEOPORT_TYPE_INTERNAL;
const int VideoOutputPortType::kMax 		= dsVIDEOPORT_TYPE_MAX;

VideoOutputPortTypeImpl* VideoOutputPortType::impl = nullptr;
VideoOutputPortType::VideoOutputPortType(){}

void VideoOutputPortType::setImpl(VideoOutputPortTypeImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

int VideoOutputPortType::getId() const {
    EXPECT_NE(impl, nullptr);
    return impl->getId();
}

const List<VideoResolution> VideoOutputPortType::getSupportedResolutions() const {
    EXPECT_NE(impl, nullptr);
    return impl->getSupportedResolutions();
}


DisplayImpl* Display::impl = nullptr;


void Display::setImpl(DisplayImpl* newImpl) {
        // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Display& Display::getInstance() {
    static Display instance;
    return instance;
}

void Display::getEDIDBytes(std::vector<uint8_t>& edid) {
    EXPECT_NE(impl, nullptr);
    impl->getEDIDBytes(edid);
}

bool Display::isConnectedDeviceRepeater()
{
    EXPECT_NE(impl, nullptr);
    return impl->isConnectedDeviceRepeater();
}

int Display::getSurroundMode()
{
    EXPECT_NE(impl, nullptr);
    return impl->getSurroundMode();
}

void Display::setAllmEnabled(bool enable) {
    EXPECT_NE(impl, nullptr);
    impl->setAllmEnabled(enable);
}
void Display::setAVIContentType(int contentType) {
    EXPECT_NE(impl, nullptr);
    impl->setAVIContentType(contentType);
}
void Display::setAVIScanInformation(int scanInfo) {
    EXPECT_NE(impl, nullptr);
    impl->setAVIScanInformation(scanInfo);
}

VideoOutputPortImpl* VideoOutputPort::impl = nullptr;

VideoOutputPort::VideoOutputPort() {}

void VideoOutputPort::setImpl(VideoOutputPortImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

VideoOutputPort& VideoOutputPort::getInstance() {
    static VideoOutputPort instance;
    return instance;
}

const VideoOutputPortType& VideoOutputPort::getType() const {
    EXPECT_NE(impl, nullptr);
    return impl->getType();
}

const std::string& VideoOutputPort::getName() const {
    EXPECT_NE(impl, nullptr);
    return impl->getName();
}

const VideoResolution& VideoOutputPort::getDefaultResolution() const {
    EXPECT_NE(impl, nullptr);
    return impl->getDefaultResolution();
}

int VideoOutputPort::getHDCPProtocol() {
    EXPECT_NE(impl, nullptr);
    return impl->getHDCPProtocol();
}

int VideoOutputPort::getHDCPReceiverProtocol() {
    EXPECT_NE(impl, nullptr);
    return impl->getHDCPReceiverProtocol();
}

int VideoOutputPort::getHDCPCurrentProtocol() {
    EXPECT_NE(impl, nullptr);
    return impl->getHDCPCurrentProtocol();
}

int VideoOutputPort::getHDCPStatus() {
    EXPECT_NE(impl, nullptr);
    return impl->getHDCPStatus();
}

int VideoOutputPort::GetHdmiPreference(){
    EXPECT_NE(impl, nullptr);
    return impl->GetHdmiPreference();
}

bool VideoOutputPort::SetHdmiPreference(dsHdcpProtocolVersion_t hdcpProtocol){
    EXPECT_NE(impl, nullptr);
    return impl->SetHdmiPreference(hdcpProtocol);
}

int VideoOutputPort::getColorSpace() const {
    EXPECT_NE(impl, nullptr);
    return impl->getColorSpace();
}

int VideoOutputPort::getColorDepth() const {
    EXPECT_NE(impl, nullptr);
    return impl->getColorDepth();
}

int VideoOutputPort::getQuantizationRange() const {
    EXPECT_NE(impl, nullptr);
    return impl->getQuantizationRange();
}

bool VideoOutputPort::IsOutputHDR() {
    EXPECT_NE(impl, nullptr);
    return impl->IsOutputHDR();
}

AudioOutputPort& VideoOutputPort::getAudioOutputPort() {
    EXPECT_NE(impl, nullptr);
    return impl->getAudioOutputPort();
}

bool VideoOutputPort::isDisplayConnected() {
    EXPECT_NE(impl, nullptr);
    return impl->isDisplayConnected();
}
int VideoOutputPort::getVideoEOTF()
{
    EXPECT_NE(impl, nullptr);
    return impl->getVideoEOTF();
}
bool VideoOutputPort::setScartParameter(std::string sScartParameter, std::string sScartParameterData)
{
    EXPECT_NE(impl, nullptr);
    return impl->setScartParameter( sScartParameter, sScartParameterData);
}

bool VideoOutputPort::isContentProtected() {
    EXPECT_NE(impl, nullptr);
    return impl->isContentProtected();
}

Display& VideoOutputPort::getDisplay() {
    EXPECT_NE(impl, nullptr);
    return impl->getDisplay();
}

bool VideoOutputPort::isActive() {
    EXPECT_NE(impl, nullptr);
    return impl->isActive();
}

VideoOutputPortConfigImpl* VideoOutputPortConfig::impl = nullptr;

VideoOutputPortConfig::VideoOutputPortConfig() {}

void VideoOutputPortConfig::setImpl(VideoOutputPortConfigImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

VideoOutputPortConfig& VideoOutputPortConfig::getInstance() {
    static VideoOutputPortConfig instance;
    return instance;
}

VideoOutputPortType& VideoOutputPortConfig::getPortType(int id) {
    EXPECT_NE(impl, nullptr);
    return impl->getPortType(id);
}

VideoOutputPort& VideoOutputPortConfig::getPort(const std::string& name) {
    EXPECT_NE(impl, nullptr);
    return impl->getPort(name);
}

ManagerImpl* Manager::impl = nullptr;

Manager::Manager() {}

void Manager::setImpl(ManagerImpl* newImpl) {
        // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Manager& Manager::getInstance() {
    static Manager instance;
    return instance;
}

void Manager::Initialize() {
    EXPECT_NE(impl, nullptr);
    impl->Initialize();
}

void Manager::DeInitialize() {
    EXPECT_NE(impl, nullptr);
    impl->DeInitialize();
}

HostImpl* Host::impl = nullptr;


void Host::setImpl(HostImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Host& Host::getInstance() {
    static Host instance;
    return instance;
}

SleepMode Host::getPreferredSleepMode() {
    EXPECT_NE(impl, nullptr);
    return impl->getPreferredSleepMode();
}

int Host::setPreferredSleepMode(const SleepMode mode) {
    EXPECT_NE(impl, nullptr);
    return impl->setPreferredSleepMode(mode);
}

List<SleepMode> Host::getAvailableSleepModes() {
    EXPECT_NE(impl, nullptr);
    return impl->getAvailableSleepModes();
}

List<VideoOutputPort> Host::getVideoOutputPorts() {
    EXPECT_NE(impl, nullptr);
    return impl->getVideoOutputPorts();
}

List<AudioOutputPort> Host::getAudioOutputPorts() {
    EXPECT_NE(impl, nullptr);
    return impl->getAudioOutputPorts();
}

void Host::getMS12ConfigDetails(std::string type){
    EXPECT_NE(impl, nullptr);
    return impl->getMS12ConfigDetails(type);
}
bool Host::isHDMIOutPortPresent()
{
    EXPECT_NE(impl, nullptr);
    return impl->isHDMIOutPortPresent();
}
void Host::getSecondaryLanguage(std::string secondaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->getSecondaryLanguage(secondaryLanguage);
}
void Host::setSecondaryLanguage(std::string secondaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->setSecondaryLanguage(secondaryLanguage);
}
void Host::getSinkDeviceAtmosCapability(dsATMOSCapability_t atmosCapability)
{
    EXPECT_NE(impl, nullptr);
    return impl->getSinkDeviceAtmosCapability(atmosCapability);
}
void Host::getFaderControl(int *mixerBalance)
{
    EXPECT_NE(impl, nullptr);
    return impl->getFaderControl(mixerBalance);
}
void Host::setFaderControl(int mixerBalance)
{
    EXPECT_NE(impl, nullptr);
    return impl->setFaderControl(mixerBalance);
}
void Host::setPrimaryLanguage(std::string primaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->setPrimaryLanguage(primaryLanguage);
}
void Host::getPrimaryLanguage(std::string primaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->getPrimaryLanguage(primaryLanguage);
}
void Host::setAudioAtmosOutputMode (int enable)
{
    EXPECT_NE(impl, nullptr);
    return impl->setAudioAtmosOutputMode(enable);
}
void Host::setAssociatedAudioMixing(bool mixing)
{
    EXPECT_NE(impl, nullptr);
    return impl->setAssociatedAudioMixing(mixing);
}
void Host::getCurrentAudioFormat(dsAudioFormat_t audioFormat)
{
    EXPECT_NE(impl, nullptr);
    return impl->getCurrentAudioFormat(audioFormat);
}
void Host::getAssociatedAudioMixing(bool *mixing)
{
    EXPECT_NE(impl, nullptr);
    return impl->getAssociatedAudioMixing(mixing);
}

dsError_t Host::Register(Host::IDisplayEvents* listener, const std::string& clientName)
{
    EXPECT_NE(impl, nullptr);
    return impl->Register(listener);
}
dsError_t Host::UnRegister(Host::IDisplayEvents* listener)
{
    EXPECT_NE(impl, nullptr);
    return impl->UnRegister(listener);
}

dsError_t Host::Register(Host::IAudioOutputPortEvents* listener, const std::string& clientName)
{
    EXPECT_NE(impl, nullptr);
    return impl->Register(listener);
}
dsError_t Host::UnRegister(Host::IAudioOutputPortEvents* listener)
{
    EXPECT_NE(impl, nullptr);
    return impl->UnRegister(listener);
}

dsError_t Host::Register(Host::IHdmiInEvents* listener, const std::string& clientName)
{
    EXPECT_NE(impl, nullptr);
    return impl->Register(listener);
}
dsError_t Host::UnRegister(Host::IHdmiInEvents* listener)
{
    EXPECT_NE(impl, nullptr);
    return impl->UnRegister(listener);
}

dsError_t Host::Register(Host::IDisplayDeviceEvents* listener, const std::string& clientName)
{
    EXPECT_NE(impl, nullptr);
    return impl->Register(listener);
}
dsError_t Host::UnRegister(Host::IDisplayDeviceEvents* listener)
{
    EXPECT_NE(impl, nullptr);
    return impl->UnRegister(listener);
}

dsError_t Host::Register(Host::IVideoDeviceEvents* listener, const std::string& clientName)
{
    EXPECT_NE(impl, nullptr);
    return impl->Register(listener);
}
dsError_t Host::UnRegister(Host::IVideoDeviceEvents* listener)
{
    EXPECT_NE(impl, nullptr);
    return impl->UnRegister(listener);
}

dsError_t Host::Register(Host::IVideoOutputPortEvents* listener, const std::string& clientName)
{
    EXPECT_NE(impl, nullptr);
    return impl->Register(listener);
}
dsError_t Host::UnRegister(Host::IVideoOutputPortEvents* listener)
{
    EXPECT_NE(impl, nullptr);
    return impl->UnRegister(listener);
}
dsError_t Host::Register(Host::ICompositeInEvents* listener, const std::string& clientName)
{
    EXPECT_NE(impl, nullptr);
    return impl->Register(listener);
}
dsError_t Host::UnRegister(Host::ICompositeInEvents* listener)
{
    EXPECT_NE(impl, nullptr);
    return impl->UnRegister(listener);
}

IDisplayEventsImpl* Host::IDisplayEvents::impl = nullptr;

void Host::IDisplayEvents::setImpl(IDisplayEventsImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Host::IDisplayEvents& Host::IDisplayEvents::getInstance(){
    static Host::IDisplayEvents instance;
    return instance;
}

void Host::IDisplayEvents::OnDisplayRxSense(dsDisplayEvent_t displayEvent)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnDisplayRxSense(displayEvent);
}


IAudioOutputPortEventsImpl* Host::IAudioOutputPortEvents::impl = nullptr;

void Host::IAudioOutputPortEvents::setImpl(IAudioOutputPortEventsImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Host::IAudioOutputPortEvents& Host::IAudioOutputPortEvents::getInstance(){
    static Host::IAudioOutputPortEvents instance;
    return instance;
}

void Host::IAudioOutputPortEvents::OnAudioOutHotPlug(dsAudioPortType_t portType, uint32_t uiPortNumber, bool isPortConnected)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAudioOutHotPlug(portType, uiPortNumber, isPortConnected);
}
void Host::IAudioOutputPortEvents::OnAudioFormatUpdate(dsAudioFormat_t audioFormat)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAudioFormatUpdate(audioFormat);
}
void Host::IAudioOutputPortEvents::OnDolbyAtmosCapabilitiesChanged(dsATMOSCapability_t atmosCapability, bool status)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnDolbyAtmosCapabilitiesChanged(atmosCapability, status);
}
void Host::IAudioOutputPortEvents::OnAudioPortStateChanged(dsAudioPortState_t audioPortState)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAudioPortStateChanged(audioPortState);

}
void Host::IAudioOutputPortEvents::OnAssociatedAudioMixingChanged(bool mixing)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAssociatedAudioMixingChanged(mixing);
}
void Host::IAudioOutputPortEvents::OnAudioFaderControlChanged(int mixerBalance)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAudioFaderControlChanged(mixerBalance);
}
void Host::IAudioOutputPortEvents::OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAudioPrimaryLanguageChanged(primaryLanguage);
}
void Host::IAudioOutputPortEvents::OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAudioSecondaryLanguageChanged(secondaryLanguage);
}

void Host::IAudioOutputPortEvents::OnAudioModeEvent(dsAudioPortType_t audioPortType, dsAudioStereoMode_t audioStereoMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAudioModeEvent(audioPortType, audioStereoMode);
}

void Host::IAudioOutputPortEvents::OnAudioLevelChangedEvent(int audioLevel)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnAudioLevelChangedEvent(audioLevel);
}

IDisplayDeviceEventsImpl* Host::IDisplayDeviceEvents::impl = nullptr;

void Host::IDisplayDeviceEvents::setImpl(IDisplayDeviceEventsImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Host::IDisplayDeviceEvents& Host::IDisplayDeviceEvents::getInstance(){
    static Host::IDisplayDeviceEvents instance;
    return instance;
}

void Host::IDisplayDeviceEvents::OnDisplayHDMIHotPlug(dsDisplayEvent_t displayEvent)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnDisplayHDMIHotPlug(displayEvent);
}

IHdmiInEventsImpl* Host::IHdmiInEvents::impl = nullptr;

void Host::IHdmiInEvents::setImpl(IHdmiInEventsImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Host::IHdmiInEvents& Host::IHdmiInEvents::getInstance(){
    static Host::IHdmiInEvents instance;
    return instance;
}

void Host::IHdmiInEvents::OnHdmiInEventHotPlug(dsHdmiInPort_t port, bool isConnected)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHdmiInEventHotPlug(port, isConnected);
}

void Host::IHdmiInEvents::OnHdmiInEventSignalStatus(dsHdmiInPort_t port, dsHdmiInSignalStatus_t signalStatus)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHdmiInEventSignalStatus(port, signalStatus);
}

void Host::IHdmiInEvents::OnHdmiInEventStatus(dsHdmiInPort_t activePort, bool isPresented)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHdmiInEventStatus(activePort, isPresented);
}

void Host::IHdmiInEvents::OnHdmiInVideoModeUpdate(dsHdmiInPort_t port, const dsVideoPortResolution_t& videoPortResolution)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHdmiInVideoModeUpdate(port, videoPortResolution);
}

void Host::IHdmiInEvents::OnHdmiInAllmStatus(dsHdmiInPort_t port, bool allmStatus)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHdmiInAllmStatus(port, allmStatus);
}

void Host::IHdmiInEvents::OnHdmiInAVIContentType(dsHdmiInPort_t port, dsAviContentType_t aviContentType)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHdmiInAVIContentType(port, aviContentType);
}

void Host::IHdmiInEvents::OnHdmiInVRRStatus(dsHdmiInPort_t port, dsVRRType_t vrrType)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHdmiInVRRStatus(port, vrrType);
}

void Host::IHdmiInEvents::OnHdmiInAVLatency(int audioDelay, int videoDelay)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHdmiInAVLatency(audioDelay, videoDelay);
}

ICompositeInEventsImpl* Host::ICompositeInEvents::impl = nullptr;

void Host::ICompositeInEvents::setImpl(ICompositeInEventsImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Host::ICompositeInEvents& Host::ICompositeInEvents::getInstance(){
    static Host::ICompositeInEvents instance;
    return instance;
}

void Host::ICompositeInEvents::OnCompositeInHotPlug(dsCompositeInPort_t port, bool isConnected)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnCompositeInHotPlug(port, isConnected);
}

void Host::ICompositeInEvents::OnCompositeInSignalStatus(dsCompositeInPort_t port, dsCompInSignalStatus_t signalStatus)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnCompositeInSignalStatus(port, signalStatus);
}

void Host::ICompositeInEvents::OnCompositeInStatus(dsCompositeInPort_t activePort, bool isPresented)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnCompositeInStatus(activePort, isPresented);
}

void Host::ICompositeInEvents::OnCompositeInVideoModeUpdate(dsCompositeInPort_t activePort, dsVideoPortResolution_t videoResolution)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnCompositeInVideoModeUpdate(activePort, videoResolution);
}

IVideoDeviceEventsImpl* Host::IVideoDeviceEvents::impl = nullptr;

void Host::IVideoDeviceEvents::setImpl(IVideoDeviceEventsImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Host::IVideoDeviceEvents& Host::IVideoDeviceEvents::getInstance(){
    static Host::IVideoDeviceEvents instance;
    return instance;
}

void Host::IVideoDeviceEvents::OnZoomSettingsChanged(dsVideoZoom_t zoomSetting)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnZoomSettingsChanged(zoomSetting);
}

void Host::IVideoDeviceEvents::OnDisplayFrameratePreChange(const std::string& frameRate)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnDisplayFrameratePreChange(frameRate);
}

void Host::IVideoDeviceEvents::OnDisplayFrameratePostChange(const std::string& frameRate)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnDisplayFrameratePostChange(frameRate);
}

IVideoOutputPortEventsImpl* Host::IVideoOutputPortEvents::impl = nullptr;

void Host::IVideoOutputPortEvents::setImpl(IVideoOutputPortEventsImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

Host::IVideoOutputPortEvents& Host::IVideoOutputPortEvents::getInstance(){
    static Host::IVideoOutputPortEvents instance;
    return instance;
}

void Host::IVideoOutputPortEvents::OnResolutionPreChange(const int width, const int height)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnResolutionPreChange(width, height);
}
void Host::IVideoOutputPortEvents::OnResolutionPostChange(const int width, const int height)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnResolutionPostChange(width, height);
}
void Host::IVideoOutputPortEvents::OnVideoFormatUpdate(dsHDRStandard_t videoFormatHDR)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnVideoFormatUpdate(videoFormatHDR);
}

void Host::IVideoOutputPortEvents::OnHDCPStatusChange(dsHdcpStatus_t hdcpStatus)
{
    EXPECT_NE(impl, nullptr);
    return impl->OnHDCPStatusChange(hdcpStatus);
}

void VideoOutputPort::getSupportedTvResolutions(int *tvResolutions) {
    EXPECT_NE(impl, nullptr);
    return impl->getSupportedTvResolutions(tvResolutions);
}

List<VideoDevice> Host::getVideoDevices() {
    EXPECT_NE(impl, nullptr);
    return impl->getVideoDevices();
}
const VideoDFC& VideoDevice::getDFC()
{
    EXPECT_NE(impl, nullptr);
    return impl->getDFC();
}
void VideoDevice::setDFC(std::string zoomSetting)
{
    EXPECT_NE(impl, nullptr);
    return impl->setDFC(zoomSetting);
}
void VideoOutputPort::setResolution(std::string resolution, bool persist, bool isIgnoreEdid)
{
    EXPECT_NE(impl, nullptr);
    return impl->setResolution(resolution,persist,isIgnoreEdid);
}
bool VideoOutputPort::getCurrentOutputSettings(int& videoEOTF, int& matrixCoefficients, int& colorSpace, int& colorDepth, int& quantizationRange)
{
    EXPECT_NE(impl, nullptr);
    return impl->getCurrentOutputSettings(videoEOTF, matrixCoefficients, colorSpace, colorDepth, quantizationRange);
}
void VideoOutputPort::setPreferredColorDepth(dsDisplayColorDepth_t colorDepth, bool persist)
{
    EXPECT_NE(impl, nullptr);
    return impl->setPreferredColorDepth(colorDepth, persist);
}
void VideoOutputPort::getColorDepthCapabilities(unsigned int *capabilities)
{
    EXPECT_NE(impl, nullptr);
    return impl->getColorDepthCapabilities(capabilities);
}
void VideoOutputPort::getTVHDRCapabilities(int* capabilities)
{
    EXPECT_NE(impl, nullptr);
    return impl->getTVHDRCapabilities(capabilities);
}
bool VideoOutputPort::setForceHDRMode(dsHDRStandard_t mode)
{
    EXPECT_NE(impl, nullptr);
    return impl->setForceHDRMode(mode);
}
int VideoOutputPort::getPreferredColorDepth(bool persist)
{
    EXPECT_NE(impl, nullptr);
    return impl->getPreferredColorDepth(persist);
}
const VideoResolution& VideoOutputPort::getResolution() const
{
    EXPECT_NE(impl, nullptr);
    return impl->getResolution();
}

VideoOutputPort& Host::getVideoOutputPort(const std::string& name) {
    EXPECT_NE(impl, nullptr);
    return impl->getVideoOutputPort(name);
}

AudioOutputPort& Host::getAudioOutputPort(const std::string& name) {
    EXPECT_NE(impl, nullptr);
    return impl->getAudioOutputPort(name);
}

void Host::getHostEDID(std::vector<uint8_t>& edid) const {
    EXPECT_NE(impl, nullptr);
    return impl->getHostEDID(edid);
}

std::string Host::getDefaultVideoPortName() {
    EXPECT_NE(impl, nullptr);
    return impl->getDefaultVideoPortName();
}

std::string Host::getDefaultAudioPortName() {
    EXPECT_NE(impl, nullptr);
    return impl->getDefaultAudioPortName();
}

void Host::setAudioMixerLevels(dsAudioInput_t input, int volume) {
    EXPECT_NE(impl, nullptr);
    return impl->setAudioMixerLevels(input,volume);
}

FrontPanelIndicator::ColorImpl* FrontPanelIndicator::Color::impl = nullptr;

FrontPanelIndicator::Color::Color() {}

void FrontPanelIndicator::Color::setImpl(ColorImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

FrontPanelIndicator::Color& FrontPanelIndicator::Color::getInstance() {
    static Color instance;
    return instance;
}

const FrontPanelIndicator::Color& FrontPanelIndicator::Color::getInstance(int id) {
    EXPECT_NE(impl, nullptr);
    return impl->getInstanceById(id);
}

const FrontPanelIndicator::Color& FrontPanelIndicator::Color::getInstance(const std::string& name) {
    EXPECT_NE(impl, nullptr);
    return impl->getInstanceByName(name);
}

const int FrontPanelIndicator::Color::kWhite = dsFPD_COLOR_WHITE;

std::string FrontPanelIndicator::Color::getName() const {
    EXPECT_NE(impl, nullptr);
    return impl->getName();
}

FrontPanelIndicator::Color::~Color() {}


FrontPanelTextDisplayImpl* FrontPanelTextDisplay::impl = nullptr;

FrontPanelTextDisplay::FrontPanelTextDisplay() {}

void FrontPanelTextDisplay::setImpl(FrontPanelTextDisplayImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

FrontPanelTextDisplay& FrontPanelTextDisplay::getInstance(const std::string& name) {
    EXPECT_NE(impl, nullptr);
    return impl->getInstanceByName(name);
}

FrontPanelTextDisplay& FrontPanelTextDisplay::getInstance(int id) {
    EXPECT_NE(impl, nullptr);
    return impl->getInstanceById(id);
}

FrontPanelTextDisplay& FrontPanelTextDisplay::getInstance() {
    static FrontPanelTextDisplay instance;
    return instance;
}

const int FrontPanelTextDisplay::kModeClock12Hr = dsFPD_TIME_12_HOUR;
const int FrontPanelTextDisplay::kModeClock24Hr = dsFPD_TIME_24_HOUR;

int FrontPanelTextDisplay::getCurrentTimeFormat() const {
    EXPECT_NE(impl, nullptr);
    return impl->getCurrentTimeFormat();
}

void FrontPanelTextDisplay::setTimeFormat(const int iTimeFormat) {
    EXPECT_NE(impl, nullptr);
    impl->setTimeFormat(iTimeFormat);
}

void FrontPanelTextDisplay::setText(const std::string text) {
    EXPECT_NE(impl, nullptr);
    impl->setText(text);
}

void FrontPanelTextDisplay::setMode(int mode) {
    EXPECT_NE(impl, nullptr);
    impl->setMode(mode);
}

int FrontPanelTextDisplay::getTextBrightness() const {
    EXPECT_NE(impl, nullptr);
    return impl->getTextBrightness();
}

void FrontPanelTextDisplay::setTextBrightness(const int brightness) const {
    EXPECT_NE(impl, nullptr);
    impl->setTextBrightness(brightness);
}

FrontPanelIndicator::FrontPanelIndicatorImpl* FrontPanelIndicator::impl = nullptr;

FrontPanelIndicator::FrontPanelIndicator() {}

void FrontPanelIndicator::setImpl(FrontPanelIndicatorImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

FrontPanelIndicator& FrontPanelIndicator::getInstance() {
    static FrontPanelIndicator instance;
    return instance;
}

FrontPanelIndicator& FrontPanelIndicator::getInstance(int id) {
    EXPECT_NE(impl, nullptr);
    return impl->getInstanceInt(id);
}

FrontPanelIndicator& FrontPanelIndicator::getInstance(const std::string& name) {
    EXPECT_NE(impl, nullptr);
    return impl->getInstanceString(name);
}

void FrontPanelIndicator::setState(const bool bState) const {
    EXPECT_NE(impl, nullptr);
    return impl->setState(bState);
}

std::string FrontPanelIndicator::getName() const {
    EXPECT_NE(impl, nullptr);
    return impl->getName();
}

// void FrontPanelIndicator::setBrightness(const int& brightness) {
//     EXPECT_NE(impl, nullptr);
//     impl->setBrightnessInt(brightness);
// }

void FrontPanelIndicator::setBrightness(const int brightness, const bool toPersist) const {
    EXPECT_NE(impl, nullptr);
    return impl->setBrightness(brightness, toPersist);
}

int FrontPanelIndicator::getBrightness(const bool persist) const {
    EXPECT_NE(impl, nullptr);
    return impl->getBrightness(persist);
}

void FrontPanelIndicator::setColor(const Color &newColor, const bool toPersist) const {
    EXPECT_NE(impl, nullptr);
    return impl->setColor(newColor, toPersist);
}

void FrontPanelIndicator::setColor(const uint32_t color, const bool toPersist) const {
    EXPECT_NE(impl, nullptr);
    return impl->setColorInt(color, toPersist);
}

void FrontPanelIndicator::getBrightnessLevels(int &levels, int &min, int &max) const {
    EXPECT_NE(impl, nullptr);
    return impl->getBrightnessLevels(levels, min, max);
}

device::List<FrontPanelIndicator::Color> FrontPanelIndicator::getSupportedColors() const {
    EXPECT_NE(impl, nullptr);
    return impl->getSupportedColors();
}

int FrontPanelIndicator::getColorMode() const {
    EXPECT_NE(impl, nullptr);
    return impl->getColorMode();
}

std::string FrontPanelIndicator::getColorName() const {
    EXPECT_NE(impl, nullptr);
    return impl->getColorName();
}

FrontPanelConfigImpl* FrontPanelConfig::impl = nullptr;

FrontPanelConfig::FrontPanelConfig() {}

void FrontPanelConfig::setImpl(FrontPanelConfigImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

FrontPanelConfig& FrontPanelConfig::getInstance() {
    static FrontPanelConfig instance;
    return instance;
}

List<FrontPanelIndicator> FrontPanelConfig::getIndicators() {
    EXPECT_NE(impl, nullptr);
    return impl->getIndicators();
}

FrontPanelTextDisplay& FrontPanelConfig::getTextDisplay(const std::string& name) {
    EXPECT_NE(impl, nullptr);
    return impl->getTextDisplay(name);
}

List<FrontPanelTextDisplay> FrontPanelConfig::getTextDisplays() {
    EXPECT_NE(impl, nullptr);
    return impl->getTextDisplays();
}

FrontPanelTextDisplay& FrontPanelConfig::getTextDisplay(int id) {
    EXPECT_NE(impl, nullptr);
    return impl->getTextDisplay(id);
}

FrontPanelTextDisplay& FrontPanelConfig::getTextDisplay() const {
    EXPECT_NE(impl, nullptr);
    return impl->getTextDisplay();
}
}

rtObjectBaseImpl* rtObjectBase::impl = nullptr;
rtObjectBase& rtObjectBase::getInstance() {
    static rtObjectBase instance;
    return instance;
}

void rtObjectBase::setImpl(rtObjectBaseImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

rtError rtObjectBase::set(const char* name, const char* value) {
    EXPECT_NE(impl, nullptr);
    return impl->set(name, value);
}

rtError rtObjectBase::set(const char* name, bool value) {
    EXPECT_NE(impl, nullptr);
    return impl->set(name, value);
}

rtError rtObjectBase::set(const char* name, const rtValue& value) {
    EXPECT_NE(impl, nullptr);
    return impl->set(name, value);
}


rtObjectRefImpl* rtObjectRef::impl = nullptr;

rtObjectRef& rtObjectRef::getInstance() {
    static rtObjectRef instance;
    return instance;
}

void rtObjectRef::setImpl(rtObjectRefImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}
rtObjectRef::rtObjectRef() {}

rtObjectRef::rtObjectRef(const rtObjectRef&) = default;

rtObjectRef::rtObjectRef(const rtMapObject* o) {
    delete o;
    o = nullptr;
}

rtObjectRef& rtObjectRef::operator=(rtMapObject* o) {
    delete o;
    o = nullptr;
    return *this;
}

rtObjectRef& rtObjectRef::operator=(const rtObjectRef&) {
    return *this;
}

rtObjectRef& rtObjectRef::operator=(rtIObject* o) {
    asn(o);
    return *this;
}

rtObjectRef& rtObjectRef::operator=(rtObjectRef&&) = default;

rtError rtObjectRef::send(const char* messageName) {
    EXPECT_NE(impl, nullptr);
    return impl->send(messageName);
}

rtError rtObjectRef::send(const char* messageName, const char* method, rtFunctionCallback* callback) {
    EXPECT_NE(impl, nullptr);
    return impl->send(messageName, method, callback);
}

rtError rtObjectRef::send(const char* messageName, const rtValue& arg1) {
    EXPECT_NE(impl, nullptr);
    return impl->send(messageName, arg1);
}

rtError rtObjectRef::send(const char* messageName, rtObjectRef& base) {
    EXPECT_NE(impl, nullptr);
    return impl->send(messageName, base);
}

rtObjectRef::~rtObjectRef() {}

rtValueImpl* rtValue::impl = nullptr;

rtValue& rtValue::getInstance() {
    static rtValue instance;
    return instance;
}

rtValue::rtValue() = default;

rtValue::rtValue(bool v) : mValue({ .boolValue = v }) {}

rtValue::rtValue(const char* v) : mValue({ .stringValue = v }) {}

rtValue::rtValue(const rtString& v) : mValue({ .stringValue = v.cString() }) {}

rtValue::rtValue(rtIObject* v) {
    if (v) {
        delete v;
        v = nullptr;
    }
}

rtValue::rtValue(const rtObjectRef& v) {}

rtValue::rtValue(const rtValue& other) {
    mValue = other.mValue;
}

rtValue::~rtValue() {}

rtValue& rtValue::operator=(bool v) {
    mValue.boolValue = v;
    return *this;
}

rtValue& rtValue::operator=(const char* v) {
    mValue.stringValue = v;
    return *this;
}

rtValue& rtValue::operator=(const rtString& v) {
    mValue.stringValue = v.cString();
    return *this;
}

rtValue& rtValue::operator=(const rtIObject* v) {
    delete v;
    v = nullptr;
    return *this;
}

rtValue& rtValue::operator=(const rtObjectRef& v) {
    return *this;
}

rtValue& rtValue::operator=(const rtValue& other) {
    if (this != &other) {
        mValue = other.mValue;
    }
    return *this;
}

void rtValue::setImpl(rtValueImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

rtObjectRef rtValue::toObject() const {
    rtObjectRef v;
    return v;
}

void rtValue::setString(const char* v) {
    mValue.stringValue = v;
}

void rtValue::setString(const rtString& v) {
    mValue.stringValue = v.cString();
}


rtArrayObjectImpl* rtArrayObject::impl = nullptr;

rtArrayObject& rtArrayObject::getInstance() {
    static rtArrayObject instance;
    return instance;
}

void rtArrayObject::pushBack(const char* v) {
    EXPECT_NE(impl, nullptr);
    impl->pushBack(v);
}

void rtArrayObject::pushBack(rtValue v) {
    EXPECT_NE(impl, nullptr);
    impl->pushBack(v);
}

void rtArrayObject::setImpl(rtArrayObjectImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

floatingRtFunctionsImpl* floatingRtFunctions::impl = nullptr;
floatingRtFunctions& floatingRtFunctions::getInstance()
{
    static floatingRtFunctions instance;
    return instance;
}
void floatingRtFunctions::setImpl(floatingRtFunctionsImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

rtError rtRemoteProcessSingleItem() {
    return floatingRtFunctions::getInstance().impl->rtRemoteProcessSingleItem();
}

rtError rtRemoteLocateObject(rtRemoteEnvironment* env, const char* str, rtObjectRef& obj, int x, remoteDisconnectCallback back, void* cbdata) {
    return floatingRtFunctions::getInstance().impl->rtRemoteLocateObject(env, str, obj, x, back, cbdata);
}

rtRemoteEnvironment* rtEnvironmentGetGlobal() {
    return floatingRtFunctions::getInstance().impl->rtEnvironmentGetGlobal();
}

rtError rtRemoteInit(rtRemoteEnvironment* env) {
    return floatingRtFunctions::getInstance().impl->rtRemoteInit(env);
}

rtError rtRemoteShutdown(rtRemoteEnvironment* env) {
    return floatingRtFunctions::getInstance().impl->rtRemoteShutdown(env);
}

char* rtStrError(rtError err) {
    return floatingRtFunctions::getInstance().impl->rtStrError(err);
}

namespace edid_parser{

    edidParserImpl* edidParserImpl::impl = nullptr;
    void edidParserImpl::setImpl(edidParserImpl* newImpl) {
        // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
        EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
        impl = newImpl;
    }
    edid_status_e EDID_Parse(unsigned char* bytes, size_t count, edid_data_t* data_ptr){
        EXPECT_NE(edidParserImpl::impl, nullptr);
        return edidParserImpl::impl->EDID_Parse(bytes, count, data_ptr);
    }
    edid_status_e EDID_Verify(unsigned char* bytes, size_t count){
        EXPECT_NE(edidParserImpl::impl, nullptr);
        return edidParserImpl::impl->EDID_Verify(bytes, count);
    }
}

drmImpl* drmImpl::impl = nullptr;
void drmImpl::setImpl(drmImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id){
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeGetEncoder(fd, encoder_id);
}
void drmModeFreeEncoder(drmModeEncoderPtr* encoder){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreeEncoder(encoder);
}
drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connectorId){
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeGetConnector(fd, connectorId);
}
drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId){
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeGetCrtc(fd, crtcId);
}
drmModeResPtr drmModeGetResources(int fd){
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeGetResources(fd);
}
void drmModeFreeConnector( drmModeConnectorPtr ptr ){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreeConnector(ptr);
}
void drmModeFreeCrtc( drmModeCrtcPtr ptr ){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreeCrtc(ptr);
}
void drmModeFreeResources( drmModeResPtr ptr ){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreeResources(ptr);
}
drmModePropertyPtr drmModeGetProperty(int fd, uint32_t propertyId){
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeGetProperty(fd, propertyId);
}
void drmModeFreeProperty(drmModePropertyPtr ptr){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreeProperty(ptr);
}
drmModePlaneResPtr drmModeGetPlaneResources(int fd){
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeGetPlaneResources(fd);
}
drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id){
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeGetPlane(fd, plane_id);
}
drmModeObjectPropertiesPtr drmModeObjectGetProperties(int fd,uint32_t object_id, uint32_t object_type){
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeObjectGetProperties(fd, object_id, object_type);
}
void drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreeObjectProperties(ptr);
}
void drmModeFreePlane( drmModePlanePtr ptr ){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreePlane(ptr);
}
void drmModeFreePlaneResources(drmModePlaneResPtr ptr){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreePlaneResources(ptr);
}

void drmModeFreeFB(drmModeFBPtr ptr) {
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreeFB(ptr);
}

drmModeFBPtr drmModeGetFB(int fd, uint32_t bufferId) {
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmModeGetFB(fd, bufferId);
}

void drmModeFreeEncoder( drmModeEncoderPtr ptr ){
    EXPECT_NE(drmImpl::impl, nullptr);
    drmImpl::impl->drmModeFreeEncoder(ptr);
}

int drmSetClientCap(int fd, uint64_t capability, uint64_t value) {
    EXPECT_NE(drmImpl::impl, nullptr);
    return drmImpl::impl->drmSetClientCap(fd, capability, value);
}
