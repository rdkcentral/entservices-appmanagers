/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
**/

#pragma once
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <sstream>
#include <string>
#include <map>
#include <exception>
#include "devicesettings.h"

typedef uint32_t Op_t;

enum {
    ACTIVE_SOURCE 					= 0x82,
	IMAGE_VIEW_ON 					= 0x04,
	TEXT_VIEW_ON 					= 0x0D,
	INACTIVE_SOURCE 				= 0x9D,
	REQUEST_ACTIVE_SOURCE 			= 0x85,
	ROUTING_CHANGE                  = 0x80,
	ROUTING_INFORMATION             = 0x81,
	SET_STREAM_PATH                 = 0x86,
	STANDBY 						= 0x36,
	RECORD_OFF                      = 0X0B,
	RECORD_ON                       = 0X09,
	RECORD_STATUS                   = 0X0A,
	RECORD_TV_SCREEN                = 0X0F,
	CLEAR_ANALOGUE_TIMER            = 0X33,
	CLEAR_DIGITAL_TIMER             = 0X99,
	CLEAR_EXTERNAL_TIMER            = 0XA1,
	SET_ANALOG_TIMER                = 0X34,
	SET_DIGITAL_TIMER               = 0X97,
	SET_EXTERNAL_TIMER              = 0XA2,
	SET_TIMER_PROGRAM_TITLE         = 0X67,
	TIMER_CLEARED_STATUS            = 0X43,
	TIMER_STATUS                    = 0X35,
	GET_CEC_VERSION 				= 0x9F,
	CEC_VERSION 					= 0x9E,
	GIVE_PHYSICAL_ADDRESS 			= 0x83,
	GET_MENU_LANGUAGE               = 0X91,
	REPORT_PHYSICAL_ADDRESS 		= 0x84,
	SET_MENU_LANGUAGE               = 0X32,
	DECK_CONTROL                    = 0X42,
	DECK_STATUS                     = 0X1B,
	GIVE_DECK_STATUS                = 0X1A,
	PLAY                            = 0X41,
	GIVE_TUNER_DEVICE_STATUS        = 0X08,
	SELECT_ANALOGUE_SERVICE         = 0X92,
	SELECT_DIGITAL_SERVICE          = 0X93,
	TUNER_DEVICE_STATUS             = 0X07,
	TUNER_STEP_DECREMENT            = 0X06,
	TUNER_STEP_INCREMENT            = 0X05,
	DEVICE_VENDOR_ID 				= 0x87,
	GIVE_DEVICE_VENDOR_ID 			= 0x8C,
	VENDOR_COMMAND                  = 0X89,
	VENDOR_COMMAND_WITH_ID          = 0XA0,
	VENDOR_REMOTE_BUTTON_DOWN       = 0X8A,
	VENDOR_REMOTE_BUTTON_UP         = 0X8B,
	SET_OSD_STRING 					= 0x64,
	GIVE_OSD_NAME 					= 0x46,
	SET_OSD_NAME 					= 0x47,
	MENU_REQUEST                    = 0X8D,
	MENU_STATUS                     = 0X8E,
	USER_CONTROL_PRESSED            = 0X44,
	USER_CONTROL_RELEASED           = 0X45,
	GIVE_DEVICE_POWER_STATUS 		= 0x8F,
	REPORT_POWER_STATUS 			= 0x90,
	FEATURE_ABORT 					= 0x00,
	ABORT 							= 0xFF,
	GIVE_AUDIO_STATUS               = 0X71,
	GIVE_SYSTEM_AUDIO_MODE_STATUS   = 0X7D,
	REPORT_AUDIO_STATUS             = 0X7A,
	REPORT_SHORT_AUDIO_DESCRIPTOR   = 0XA3,
	REQUEST_SHORT_AUDIO_DESCRIPTOR  = 0XA4,
	SET_SYSTEM_AUDIO_MODE           = 0X72,
	SYSTEM_AUDIO_MODE_REQUEST       = 0X70,
	SYSTEM_AUDIO_MODE_STATUS        = 0X7E,
	SET_AUDIO_RATE                  = 0X9A,
	INITIATE_ARC                    = 0XC0,
	REPORT_ARC_INITIATED            = 0XC1,
	REPORT_ARC_TERMINATED           = 0XC2,
	REQUEST_ARC_INITIATION          = 0XC3,
	REQUEST_ARC_TERMINATION         = 0XC4,
	TERMINATE_ARC                   = 0XC5,
	CDC_MESSAGE                     = 0XF8,
	POLLING 						= 0x200, // Special Code for Polling Msg.

	GIVE_FEATURES                   = 0xA5,
	REPORT_FEATURES                 = 0xA6,
	REQUEST_CURRENT_LATENCY		= 0xA7,
	REPORT_CURRENT_LATENCY		= 0xA8,
	UNKNOWN                         = 0xFFFF
};

typedef struct _dsHdmiInGetNumberOfInputsParam_t {
    dsError_t result;
    uint8_t numHdmiInputs;
} dsHdmiInGetNumberOfInputsParam_t;

typedef struct _dsHdmiInStatus_t {
    bool isPresented;
    bool isPortConnected[dsHDMI_IN_PORT_MAX];
    dsHdmiInPort_t activePort;
} dsHdmiInStatus_t;

typedef struct _dsHdmiInGetStatusParam_t {
    dsError_t result;
    dsHdmiInStatus_t status;
} dsHdmiInGetStatusParam_t;

typedef struct _dsGetHDMIARCPortIdParam_t {
    dsError_t result;
    int portId;
} dsGetHDMIARCPortIdParam_t;

typedef struct _Throw_e {
} Throw_e;

class Exception : public std::exception {
public:
    virtual const char* what() const throw()
    {
        return "Base Exception..";
    }
};

class CECNoAckException : public Exception {
public:
    virtual const char* what() const throw()
    {
        return "Ack not received..";
    }
};

class InvalidStateException : public Exception {
public:
    virtual const char* what() const throw()
    {
        return "Invalid state..";
    }
};

class CECFrame {
public:
    enum {
        MAX_LENGTH = 128,
    };

    CECFrame(const uint8_t* buf = NULL, uint16_t len = 0) : len_(0) {
        if (buf && len <= MAX_LENGTH) {
            memcpy(buf_, buf, len);
            len_ = len;
        }
    }

    static CECFrame& getInstance() {
        static CECFrame instance;
        return instance;
    }

    void push_back(uint8_t byte) {
        if (len_ < MAX_LENGTH) {
            buf_[len_++] = byte;
        }
    }

    uint8_t operator[](size_t index) const {
        return (index < len_) ? buf_[index] : 0;
    }

    uint8_t opcode() const {
        return (len_ > 1) ? buf_[1] : 0;
    }

    void getBuffer(const uint8_t** buf, size_t* len) const {
        *buf = buf_;
        *len = len_;
    }

    size_t length() const {
        return len_;
    }

private:
    uint8_t buf_[MAX_LENGTH];
    size_t len_;
};

class CECBytes {
protected:
    std::vector<uint8_t> str;
    CECBytes(const uint8_t val){
                str.push_back(val);
        }
    CECBytes(const uint8_t* buf, size_t len) {
        if(buf && len){
            for(size_t i =0; i < len; i++){
                str.push_back(buf[i]);
            }
        }
    }
    CECBytes(const CECFrame& frame, size_t startPos, size_t len) {
        const uint8_t* buf = nullptr;
        size_t frameLen = 0;
        frame.getBuffer(&buf, &frameLen);
        if (buf && startPos < frameLen) {
            size_t copyLen = (startPos + len > frameLen) ? (frameLen - startPos) : len;
            str.insert(str.end(), buf + startPos, buf + startPos + copyLen);
        }
    }

public:
    CECFrame& serialize(CECFrame& frame) const
    {
        return frame;
    }

    bool operator==(const CECBytes& in) const
    {
        return this->str == in.str;
    };

    virtual const std::string name(void) const
    {
        return "Operand";
    }

    virtual const std::string toString(void) const
    {
        std::stringstream stream;
        for(size_t i =0; i < str.size(); i++) {
            stream << std :: hex << (int)str[i];
        }
        return stream.str();
    };

    ~CECBytes(void) {}
};

class OSDString : public CECBytes {
public:
    OSDString() : CECBytes((uint8_t)0) {}
};

class OSDName : public CECBytes {
public:
    OSDName(const char* str1)
        : CECBytes((const uint8_t*)str1, strlen(str1)){
    }

    const std::string toString(void) const {
        return std::string(str.begin(), str.end());
    }
};

class AbortReasonImpl {
public:
    virtual int toInt(void) const = 0;
};

class AbortReason : public CECBytes {
public:
    enum {
        UNRECOGNIZED_OPCODE,
    };

    AbortReason(int reason)
        : CECBytes((uint8_t)reason){
    }

    AbortReason();

    AbortReason* impl;
    static AbortReason& getInstance()
    {
        static AbortReason instance;
        return instance;
    }

    int toInt(void) const
    {
        // Use impl if available, fallback to direct parsing for L2 tests
        if (impl && impl != this) {
            return impl->toInt();
        }
        // L2 fallback: access the protected str member directly
        return (!str.empty()) ? str[0] : 0;
    }
};

class DeviceType : public CECBytes {
public:
    enum {
        MAX_LEN = 1,
    };

    enum {
        TV = 0x0,
                RECORDING_DEVICE,
                RESERVED,
                TUNER,
                PLAYBACK_DEVICE,
                AUDIO_SYSTEM,
                PURE_CEC_SWITCH,
                VIDEO_PROCESSOR,
    };

    DeviceType(const CECFrame& frame, size_t startPos)
        : CECBytes(frame, startPos, MAX_LEN)
    {
    }
    DeviceType(int type)
        : CECBytes((uint8_t)type)
    {
    }

    const std::string toString(void) const {
            static const char *names_[] = {
                "TV",
                "Recording Device",
                "Reserved",
                "Tuner",
                "Playback Device",
                "Audio System",
                "Pure CEC Switch",
                "Video Processor",
            };
            if((str[0] <= VIDEO_PROCESSOR)){
                return names_[str[0]];
            }
            else{
                return "Unknown";
            }
        }
};

class Language : public CECBytes {
public:
    enum {
        MAX_LEN = 3,
    };

    Language(const char* str1)
        : CECBytes((const uint8_t*)str1, MAX_LEN){};
};


class VendorID : public CECBytes{
public:
    enum {
        MAX_LEN = 3,
    };

    VendorID(uint8_t byte0, uint8_t byte1, uint8_t byte2): CECBytes (NULL,0){
        uint8_t bytes[MAX_LEN];
        bytes[0] = byte0;
        bytes[1] = byte1;
        bytes[2] = byte2;
        str.insert(str.begin(), bytes, bytes + MAX_LEN);
    };

    // Fix the frame constructor
    VendorID(const uint8_t* buf, size_t len): CECBytes(buf, len >= 3 ? 3 : len) {};

    VendorID(): CECBytes(NULL,0){
        // Default vendor ID
        str.push_back(0x00);
        str.push_back(0x19);
        str.push_back(0xFB);
    };
};

class PhysicalAddress : public CECBytes {
public:
    enum {
        MAX_LEN = 2,
    };
    PhysicalAddress(const CECFrame& frame, size_t startPos)
        : CECBytes(frame, startPos, MAX_LEN){};
    PhysicalAddress(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3)
        : CECBytes(NULL, 0) {
        str.push_back(byte0);
        str.push_back(byte1);
        str.push_back(byte2);
        str.push_back(byte3);
    }
    explicit PhysicalAddress(uint16_t addr)
        : CECBytes(NULL, 0) {
        str.push_back((addr >> 8) & 0xFF);
        str.push_back(addr & 0xFF);
    }
    PhysicalAddress(std::string& addr): CECBytes(NULL, 0) {
    }
    PhysicalAddress(): CECBytes(NULL, 0) {};

    static PhysicalAddress& getInstance()
    {
        static PhysicalAddress instance;
        return instance;
    }

    uint8_t getByteValue(int index) const
    {
        return (index >= 0 && static_cast<size_t>(index) < str.size()) ? str[index] : 0;
    }
};

class LogicalAddressImpl {
public:
    virtual int getType() const = 0;
};

class LogicalAddress : public CECBytes {
public:
    enum {
        TV = 0,
        AUDIO_SYSTEM = 5,
        SPECIFIC_USE = 14,
        UNREGISTERED = 15,
        BROADCAST = UNREGISTERED,
    };

    LogicalAddress* impl;
    static LogicalAddress& getInstance()
    {
        static LogicalAddress instance;
        return instance;
    }

    LogicalAddress(int addr = UNREGISTERED)
        : CECBytes((uint8_t)addr){};

        int toInt() const
    {
        return str[0];
    }

    int getType(void) const {
                static int _type[] = {
            DeviceType::TV,
            DeviceType::RECORDING_DEVICE,
            DeviceType::RECORDING_DEVICE,
            DeviceType::TUNER,
            DeviceType::PLAYBACK_DEVICE,
            DeviceType::AUDIO_SYSTEM,
            DeviceType::TUNER,
            DeviceType::TUNER,
            DeviceType::PLAYBACK_DEVICE,
            DeviceType::RECORDING_DEVICE,
            DeviceType::TUNER,
            DeviceType::PLAYBACK_DEVICE,
            DeviceType::RESERVED,
            DeviceType::RESERVED,
            DeviceType::RESERVED,
            DeviceType::RESERVED,
        };
        return _type[str[0]];
        }
};

class Version : public CECBytes {
public:
    enum {
        V_1_4 = 0x05,
        V_2_0 = 0x06
    };

    Version(int version)
        : CECBytes((uint8_t)version){};

    // Add frame constructor
    Version(const CECFrame& frame, int startPos = 2)
        : CECBytes(frame.length() > (size_t)startPos ? frame[startPos] : (uint8_t)V_1_4) {}
};

class PowerStatusImpl {
public:
    virtual int toInt(void) const = 0;
};

class PowerStatus : public CECBytes {
public:
    enum {
        ON = 0,
        STANDBY = 0x01,
        TRANSITIONING_STANDBY_TO_ON = 0x02,
        TRANSITIONING_ON_TO_STANDBY = 0x03,
        POWER_STATUS_NOT_KNOWN = 0x4,
        POWER_STATUS_FEATURE_ABORT = 0x05,
    };

    PowerStatus(int status = POWER_STATUS_NOT_KNOWN)
        : CECBytes((uint8_t)status) {};

    PowerStatus& operator=(int status) {
        str.clear();
        str.push_back((uint8_t)status);
        return *this;
    }

    PowerStatus* impl;

    static PowerStatus& getInstance() {
        static PowerStatus instance;
        return instance;
    }

    int toInt(void) const {
        return str[0];
    }

    const std::string toString(void) const {
        switch(str[0]) {
            case ON: return "ON";
            case STANDBY: return "STANDBY";
            case TRANSITIONING_STANDBY_TO_ON: return "TRANSITIONING_STANDBY_TO_ON";
            case TRANSITIONING_ON_TO_STANDBY: return "TRANSITIONING_ON_TO_STANDBY";
            case POWER_STATUS_NOT_KNOWN: return "NOT_KNOWN";
            default: return "UNKNOWN";
        }
    }
};

class RequestAudioFormat : public CECBytes {
};

class ShortAudioDescriptorImpl {
public:
    virtual uint32_t getAudiodescriptor(void) const = 0;
};

class ShortAudioDescriptor : public CECBytes {
public:
    ShortAudioDescriptor();

    ShortAudioDescriptor* impl;
    static ShortAudioDescriptor& getInstance() {
        static ShortAudioDescriptor instance;
        return instance;
    }

    uint32_t getAudiodescriptor(void) const {
        return impl->getAudiodescriptor();
    }
};

class SystemAudioStatusImpl {
public:
    virtual int toInt(void) const = 0;
};

class SystemAudioStatus : public CECBytes {
public:
    SystemAudioStatus* impl;

    SystemAudioStatus() : CECBytes((uint8_t)0), impl(nullptr) {}
    SystemAudioStatus(uint8_t statusByte) : CECBytes(statusByte), impl(nullptr) {}

    static SystemAudioStatus& getInstance() {
        static SystemAudioStatus instance;
        return instance;
    }

    int toInt(void) const {
        // Use impl if available, fallback to direct parsing for L2 tests
        if (impl && impl != this) {
            return impl->toInt();
        }
        // L2 fallback: access the protected str member directly
        return (!str.empty()) ? str[0] : 0;
    }
};

class AudioStatusImpl {
public:
    virtual int getAudioMuteStatus(void) const = 0;
    virtual int getAudioVolume(void) const = 0;
};

class AudioStatus : public CECBytes {
public:
    AudioStatus* impl;

    AudioStatus() : CECBytes((uint8_t)0), impl(nullptr) {}
    AudioStatus(uint8_t statusByte) : CECBytes(statusByte), impl(nullptr) {}

    static AudioStatus& getInstance() {
        static AudioStatus instance;
        return instance;
    }

    int getAudioMuteStatus(void) const {
        // Use impl if available, fallback to direct parsing for L2 tests
        if (impl && impl != this) {
            return impl->getAudioMuteStatus();
        }
        // L2 fallback: access the protected str member directly
        return (!str.empty()) ? ((str[0] & 0x80) ? 1 : 0) : 0;
    }

    int getAudioVolume(void) const {
        // Use impl if available, fallback to direct parsing for L2 tests
        if (impl && impl != this) {
            return impl->getAudioVolume();
        }
        // L2 fallback: access the protected str member directly
        return (!str.empty()) ? (str[0] & 0x7F) : 0;
    }
};

class UICommand : public CECBytes {
public:
    enum {
        UI_COMMAND_VOLUME_UP = 0x41,
        UI_COMMAND_VOLUME_DOWN = 0x42,
        UI_COMMAND_MUTE = 0x43,
        UI_COMMAND_MUTE_FUNCTION = 0x65,
        UI_COMMAND_RESTORE_FUNCTION = 0x66,
        UI_COMMAND_POWER_OFF_FUNCTION = 0x6C,
        UI_COMMAND_POWER_ON_FUNCTION = 0x6D,
        UI_COMMAND_UP = 0x01,
        UI_COMMAND_DOWN = 0x02,
        UI_COMMAND_LEFT = 0x03,
        UI_COMMAND_RIGHT = 0x04,
        UI_COMMAND_SELECT = 0x00,
        UI_COMMAND_HOME = 0x09,
        UI_COMMAND_BACK = 0x0D,
        UI_COMMAND_NUM_0 = 0x20,
        UI_COMMAND_NUM_1 = 0x21,
        UI_COMMAND_NUM_2 = 0x22,
        UI_COMMAND_NUM_3 = 0x23,
        UI_COMMAND_NUM_4 = 0x24,
        UI_COMMAND_NUM_5 = 0x25,
        UI_COMMAND_NUM_6 = 0x26,
        UI_COMMAND_NUM_7 = 0x27,
        UI_COMMAND_NUM_8 = 0x28,
        UI_COMMAND_NUM_9 = 0x29,
    };

    UICommand(int command)
        : CECBytes((uint8_t)command){};

    int toInt() const
    {
        return str[0];
    }
};

class AllDeviceTypes : public CECBytes
{
public:
    enum {
        MAX_LEN = 1,
    };

    enum {
        CEC_SWITCH = 2,
        AUDIO_SYSTEM,
        PLAYBACK_DEVICE,
        TUNER,
        RECORDING_DEVICE,
        TV,
    };

    AllDeviceTypes(uint8_t types) : CECBytes((uint8_t)types) { };
};

class RcProfile : public CECBytes
{
public:
    enum {
        MAX_LEN = 4,
    };

    enum {
        MEDIA_CONTEXT_MENU,
        MEDIA_TOP_MENU,
        CONTENTS_MENU,
        DEVICE_SETUP_MENU,
        DEVICE_ROOT_MENU,
        RC_PROFILE_BIT = 6,
    };

    RcProfile(uint8_t info) : CECBytes((uint8_t)info) { };

};

class DeviceFeatures : public CECBytes
{
public:
    enum {
        MAX_LEN = 4,
    };

    enum {
        RESERVED,
        ARC_RX_SUPPORT,
        SINK_ARC_TX_SUPPORT,
        SET_AUDIO_RATE_SUPPORT,
        CONTROLLED_BY_DECK,
        SET_OSD_STRING_SUPPORT,
        RECORD_TV_SCREEN_SUPPORT,
    };

    DeviceFeatures(uint8_t info) : CECBytes((uint8_t)info) { };
};

class DataBlock {
};

inline const char* GetOpName(Op_t op) {
    switch(op) {
        case ACTIVE_SOURCE: return "ACTIVE_SOURCE";
        case IMAGE_VIEW_ON: return "IMAGE_VIEW_ON";
        case TEXT_VIEW_ON: return "TEXT_VIEW_ON";
        case INACTIVE_SOURCE: return "INACTIVE_SOURCE";
        case REQUEST_ACTIVE_SOURCE: return "REQUEST_ACTIVE_SOURCE";
        case ROUTING_CHANGE: return "ROUTING_CHANGE";
        case ROUTING_INFORMATION: return "ROUTING_INFORMATION";
        case SET_STREAM_PATH: return "SET_STREAM_PATH";
        case STANDBY: return "STANDBY";
        case GET_CEC_VERSION: return "GET_CEC_VERSION";
        case CEC_VERSION: return "CEC_VERSION";
        case GIVE_PHYSICAL_ADDRESS: return "GIVE_PHYSICAL_ADDRESS";
        case REPORT_PHYSICAL_ADDRESS: return "REPORT_PHYSICAL_ADDRESS";
        case DEVICE_VENDOR_ID: return "DEVICE_VENDOR_ID";
        case GIVE_DEVICE_VENDOR_ID: return "GIVE_DEVICE_VENDOR_ID";
        case SET_OSD_STRING: return "SET_OSD_STRING";
        case GIVE_OSD_NAME: return "GIVE_OSD_NAME";
        case SET_OSD_NAME: return "SET_OSD_NAME";
        case USER_CONTROL_PRESSED: return "USER_CONTROL_PRESSED";
        case USER_CONTROL_RELEASED: return "USER_CONTROL_RELEASED";
        case GIVE_DEVICE_POWER_STATUS: return "GIVE_DEVICE_POWER_STATUS";
        case REPORT_POWER_STATUS: return "REPORT_POWER_STATUS";
        case FEATURE_ABORT: return "FEATURE_ABORT";
        case ABORT: return "ABORT";
        case REPORT_AUDIO_STATUS: return "REPORT_AUDIO_STATUS";
        case SET_SYSTEM_AUDIO_MODE: return "SET_SYSTEM_AUDIO_MODE";
        case SYSTEM_AUDIO_MODE_REQUEST: return "SYSTEM_AUDIO_MODE_REQUEST";
        case GIVE_AUDIO_STATUS: return "GIVE_AUDIO_STATUS";
        case INITIATE_ARC: return "INITIATE_ARC";
        case TERMINATE_ARC: return "TERMINATE_ARC";
        case REQUEST_ARC_INITIATION: return "REQUEST_ARC_INITIATION";
        case REQUEST_ARC_TERMINATION: return "REQUEST_ARC_TERMINATION";
        case REPORT_ARC_INITIATED: return "REPORT_ARC_INITIATED";
        case REPORT_ARC_TERMINATED: return "REPORT_ARC_TERMINATED";
        case REQUEST_SHORT_AUDIO_DESCRIPTOR: return "REQUEST_SHORT_AUDIO_DESCRIPTOR";
        case REPORT_SHORT_AUDIO_DESCRIPTOR: return "REPORT_SHORT_AUDIO_DESCRIPTOR";
        case GIVE_FEATURES: return "GIVE_FEATURES";
        case REPORT_FEATURES: return "REPORT_FEATURES";
        case REQUEST_CURRENT_LATENCY: return "REQUEST_CURRENT_LATENCY";
        case REPORT_CURRENT_LATENCY: return "REPORT_CURRENT_LATENCY";
        case SET_MENU_LANGUAGE: return "SET_MENU_LANGUAGE";
        case GET_MENU_LANGUAGE: return "GET_MENU_LANGUAGE";
        default: return "UNKNOWN";
    }
}

class ActiveSource;
class InActiveSource;
class ImageViewOn;
class TextViewOn;
class RequestActiveSource;
class Standby;
class GetCECVersion;
class CECVersion;
class SetMenuLanguage;
class GiveOSDName;
class GivePhysicalAddress;
class GiveDeviceVendorID;
class SetOSDString;
class SetOSDName;
class RoutingChange;
class RoutingInformation;
class SetStreamPath;
class GetMenuLanguage;
class ReportPhysicalAddress;
class DeviceVendorID;
class UserControlReleased;
class UserControlPressed;
class GiveDevicePowerStatus;
class ReportPowerStatus;
class FeatureAbort;
class Abort;
class Polling;
class InitiateArc;
class TerminateArc;
class RequestShortAudioDescriptor;
class ReportShortAudioDescriptor;
class SystemAudioModeRequest;
class SetSystemAudioMode;
class ReportAudioStatus;
class GiveFeatures;
class ReportFeatures;
class RequestCurrentLatency;
class ReportCurrentLatency;
class Header;
class ReportArcInitiation;
class ReportArcTermination;
class RequestArcInitiation;
class RequestArcTermination;
class GiveAudioStatus;

class MessageProcessor {
public:
    MessageProcessor() {}
    virtual void process(const ActiveSource &msg, const Header &header) {}
    virtual void process(const InActiveSource &msg, const Header &header) {}
    virtual void process(const ImageViewOn &msg, const Header &header) {}
    virtual void process(const TextViewOn &msg, const Header &header) {}
    virtual void process(const RequestActiveSource &msg, const Header &header) {}
    virtual void process(const Standby &msg, const Header &header) {}
    virtual void process(const GetCECVersion &msg, const Header &header) {}
    virtual void process(const CECVersion &msg, const Header &header) {}
    virtual void process(const SetMenuLanguage &msg, const Header &header) {}
    virtual void process(const GiveOSDName &msg, const Header &header) {}
    virtual void process(const GivePhysicalAddress &msg, const Header &header) {}
    virtual void process(const GiveDeviceVendorID &msg, const Header &header) {}
    virtual void process(const SetOSDString &msg, const Header &header) {}
    virtual void process(const SetOSDName &msg, const Header &header) {}
    virtual void process(const RoutingChange &msg, const Header &header) {}
    virtual void process(const RoutingInformation &msg, const Header &header) {}
    virtual void process(const SetStreamPath &msg, const Header &header) {}
    virtual void process(const GetMenuLanguage &msg, const Header &header) {}
    virtual void process(const ReportPhysicalAddress &msg, const Header &header) {}
    virtual void process(const DeviceVendorID &msg, const Header &header) {}
    virtual void process(const UserControlReleased &msg, const Header &header) {}
    virtual void process(const UserControlPressed &msg, const Header &header) {}
    virtual void process(const GiveDevicePowerStatus &msg, const Header &header) {}
    virtual void process(const ReportPowerStatus &msg, const Header &header) {}
    virtual void process(const FeatureAbort &msg, const Header &header) {}
    virtual void process(const Abort &msg, const Header &header) {}
    virtual void process(const Polling &msg, const Header &header) {}
    virtual void process(const InitiateArc &msg, const Header &header) {}
    virtual void process(const TerminateArc &msg, const Header &header) {}
    virtual void process(const RequestShortAudioDescriptor &msg, const Header &header) {}
    virtual void process(const ReportShortAudioDescriptor &msg, const Header &header) {}
    virtual void process(const SystemAudioModeRequest &msg, const Header &header) {}
    virtual void process(const SetSystemAudioMode &msg, const Header &header) {}
    virtual void process(const ReportAudioStatus &msg, const Header &header) {}
    virtual void process(const GiveFeatures &msg, const Header &header) {}
    virtual void process(const ReportFeatures &msg, const Header &header) {}
    virtual void process(const RequestCurrentLatency &msg, const Header &header) {}
    virtual void process(const ReportCurrentLatency &msg, const Header &header) {}
    virtual void process(const ReportArcInitiation &msg, const Header &header) {}
    virtual void process(const ReportArcTermination &msg, const Header &header) {}
    virtual void process(const RequestArcInitiation &msg, const Header &header) {}
    virtual void process(const RequestArcTermination &msg, const Header &header) {}
    virtual void process(const GiveAudioStatus &msg, const Header &header) {}
    virtual ~MessageProcessor() {}
};

class ActiveSource : public DataBlock {
public:
    Op_t opCode(void) const { return ACTIVE_SOURCE; }

    ActiveSource(PhysicalAddress& phyAddress)
        : physicalAddress(phyAddress){
    }

    ActiveSource(const CECFrame& frame, int startPos = 2)
    : physicalAddress(frame, startPos) {}

    PhysicalAddress physicalAddress;
};

class InActiveSource {
public:
    Op_t opCode(void) const{
        return 1;
    }

    InActiveSource() = default;
    InActiveSource(const CECFrame& frame, int startPos = 2)
    : physicalAddress(frame, startPos) {}

    PhysicalAddress physicalAddress;
};

class Header {
public:
    LogicalAddress from;
    LogicalAddress to;
};

class ImageViewOn : public DataBlock{
public:
    ImageViewOn() = default;
    ImageViewOn(const CECFrame& frame, int startPos = 2) {}
};

class TextViewOn : public DataBlock{
public:
    TextViewOn() = default;
    TextViewOn(const CECFrame& frame, int startPos = 2) {}
};

class RequestActiveSource : public DataBlock {
public:
    RequestActiveSource() = default;
    RequestActiveSource(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return REQUEST_ACTIVE_SOURCE; }
};

class Standby : public DataBlock {
public:
    Standby() = default;
    Standby(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return STANDBY; }
};

class CECVersion : public DataBlock {
public:
    CECVersion(const Version& ver)
        : version(ver){
    }

    // Add frame parsing constructor
    CECVersion(const CECFrame& frame, int startPos = 2)
        : version(frame.length() > (size_t)startPos ? frame[startPos] : Version::V_1_4) {}

    Version version;
};

class GetCECVersion : public DataBlock {
public:
    GetCECVersion() = default;
    GetCECVersion(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return GET_CEC_VERSION; }
};

class GetMenuLanguage : public DataBlock {
public:
    GetMenuLanguage() = default;
    GetMenuLanguage(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return GET_MENU_LANGUAGE; }
};

class SetMenuLanguage : public DataBlock {
public:
    SetMenuLanguage(const Language& lan)
        : language(lan){};

    SetMenuLanguage(const CECFrame& frame, int startPos = 2)
        : language([&]() -> const char* {  // ✅ Add explicit return type
            const uint8_t* buf = nullptr;
            size_t len = 0;
            frame.getBuffer(&buf, &len);
            if (buf && len > (size_t)startPos + 2) {
                // Extract 3-byte language code
                static char lang[4] = {0};
                lang[0] = buf[startPos];
                lang[1] = buf[startPos + 1];
                lang[2] = buf[startPos + 2];
                lang[3] = '\0';
                return lang;
            }
            return "eng"; // default - now consistent const char*
        }()) {}

    const Language language;
};

class GiveOSDName : public DataBlock {
public:
    GiveOSDName() = default;
    GiveOSDName(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return GIVE_OSD_NAME; }
};

class GivePhysicalAddress : public DataBlock {
public:
    GivePhysicalAddress() = default;
    GivePhysicalAddress(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return GIVE_PHYSICAL_ADDRESS; }
};

class GiveDeviceVendorID : public DataBlock {
public:
    GiveDeviceVendorID() = default;
    GiveDeviceVendorID(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return GIVE_DEVICE_VENDOR_ID; }
};

class SetOSDString {
public:
    SetOSDString() : osdString() {}
    SetOSDString(const CECFrame& frame, int startPos = 2) : osdString() {}

    OSDString osdString;
};

class RoutingChange : public DataBlock {
public:
    RoutingChange(PhysicalAddress& from1, const PhysicalAddress& to1)
        : from(from1)
        , to(to1)
    {
    }

    RoutingChange(const CECFrame& frame, int startPos = 2)
        : from(frame, startPos)
        , to(frame, startPos + PhysicalAddress::MAX_LEN)
    {
        // No changes to existing fields or logic
    }

    PhysicalAddress from;
    PhysicalAddress to;
};

class RoutingInformation {
public:
    RoutingInformation() = default;
    RoutingInformation(const CECFrame& frame, int startPos = 2)
    : toSink(frame, startPos) {}

    PhysicalAddress toSink;
};

class SetStreamPath : public DataBlock {
public:
    SetStreamPath(const PhysicalAddress& toSink1)
        : toSink(toSink1){
    }

    SetStreamPath(const CECFrame& frame, int startPos = 0)
        : toSink(frame, startPos){
    }

    PhysicalAddress toSink;
};

class GiveDevicePowerStatus : public DataBlock {
public:
    GiveDevicePowerStatus() = default;
    GiveDevicePowerStatus(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return GIVE_DEVICE_POWER_STATUS; }
};

class ReportPhysicalAddress : public DataBlock {
public:
    ReportPhysicalAddress(PhysicalAddress& physAddress, const DeviceType& devType)
        : physicalAddress(physAddress)
        , deviceType(devType){
    }

    ReportPhysicalAddress(const CECFrame& frame, int startPos = 0)
        : physicalAddress(frame, startPos)
        , deviceType(frame, startPos + PhysicalAddress::MAX_LEN){
    }

    PhysicalAddress physicalAddress;
    DeviceType deviceType;
};

class DeviceVendorID : public DataBlock {
public:
    DeviceVendorID(const VendorID& vendor)
        : vendorId(vendor) {
    }

    DeviceVendorID(const CECFrame& frame, int startPos = 2)
        : vendorId([&]{
            const uint8_t* buf = nullptr;
            size_t len = 0;
            frame.getBuffer(&buf, &len);
            if (buf && len >= (size_t)startPos + 3) {
                return VendorID(buf[startPos], buf[startPos+1], buf[startPos+2]);
            }
            return VendorID();
        }()) {}

    VendorID vendorId;
};

class ReportPowerStatus : public DataBlock {
public:
    ReportPowerStatus(PowerStatus stat)
        : status(stat) {
    }

    // Improve frame parsing with proper bounds checking
    ReportPowerStatus(const CECFrame& frame, int startPos = 2)
        : status([&]{
            if (frame.length() > (size_t)startPos) {
                uint8_t powerByte = frame[startPos];
                // Validate power status value
                if (powerByte <= PowerStatus::POWER_STATUS_FEATURE_ABORT) {
                    return PowerStatus(powerByte);
                }
            }
            return PowerStatus(PowerStatus::POWER_STATUS_NOT_KNOWN);
        }()) {}

    PowerStatus status;
};

class OpCode :public DataBlock {
public:
    std::string toString(void) const { return GetOpName(opCode_); }
    Op_t opCode(void) const { return opCode_; }
    OpCode(Op_t opCode): opCode_(opCode){};

private:
    Op_t opCode_;
};

class FeatureAbort : public DataBlock {
public:
    FeatureAbort(const OpCode& abfeature, const AbortReason& abreason)
        : feature(abfeature), reason(abreason){
    }

    // Add frame parsing constructor
    FeatureAbort(const CECFrame& frame, int startPos = 2)
        : feature(frame.length() > (size_t)startPos ? frame[startPos] : 0)
        , reason(frame.length() > (size_t)startPos + 1 ? frame[startPos + 1] : 0)
    {}

    OpCode feature;
    AbortReason reason;
};

class Abort : public DataBlock {
public:
    Abort() = default;
    Abort(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return ABORT; }
};

class UserControlReleased : public DataBlock {
public:
    UserControlReleased() = default;
    UserControlReleased(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return USER_CONTROL_RELEASED; }
};

class Polling : public DataBlock {
public:
    Polling() = default;
    Polling(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return POLLING; }
};

class RequestShortAudioDescriptor : public DataBlock {
public:
    RequestShortAudioDescriptor(const std::vector<uint8_t> formatid, const std::vector<uint8_t> audioFormatCode, uint8_t number_of_descriptor = 1) {

    }

    RequestShortAudioDescriptor(const CECFrame& frame, int startPos = 2) {

    }
};

class UserControlPressed : public DataBlock {
public:
    UserControlPressed(const UICommand& command)
        : uiCommand(command){
    }

    UserControlPressed(const CECFrame& frame, int startPos = 2)
    : uiCommand([&]{
        const uint8_t* buf = nullptr;
        size_t len = 0;
        frame.getBuffer(&buf, &len);
        return (buf && len > (size_t)startPos) ? buf[startPos] : 0;
    }()) {}

    UICommand uiCommand;
};

class ReportAudioStatus {
public:
    Op_t opCode(void) const { return REPORT_AUDIO_STATUS; }

    ReportAudioStatus() = default;

    ReportAudioStatus(const CECFrame& frame, int startPos = 2) {
        if (frame.length() > (size_t)startPos) {
            uint8_t audioStatusByte = frame[startPos];
            // Create AudioStatus with the parsed byte using the constructor
            status = AudioStatus(audioStatusByte);
        }
    }

    AudioStatus status;
};

class SetSystemAudioMode {
public:
    Op_t opCode(void) const { return SET_SYSTEM_AUDIO_MODE; }

    SetSystemAudioMode() = default;

    SetSystemAudioMode(const CECFrame& frame, int startPos = 2) {
        if (frame.length() > (size_t)startPos) {
            uint8_t statusByte = frame[startPos];
            // Create SystemAudioStatus with the parsed byte
            status = SystemAudioStatus(statusByte);
        }
    }

    SystemAudioStatus status;
};

class ReportShortAudioDescriptor {
public:
    Op_t opCode(void) const { return REPORT_SHORT_AUDIO_DESCRIPTOR; }

    ReportShortAudioDescriptor() = default;
    ReportShortAudioDescriptor(const CECFrame& frame, int startPos = 2) {}

    std::vector<ShortAudioDescriptor> shortAudioDescriptor;
    uint8_t numberofdescriptor;
};

class InitiateArc : public DataBlock {
public:
    InitiateArc() = default;
    InitiateArc(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return INITIATE_ARC; }
};

class TerminateArc {
    public:
    TerminateArc() = default;
    TerminateArc(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return TERMINATE_ARC; }
};

class MessageDecoderImpl {
public:
    virtual void decode(const CECFrame& in) const = 0;
};

class MessageDecoder {
private:
    MessageProcessor& processor;
protected:
    static MessageDecoderImpl* impl;


public:
    static void setImpl(MessageDecoderImpl* newImpl);
    MessageDecoder(MessageProcessor& proc);
    void decode(const CECFrame& in);
};

class SetOSDName : public DataBlock {
public:
    SetOSDName(OSDName& OsdName)
        : osdName(OsdName){};

    SetOSDName(const CECFrame& frame, int startPos = 2)
    : osdName([&]{
        const uint8_t* buf = nullptr;
        size_t len = 0;
        frame.getBuffer(&buf, &len);
        if (buf && len > (size_t)startPos)
            return (const char*)(buf + startPos);
        else
            return "";
    }()) {}

    OSDName osdName;
};

class GiveFeatures : public DataBlock {
public:
    Op_t opCode(void) const {return GIVE_FEATURES;}
};

class ReportFeatures : public DataBlock {
public:
    Op_t opCode(void) const {return REPORT_FEATURES;}
    ReportFeatures(const Version &ver_sion,const AllDeviceTypes &allDevice_Types,const std::vector<RcProfile> rc_Profile,std::vector<DeviceFeatures> device_Features) : version(ver_sion), allDeviceTypes(allDevice_Types) {}

    ReportFeatures(const CECFrame& frame, int startPos = 2) : version(Version::V_1_4), allDeviceTypes(0) {}

    Version version;
    AllDeviceTypes allDeviceTypes;
    std::vector<RcProfile> rcProfile;
    std::vector<DeviceFeatures> deviceFeatures;
};

class FrameListener {
public:
    virtual void notify(const CECFrame&) const = 0;
    virtual ~FrameListener(void) {}
};

class ConnectionImpl {
public:
    struct DeviceInfo {
        LogicalAddress addr { LogicalAddress::UNREGISTERED };
        PhysicalAddress phyAddr {};
        PowerStatus powerStatus { PowerStatus::POWER_STATUS_NOT_KNOWN };
        OSDName name { "" };
        VendorID vendor {};
        Version cecVersion { Version::V_1_4 };
        DeviceType type { DeviceType::TV };

        DeviceInfo() = default;  // Now we can default construct since all members have defaults
    };

    virtual ~ConnectionImpl() = default;
    virtual void open() const = 0;
    virtual void close() const = 0;
    virtual void addFrameListener(FrameListener* listener) const = 0;
    virtual void ping(const LogicalAddress& from, const LogicalAddress& to, const Throw_e& doThrow) const = 0;
    virtual void sendToAsync(const LogicalAddress& to, const CECFrame& frame) const = 0;
    virtual void sendTo(const LogicalAddress& to, const CECFrame& frame) const = 0;
    virtual void sendTo(const LogicalAddress& to, const CECFrame& frame, int timeout) const = 0;
    virtual void poll(const LogicalAddress& from, const Throw_e& doThrow) const = 0;
    virtual void sendAsync(const CECFrame &frame) const = 0;

    // Making these public to allow test injection of mock devices
    mutable std::vector<FrameListener*> frameListeners;
    std::map<int, DeviceInfo> connectedDevices;

    void notifyListeners(const CECFrame& frame) const {
        for (auto listener : frameListeners) {
            if (listener) {
                listener->notify(frame);
            }
        }
    }
};

class Connection {
protected:
    static ConnectionImpl* impl;

public:
    Connection(const LogicalAddress& source = LogicalAddress::UNREGISTERED, bool opened = true, const std::string& name = "");
    static Connection& getInstance();
    static void setImpl(ConnectionImpl* newImpl);
    void open();
    void close();
    void addFrameListener(FrameListener* listener);
    void ping(const LogicalAddress& from, const LogicalAddress& to, const Throw_e& doThrow);
    void sendToAsync(const LogicalAddress& to, const CECFrame& frame);
    void sendTo(const LogicalAddress& to, const CECFrame& frame);
    void sendTo(const LogicalAddress& to, const CECFrame& frame, int timeout);
    void poll(const LogicalAddress& from, const Throw_e& doThrow);
    void sendAsync(const CECFrame& frame);
    void setSource(LogicalAddress& from);
};

class SystemAudioModeRequest : public DataBlock {
public:
    SystemAudioModeRequest(const PhysicalAddress& physicaladdress = { 0xf, 0xf, 0xf, 0xf })
        : _physicaladdress(physicaladdress){
    }

    // Improve frame parsing for optional physical address
    SystemAudioModeRequest(const CECFrame& frame, int startPos = 2)
        : _physicaladdress([&]{
            if (frame.length() > (size_t)startPos + 1) {
                // Has physical address parameter
                return PhysicalAddress(frame, startPos);
            } else {
                // No physical address - use invalid address
                return PhysicalAddress(0xf, 0xf, 0xf, 0xf);
            }
        }()) {}

    PhysicalAddress _physicaladdress;
};

class GiveAudioStatus : public DataBlock {
public:
    Op_t opCode(void) const { return GIVE_AUDIO_STATUS; }
};

class LibCCECImpl {
public:
    virtual void init(const char* name) const = 0;
    virtual void init() const = 0;
    virtual void term() const = 0;
    virtual void getPhysicalAddress(uint32_t* physicalAddress) const = 0;
    virtual int addLogicalAddress(const LogicalAddress& source) const = 0;
    virtual int getLogicalAddress(int devType) const = 0;
};

class LibCCEC {
protected:
        static  LibCCECImpl* impl;
public:
    static LibCCEC& getInstance();
    static void setImpl(LibCCECImpl* newImpl);

    void init(const char* name);
    void init();
    void term();
    void getPhysicalAddress(uint32_t* physicalAddress);
    int addLogicalAddress(const LogicalAddress& source);
    int getLogicalAddress(int devType);
};

class RequestArcInitiation : public DataBlock {
public:
    RequestArcInitiation() = default;
    RequestArcInitiation(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return REQUEST_ARC_INITIATION; }
};

class ReportArcInitiation : public DataBlock {
public:
    ReportArcInitiation() = default;
    ReportArcInitiation(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return REPORT_ARC_INITIATED; }
};

class RequestArcTermination : public DataBlock {
public:
    RequestArcTermination() = default;
    RequestArcTermination(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return REQUEST_ARC_TERMINATION; }
};

class ReportArcTermination : public DataBlock {
public:
    ReportArcTermination() = default;
    ReportArcTermination(const CECFrame& frame, int startPos = 2) {}
    Op_t opCode(void) const { return REPORT_ARC_TERMINATED; }
};

class RequestCurrentLatency : public DataBlock
{
public:
    Op_t opCode(void) const {return REQUEST_CURRENT_LATENCY;}
    RequestCurrentLatency(const PhysicalAddress &physicaladdres = {0xf,0xf,0xf,0xf} ): physicaladdress(physicaladdres) {}

    RequestCurrentLatency(const CECFrame& frame, int startPos = 2)
    : physicaladdress(frame, startPos) {}

    PhysicalAddress physicaladdress;
};

class ReportCurrentLatency : public DataBlock
{
public:
    Op_t opCode(void) const {return REPORT_CURRENT_LATENCY;}
    ReportCurrentLatency(const PhysicalAddress &physicaladdress, uint8_t videoLatency, uint8_t latencyFlags, uint8_t audioOutputDelay = 0){}

    ReportCurrentLatency(const CECFrame& frame, int startPos = 2) {}
};

class MessageEncoderImpl {
public:
    virtual CECFrame& encode(const DataBlock& m) const = 0;
    virtual CECFrame& encode(const UserControlPressed& m) const = 0;
};

class MessageEncoder {
protected:
        static MessageEncoderImpl* impl;
public:

    static void setImpl(MessageEncoderImpl* newImpl);

    CECFrame& encode(const UserControlPressed m);
    CECFrame& encode(const DataBlock m);
};

class IOException : public Exception
{
        public:
                virtual const char* what() const throw()
                {
                        return "IO Exception..";
                }
};
