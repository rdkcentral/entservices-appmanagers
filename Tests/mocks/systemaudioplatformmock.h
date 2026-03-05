#pragma once

#include "systemaudioplatform.h"
#include <gmock/gmock.h>

class SystemAudioPlatformAPIMock : public ISystemAudioPlatform {
public:
    SystemAudioPlatformAPIMock() = default;
    virtual ~SystemAudioPlatformAPIMock() = default;

    MOCK_METHOD(void, systemAudioInitialize, (), (override));
    MOCK_METHOD(void, systemAudioDeinitialize, (), (override));
    MOCK_METHOD(void, systemAudioChangePrimaryVol, (MixGain gain, int volume), (override));
    MOCK_METHOD(void, systemAudioSetDetectTime, (int detectTimeMs), (override));
    MOCK_METHOD(void, systemAudioSetHoldTime, (int holdTimeMs), (override));
    MOCK_METHOD(void, systemAudioSetThreshold, (double thresHold), (override));
    MOCK_METHOD(void, systemAudioSetVolume, (GstElement * audioVolume, AudioType audioType, PlayMode playMode, int thisVol), (override));
    MOCK_METHOD(bool, systemAudioGeneratePipeline, (GstElement * *pipeline, GstElement** source, GstElement* capsfilter, GstElement** audioSink, GstElement** audioVolume, AudioType type, PlayMode mode, SourceType sourceType, bool smartVolumeEnable), (override));
};