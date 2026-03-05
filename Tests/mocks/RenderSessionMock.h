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

#include <gmock/gmock.h>
#include "RenderSession.h"

class RenderSessionMock : public RenderSessionImpl {
public:
    virtual ~RenderSessionMock() = default;

    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(std::string, getDisplayName, (), (const, override));
    MOCK_METHOD(std::string, getSocketName, (), (const, override));
    MOCK_METHOD(void, touchTime, (), (override));
    MOCK_METHOD(void, pause, (), (override));
    MOCK_METHOD(void, resume, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(void, unmute, (), (override));  
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(void, selectCcService, (CcServiceType type, uint32_t iServiceId), (override));
    MOCK_METHOD(void, selectTtxService, (uint16_t page), (override));
    MOCK_METHOD(void, selectDvbService, (uint16_t compositionPageId, uint16_t ancillaryPageId), (override));
    MOCK_METHOD(void, selectWebvttService, (uint32_t iVideoWidth, uint32_t iVideoHeight), (override));
    MOCK_METHOD(void, selectTtmlService, (uint32_t iVideoWidth, uint32_t iVideoHeight), (override));
    MOCK_METHOD(void, selectScteService, (), (override));
    MOCK_METHOD(void, sendTimestamp, (uint64_t iMediaTimestampMs), (override));  
    MOCK_METHOD(bool, sendData, (DataType type, const std::string &data, int32_t offsetMs), (override));
    MOCK_METHOD(SessionType, getSessionType, (), (const, override));
    MOCK_METHOD(void, setTextForClosedCaptionPreview, (const std::string &text), (override));
    MOCK_METHOD(void, setCustomTtmlStyling, (const std::string& styling), (override));
    MOCK_METHOD(bool, hasCustomTtmlStyling, (), (const, override));
    MOCK_METHOD(void, setCcAttributes, (uint32_t fontColor, uint32_t fontOpacity, uint32_t fontStyle, uint32_t fontSize, 
                                        uint32_t edgeType, uint32_t edgeColor, uint32_t backgroundColor, 
                                        uint32_t backgroundOpacity, uint32_t windowColor, uint32_t windowOpacity), (override));
};

