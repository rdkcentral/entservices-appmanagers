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

#ifndef SYSTEMAUDIO_PLATFORM
#define SYSTEMAUDIO_PLATFORM

#include <gst/audio/audio.h>
#include <gst/gst.h>
#include <stdint.h>
#include <string>

enum AudioType {
    AudioType_None,
    PCM,
    MP3,
    WAV
};

enum SourceType {
    SourceType_None,
    DATA,
    HTTPSRC,
    FILESRC,
    WEBSOCKET
};

enum PlayMode {
    PlayMode_None,
    SYSTEM,
    APP
};

enum MixGain {
    MIXGAIN_PRIM,
    MIXGAIN_SYS, // direct-mode=false, aml calls it sys mode
    MIXGAIN_TTS // tts=mode=true, AML calls it app mode
};

class ISystemAudioPlatform {
public:
    ISystemAudioPlatform() = default;
    virtual ~ISystemAudioPlatform() = default;

    virtual void systemAudioInitialize() = 0;
    virtual void systemAudioDeinitialize() = 0;
    virtual void systemAudioChangePrimaryVol(MixGain gain, int volume) = 0;
    virtual void systemAudioSetDetectTime(int detectTimeMs) = 0;
    virtual void systemAudioSetHoldTime(int holdTimeMs) = 0;
    virtual void systemAudioSetThreshold(double thresHold) = 0;
    virtual void systemAudioSetVolume(GstElement* audioVolume, AudioType audioType, PlayMode playMode, int thisVol) = 0;
    virtual bool systemAudioGeneratePipeline(GstElement** pipeline, GstElement** source, GstElement* capsfilter, GstElement** audioSink, GstElement** audioVolume, AudioType type, PlayMode mode, SourceType sourceType, bool smartVolumeEnable) = 0;
};

class SystemAudioPlatformMockImpl {

protected:
    static ISystemAudioPlatform* impl;

public:
    SystemAudioPlatformMockImpl();
    virtual ~SystemAudioPlatformMockImpl() = default;
    SystemAudioPlatformMockImpl(const SystemAudioPlatformMockImpl&) = delete;

    static void setImpl(ISystemAudioPlatform* newImpl);

    static void systemAudioInitialize();
    static void systemAudioDeinitialize();
    static void systemAudioChangePrimaryVol(MixGain gain, int volume);
    static void systemAudioSetDetectTime(int detectTimeMs);
    static void systemAudioSetHoldTime(int holdTimeMs);
    static void systemAudioSetThreshold(double thresHold);
    static void systemAudioSetVolume(GstElement* audioVolume, AudioType audioType, PlayMode playMode, int thisVol);
    static bool systemAudioGeneratePipeline(GstElement** pipeline, GstElement** source, GstElement* capsfilter, GstElement** audioSink, GstElement** audioVolume, AudioType type, PlayMode mode, SourceType sourceType, bool smartVolumeEnable);
};

extern void (*systemAudioInitialize)(void);
extern void (*systemAudioDeinitialize)(void);
extern void (*systemAudioChangePrimaryVol)(MixGain, int);
extern void (*systemAudioSetDetectTime)(int);
extern void (*systemAudioSetHoldTime)(int);
extern void (*systemAudioSetThreshold)(double);
extern void (*systemAudioSetVolume)(GstElement*, AudioType, PlayMode, int);
extern bool (*systemAudioGeneratePipeline)(GstElement** pipeline, GstElement** source, GstElement* capsfilter, GstElement** audioSink, GstElement** audioVolume, AudioType type, PlayMode mode, SourceType sourceType, bool smartVolumeEnable);
#endif