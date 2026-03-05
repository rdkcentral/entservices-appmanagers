#pragma once
#include <gmock/gmock.h>
#include "PlayerInfo.h"
#include <interfaces/IPlayerInfo.h>

using namespace WPEFramework;

class MockDolbyOutput : public WPEFramework::Exchange::Dolby::IOutput {
public:
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t), (override));

    MOCK_METHOD(uint32_t, Register, (WPEFramework::Exchange::Dolby::IOutput::INotification*), (override));
    MOCK_METHOD(uint32_t, Unregister, (WPEFramework::Exchange::Dolby::IOutput::INotification*), (override));

    MOCK_METHOD(uint32_t, SoundMode, (WPEFramework::Exchange::Dolby::IOutput::SoundModes&), (const, override));
    MOCK_METHOD(uint32_t, Mode, (const WPEFramework::Exchange::Dolby::IOutput::Type&), (override));
    MOCK_METHOD(uint32_t, Mode, (WPEFramework::Exchange::Dolby::IOutput::Type&), (const, override));

    MOCK_METHOD(uint32_t, EnableAtmosOutput, (const bool& enable), (override));
    MOCK_METHOD(uint32_t, AtmosMetadata, (bool& supported), (const, override));
};
class MockAudioCodecIterator : public Exchange::IPlayerProperties::IAudioCodecIterator {
public:
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t), (override));

    MOCK_METHOD(void, Reset, (uint32_t), (override));
    MOCK_METHOD(bool, Next, (Exchange::IPlayerProperties::AudioCodec&), (override));
    MOCK_METHOD(uint32_t, Count, (), (const, override));
    MOCK_METHOD(bool, Previous, (Exchange::IPlayerProperties::AudioCodec&), (override));
    MOCK_METHOD(bool, IsValid, (), (const, override));
    MOCK_METHOD(Exchange::IPlayerProperties::AudioCodec, Current, (), (const, override));
};
class MockVideoCodecIterator : public Exchange::IPlayerProperties::IVideoCodecIterator {
public:
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t), (override));

    MOCK_METHOD(void, Reset, (uint32_t), (override));
    MOCK_METHOD(bool, Next, (Exchange::IPlayerProperties::VideoCodec&), (override));
    MOCK_METHOD(uint32_t, Count, (), (const, override));
    MOCK_METHOD(bool, Previous, (Exchange::IPlayerProperties::VideoCodec&), (override));
    MOCK_METHOD(bool, IsValid, (), (const, override));
    MOCK_METHOD(Exchange::IPlayerProperties::VideoCodec, Current, (), (const, override));
};
class MockPlayerProperties : public WPEFramework::Exchange::IPlayerProperties {
public:
    MOCK_METHOD(void*, QueryInterface, (uint32_t), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));

    MOCK_METHOD(uint32_t, AudioCodecs, (Exchange::IPlayerProperties::IAudioCodecIterator*&), (const, override));
    MOCK_METHOD(uint32_t, VideoCodecs, (Exchange::IPlayerProperties::IVideoCodecIterator*&), (const, override));
    MOCK_METHOD(uint32_t, Resolution, (Exchange::IPlayerProperties::PlaybackResolution&), (const, override));
    MOCK_METHOD(uint32_t, IsAudioEquivalenceEnabled, (bool&), (const, override));
};
class MockRemoteConnection : public RPC::IRemoteConnection {
public:
    MOCK_METHOD(uint32_t, Id, (), (const, override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(uint32_t, RemoteId, (), (const, override));
    MOCK_METHOD(void*, Acquire, (uint32_t, const std::string&, uint32_t, uint32_t), (override));
    MOCK_METHOD(void, Terminate, (), (override));
    MOCK_METHOD(uint32_t, Launch, (), (override));
    MOCK_METHOD(void, PostMortem, (), (override));
    MOCK_METHOD(void*, QueryInterface, (uint32_t), (override));
};