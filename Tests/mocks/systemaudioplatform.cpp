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

#include "systemaudioplatform.h"
#include <gmock/gmock.h>

ISystemAudioPlatform* SystemAudioPlatformMockImpl::impl = nullptr;

SystemAudioPlatformMockImpl::SystemAudioPlatformMockImpl() {}

void SystemAudioPlatformMockImpl::setImpl(ISystemAudioPlatform* newImpl)
{
    impl = newImpl;
}

void SystemAudioPlatformMockImpl::systemAudioInitialize()
{
    impl->systemAudioInitialize();
}

void SystemAudioPlatformMockImpl::systemAudioDeinitialize()
{
    impl->systemAudioDeinitialize();
}

void SystemAudioPlatformMockImpl::systemAudioChangePrimaryVol(MixGain gain, int volume)
{
    impl->systemAudioChangePrimaryVol(gain, volume);
}

void SystemAudioPlatformMockImpl::systemAudioSetDetectTime(int detectTimeMs)
{
    impl->systemAudioSetDetectTime(detectTimeMs);
}

void SystemAudioPlatformMockImpl::systemAudioSetHoldTime(int holdTimeMs)
{
    impl->systemAudioSetHoldTime(holdTimeMs);
}

void SystemAudioPlatformMockImpl::systemAudioSetThreshold(double thresHold)
{
    impl->systemAudioSetThreshold(thresHold);
}

void SystemAudioPlatformMockImpl::systemAudioSetVolume(GstElement* audioVolume, AudioType audioType, PlayMode playMode, int thisVol)
{
    impl->systemAudioSetVolume(audioVolume, audioType, playMode, thisVol);
}

bool SystemAudioPlatformMockImpl::systemAudioGeneratePipeline(GstElement** pipeline, GstElement** source, GstElement* capsfilter, GstElement** audioSink, GstElement** audioVolume, AudioType type, PlayMode mode, SourceType sourceType, bool smartVolumeEnable)
{
    return impl->systemAudioGeneratePipeline(pipeline, source, capsfilter, audioSink, audioVolume, type, mode, sourceType, smartVolumeEnable);
}

void (*systemAudioInitialize)(void) = &SystemAudioPlatformMockImpl::systemAudioInitialize;
void (*systemAudioDeinitialize)(void) = &SystemAudioPlatformMockImpl::systemAudioDeinitialize;
void (*systemAudioChangePrimaryVol)(MixGain, int) = &SystemAudioPlatformMockImpl::systemAudioChangePrimaryVol;
void (*systemAudioSetDetectTime)(int) = &SystemAudioPlatformMockImpl::systemAudioSetDetectTime;
void (*systemAudioSetHoldTime)(int) = &SystemAudioPlatformMockImpl::systemAudioSetHoldTime;
void (*systemAudioSetThreshold)(double) = &SystemAudioPlatformMockImpl::systemAudioSetThreshold;
void (*systemAudioSetVolume)(GstElement*, AudioType, PlayMode, int) = &SystemAudioPlatformMockImpl::systemAudioSetVolume;
bool (*systemAudioGeneratePipeline)(GstElement**, GstElement**, GstElement*, GstElement**, GstElement**, AudioType, PlayMode, SourceType, bool) = &SystemAudioPlatformMockImpl::systemAudioGeneratePipeline;
