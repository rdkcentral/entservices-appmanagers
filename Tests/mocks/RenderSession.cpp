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

#include "RenderSession.h"
#include <gmock/gmock.h>  

RenderSessionImpl* RenderSession::impl = nullptr;

RenderSession::RenderSession(subttxrend::ctrl::Configuration& config, const std::string& param) {
    if (!impl) {

    }
}

void RenderSession::setImpl(RenderSessionImpl* newImpl)
{
    // Ensures that 'impl' is either nullptr or the new implementation being set is valid
    EXPECT_TRUE((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

void RenderSession::start()
{
    EXPECT_NE(impl, nullptr);
    impl->start();
}

void RenderSession::stop()
{
    EXPECT_NE(impl, nullptr);
    impl->stop();
}

std::string RenderSession::getDisplayName() const
{
    EXPECT_NE(impl, nullptr);
    return impl->getDisplayName();
}

std::string RenderSession::getSocketName() const
{
    EXPECT_NE(impl, nullptr);
    return impl->getSocketName();
}

void RenderSession::touchTime()
{
    EXPECT_NE(impl, nullptr);
    impl->touchTime();
}

void RenderSession::pause()
{
    EXPECT_NE(impl, nullptr);
    impl->pause();
}

void RenderSession::resume()
{
    EXPECT_NE(impl, nullptr);
    impl->resume();
}

void RenderSession::mute()
{
    EXPECT_NE(impl, nullptr);
    impl->mute();
}

void RenderSession::unmute()
{
    EXPECT_NE(impl, nullptr);
    impl->unmute();
}

void RenderSession::reset()
{
    EXPECT_NE(impl, nullptr);
    impl->reset();
}

void RenderSession::selectCcService(CcServiceType type, uint32_t iServiceId)
{
    EXPECT_NE(impl, nullptr);
     impl->selectCcService(type,iServiceId);
    // impl->selectCcService(static_cast<RenderSessionImpl::CcServiceType>(type), iServiceId);
}

void RenderSession::selectTtxService(uint16_t page)
{
    EXPECT_NE(impl, nullptr);
    impl->selectTtxService(page);
}

void RenderSession::selectDvbService(uint16_t compositionPageId, uint16_t ancillaryPageId)
{
    EXPECT_NE(impl, nullptr);
    impl->selectDvbService(compositionPageId, ancillaryPageId);
}

void RenderSession::selectWebvttService(uint32_t iVideoWidth, uint32_t iVideoHeight)
{
    EXPECT_NE(impl, nullptr);
    impl->selectWebvttService(iVideoWidth, iVideoHeight);
}

void RenderSession::selectTtmlService(uint32_t iVideoWidth, uint32_t iVideoHeight)
{
    EXPECT_NE(impl, nullptr);
    impl->selectTtmlService(iVideoWidth, iVideoHeight);
}

void RenderSession::selectScteService()
{
    EXPECT_NE(impl, nullptr);
    impl->selectScteService();
}

void RenderSession::sendTimestamp(uint64_t iMediaTimestampMs)
{
    EXPECT_NE(impl, nullptr);
    impl->sendTimestamp(iMediaTimestampMs);
}

bool RenderSession::sendData(DataType type, const std::string& data, int32_t offsetMs)
{
    EXPECT_NE(impl, nullptr);
    return impl->sendData(type, data, offsetMs);
}

RenderSession::SessionType RenderSession::getSessionType() const 
{
    EXPECT_NE(impl, nullptr);
    return (impl->getSessionType());
}

void RenderSession::setTextForClosedCaptionPreview(const std::string& text)
{
    EXPECT_NE(impl, nullptr);
    impl->setTextForClosedCaptionPreview(text);
}

void RenderSession::setCustomTtmlStyling(const std::string& styling)
{
    EXPECT_NE(impl, nullptr);
    impl->setCustomTtmlStyling(styling);
}

bool RenderSession::hasCustomTtmlStyling() const
{
    EXPECT_NE(impl, nullptr);
    return impl->hasCustomTtmlStyling();
}

void RenderSession::setCcAttributes(uint32_t fontColor, uint32_t fontOpacity, uint32_t fontStyle, uint32_t fontSize,
                                    uint32_t edgeType, uint32_t edgeColor, uint32_t backgroundColor,
                                    uint32_t backgroundOpacity, uint32_t windowColor, uint32_t windowOpacity)
{
    EXPECT_NE(impl, nullptr);
    impl->setCcAttributes(fontColor, fontOpacity, fontStyle, fontSize,
                          edgeType, edgeColor, backgroundColor,
                          backgroundOpacity, windowColor, windowOpacity);
}

