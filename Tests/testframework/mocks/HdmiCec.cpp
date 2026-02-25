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

#include "HdmiCec.h"
#include <gmock/gmock.h>

MessageDecoderImpl* MessageDecoder::impl = nullptr;

MessageDecoder::MessageDecoder(MessageProcessor& proc)
    : processor(proc) {}

void MessageDecoder::setImpl(MessageDecoderImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

//void MessageDecoder::decode(const CECFrame& in) {
//    EXPECT_NE(impl, nullptr);
//    return impl->decode(in);
//}

void MessageDecoder::decode(const CECFrame& in) {
	if (in.length() < 2) return; // At least header + opcode

    Header header;
    header.from = (in[0] >> 4) & 0x0F;
    header.to = in[0] & 0x0F;
    uint8_t opcode = in[1];
    switch (opcode) {
        case 0x82: // Active Source
            processor.process(ActiveSource(in), header);
            break;
        case 0x9D: // Inactive Source
            processor.process(InActiveSource(in), header);
            break;
        case 0x04: // Image View On
            processor.process(ImageViewOn(), header);
            break;
        case 0x0D: // Text View On
            processor.process(TextViewOn(), header);
            break;
        case 0x85: // Request Active Source
            processor.process(RequestActiveSource(), header);
            break;
        case 0x36: // Standby
            processor.process(Standby(), header);
            break;
        case 0x9F: // Get CEC Version
            processor.process(GetCECVersion(), header);
            break;
        case 0x9E: // CEC Version
            processor.process(CECVersion(Version(in)), header);
            break;
        case 0x32: // Set Menu Language
            processor.process(SetMenuLanguage(in), header);
            break;
        case 0x91: // Get Menu Language
            processor.process(GetMenuLanguage(), header);
            break;
        case 0x46: // Give OSD Name
            processor.process(GiveOSDName(), header);
            break;
        case 0x83: // Give Physical Address
            processor.process(GivePhysicalAddress(), header);
            break;
        case 0x8C: // Give Device Vendor ID
            processor.process(GiveDeviceVendorID(), header);
            break;
        case 0x64: // Set OSD String
            {
                OSDString osdStr;
                SetOSDString msg;
                msg.osdString = osdStr;
                processor.process(msg, header);
            }
            break;
        case 0x47: // Set OSD Name
            {
                processor.process(SetOSDName(in), header);
            }
            break;
        case 0x80: // Routing Change
            {
                PhysicalAddress from, to;
                processor.process(RoutingChange(from, to), header);
            }
            break;
        case 0x81: // Routing Information
            processor.process(RoutingInformation(in), header);
            break;
        case 0x86: // Set Stream Path
            processor.process(SetStreamPath(in), header);
            break;
        case 0x84: // Report Physical Address
            processor.process(ReportPhysicalAddress(in), header);
            break;
        case 0x87: // Device Vendor ID
            processor.process(DeviceVendorID(in), header);
            break;
        case 0x44: // User Control Pressed
            processor.process(UserControlPressed(in), header);
            break;
        case 0x45: // User Control Released
            processor.process(UserControlReleased(), header);
            break;
        case 0x8F: // Give Device Power Status
            processor.process(GiveDevicePowerStatus(), header);
            break;
        case 0x90: // Report Power Status
            processor.process(ReportPowerStatus(in), header);
            break;
        case 0x00: // Feature Abort
            processor.process(FeatureAbort(in), header);
            break;
        case 0xFF: // Abort
            processor.process(Abort(), header);
            break;
        case 0x13: // Polling Message (not a real opcode, but for completeness)
            processor.process(Polling(), header);
            break;
        case 0xC0: // Initiate ARC
            processor.process(InitiateArc(), header);
            break;
        case 0xC1: // Report ARC Initiated
            processor.process(ReportArcInitiation(), header);
            break;
        case 0xC2: // Report ARC Terminated
            processor.process(ReportArcTermination(), header);
            break;
        case 0xC3: // Request ARC Initiation
            processor.process(RequestArcInitiation(), header);
            break;
        case 0xC4: // Request ARC Termination
            processor.process(RequestArcTermination(), header);
            break;
        case 0xC5: // Terminate ARC
            processor.process(TerminateArc(), header);
            break;
        case 0xA3: // Report Short Audio Descriptor
            processor.process(ReportShortAudioDescriptor(), header);
            break;
        case 0xA4: // Request Short Audio Descriptor
            processor.process(RequestShortAudioDescriptor({}, {}, 1), header);
            break;
        case 0x7A: // Report Audio Status
            processor.process(ReportAudioStatus(in), header);
            break;
        case 0xA8: // Report Current Latency
            processor.process(ReportCurrentLatency(PhysicalAddress(), 0, 0, 0), header);
            break;
        case 0xA5: // Give Features
            processor.process(GiveFeatures(), header);
            break;
        case 0xA6: // Report Features
            processor.process(ReportFeatures(Version(0), AllDeviceTypes(0), {}, {}), header);
            break;
        case 0x70: // System Audio Mode Request
            processor.process(SystemAudioModeRequest(in), header);
            break;
        case 0x72: // Set System Audio Mode
            processor.process(SetSystemAudioMode(in), header);
            break;
        case 0x7D: // Give System Audio Mode Status
            processor.process(GiveAudioStatus(), header);
            break;
        case 0x7E: // System Audio Mode Status
            processor.process(SetSystemAudioMode(), header);
            break;
        case 0x71: // Give Audio Status
            processor.process(GiveAudioStatus(), header);
            break;
        case 0xA7: // Request Current Latency
            processor.process(RequestCurrentLatency(), header);
            break;
        default:
            // Optionally log or ignore unknown opcodes
            break;
    }
}

ConnectionImpl* Connection::impl = nullptr;

Connection::Connection(const LogicalAddress& source, bool opened, const std::string& name) {}

Connection& Connection::getInstance() {
    static Connection instance;
    return instance;
}

void Connection::setImpl(ConnectionImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

void Connection::open() {
    EXPECT_NE(impl, nullptr);
    return impl->open();
}

void Connection::close() {
    EXPECT_NE(impl, nullptr);
    return impl->close();
}
void Connection::addFrameListener(FrameListener* listener) {
    EXPECT_NE(impl, nullptr);
    return impl->addFrameListener(listener);
}

void Connection::ping(const LogicalAddress& from, const LogicalAddress& to, const Throw_e& doThrow) {
    EXPECT_NE(impl, nullptr);
    return impl->ping(from, to, doThrow);
}

void Connection::sendToAsync(const LogicalAddress& to, const CECFrame& frame) {
    EXPECT_NE(impl, nullptr);
    return impl->sendToAsync(to, frame);
}

void Connection::sendTo(const LogicalAddress& to, const CECFrame& frame) {
    EXPECT_NE(impl, nullptr);
    return impl->sendTo(to, frame);
}

void Connection::sendTo(const LogicalAddress& to, const CECFrame& frame, int timeout) {
    EXPECT_NE(impl, nullptr);
    return impl->sendTo(to, frame, timeout);
}

void Connection::poll(const LogicalAddress& from, const Throw_e& doThrow) {
    EXPECT_NE(impl, nullptr);
    return impl->poll(from, doThrow);
}

void Connection::sendAsync(const CECFrame &frame){
     EXPECT_NE(impl, nullptr);
     return impl->sendAsync(frame);
}

void Connection::setSource(LogicalAddress& from) {
}


LibCCECImpl* LibCCEC::impl = nullptr;

LibCCEC& LibCCEC::getInstance() {
    static LibCCEC instance;
    return instance;
}

void LibCCEC::setImpl(LibCCECImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}
void LibCCEC::init(const char* name) {
    EXPECT_NE(impl, nullptr);
    impl->init(name);
}

void LibCCEC::init() {
    EXPECT_NE(impl, nullptr);
    impl->init();
}

void LibCCEC::term() {
}

void LibCCEC::getPhysicalAddress(uint32_t* physicalAddress) {
    EXPECT_NE(impl, nullptr);
    impl->getPhysicalAddress(physicalAddress);
}

int LibCCEC::addLogicalAddress(const LogicalAddress& source) {
    EXPECT_NE(impl, nullptr);
    return impl->addLogicalAddress(source);
}

int LibCCEC::getLogicalAddress(int devType) {
    EXPECT_NE(impl, nullptr);
    return impl->getLogicalAddress(devType);
}

MessageEncoderImpl* MessageEncoder::impl = nullptr;

void MessageEncoder::setImpl(MessageEncoderImpl* newImpl) {
        // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

CECFrame& MessageEncoder::encode(const UserControlPressed m) {
    EXPECT_NE(impl, nullptr);
    return impl->encode(m);
}

CECFrame& MessageEncoder::encode(const DataBlock m) {
    EXPECT_NE(impl, nullptr);
    return impl->encode(m);
}

ShortAudioDescriptor::ShortAudioDescriptor() : CECBytes((uint8_t)0), impl(nullptr) {}

AbortReason::AbortReason() : CECBytes((uint8_t)0), impl(nullptr) {}
