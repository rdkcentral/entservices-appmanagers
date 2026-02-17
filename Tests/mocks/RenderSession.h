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

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <memory>

#define __LOGGER_FUNC__     __func__

namespace subttxrend {
    namespace common {

class LoggerManager {
public:
    LoggerManager() = default;

    static LoggerManager* getInstance() {
        static LoggerManager instance;
        return &instance;
    }

    void init(const std::string* config) {
        loggerConfig = *config;
    }

    const std::string& getLoggerConfig() const {
        return loggerConfig;
    }

    std::string loggerConfig = "default_logger_config";  // Ensure persistence
};

class NonCopyable
{
public:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

    }  // namespace common
}  // namespace subttxrend

namespace subttxrend {
    namespace socksrc {

class PacketReceiver {
public:
    PacketReceiver() = default;
    virtual ~PacketReceiver() = default;
};

    }  // namespace socksrc
}  // namespace subttxrend

namespace subttxrend {
    namespace cc {
        enum class Opacity {
            TRANSPARENT,
            TRANSLUCENT,
            SOLID
        };
    }
}  // namespace subttxrend

namespace subttxrend
{
namespace ttmlengine
{

class TtmlEngine
{
public:

    /**
     * Constructor.
     */
    TtmlEngine() = default;

    /**
     * Destructor.
     */
    virtual ~TtmlEngine() = default ;
};
}
}

namespace subttxrend
{
namespace ctrl
{

class Channel
{
    public:

        /**
         * Constructor.
         */
        Channel();
        /**
         * Destructor.
         */
        ~Channel() = default;
};

class ControllerInterface : private common::NonCopyable
{
  public:
    virtual ~ControllerInterface() = default;
};

class Options {
public:
    // Options(int, std::nullptr_t) {}
    Options(const uint32_t& argc, char** argv) {}
};

class Configuration {
public:
    Configuration(const Options& options) {}

    std::string getMainContextSocketPath() const { return ""; }

    const std::string& getLoggerConfig() const {
        return loggerConfig;
    }

private:
    std::string loggerConfig = "mock_logger_config";
};
/**
 * Controller class for TTML IP VOD subtitles.
 */
class TtmlController final : public ControllerInterface
{
  public:
    /**
     * Constructor.
    */

    TtmlController() = default;
    ~TtmlController();

    void setCustomTtmlStyling(const std::string& styling);

  private:
    /** Current subtitle channel. */
    Channel m_channel;

    /** TTML decoder instance */
    std::unique_ptr<ttmlengine::TtmlEngine> m_ttmlEngine;
};

} // namespace ctrl
} // namespace subttxrend


// Forward declaration (optional now)
class RenderSessionImpl;

class RenderSession : public subttxrend::socksrc::PacketReceiver {
protected:
    static RenderSessionImpl* impl;

public:
    enum class CcServiceType {
        CEA608 = 0,
        CEA708 = 1
    };

    enum class DataType {
        PES,
        TTML,
        CC,
        WEBVTT
    };

    enum class SessionType {
        NONE,
        CC,
        TTX,
        DVB,
        WEBVTT,
        TTML,
        SCTE
    };

    RenderSession();
    RenderSession(subttxrend::ctrl::Configuration&, const std::string&);
    RenderSession(const RenderSession& obj) = delete;

    static void setImpl(RenderSessionImpl* newImpl);

    void start();
    void stop();
    std::string getDisplayName() const;
    std::string getSocketName() const;
    void touchTime();
    void pause();
    void resume();
    void mute();
    void unmute();
    void reset();
    void selectCcService(CcServiceType type, uint32_t iServiceId);
    void selectTtxService(uint16_t page);
    void selectDvbService(uint16_t compositionPageId, uint16_t ancillaryPageId);
    void selectWebvttService(uint32_t iVideoWidth, uint32_t iVideoHeight);
    void selectTtmlService(uint32_t iVideoWidth, uint32_t iVideoHeight);
    void selectScteService();
    void sendTimestamp(uint64_t iMediaTimestampMs);
    bool sendData(DataType type, const std::string &data, int32_t offsetMs);
    SessionType getSessionType() const;
    void setTextForClosedCaptionPreview(const std::string &text);
    void setCustomTtmlStyling(const std::string& styling);
    bool hasCustomTtmlStyling() const;
    void setCcAttributes(uint32_t fontColor, uint32_t fontOpacity, uint32_t fontStyle, uint32_t fontSize,
                         uint32_t edgeType, uint32_t edgeColor, uint32_t backgroundColor,
                         uint32_t backgroundOpacity, uint32_t windowColor, uint32_t windowOpacity);
};

// Move this after RenderSession so nested enums are visible
class RenderSessionImpl {
public:
    using CcServiceType = RenderSession::CcServiceType;
    using DataType = RenderSession::DataType;
    using SessionType = RenderSession::SessionType;

    virtual ~RenderSessionImpl() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::string getDisplayName() const = 0;
    virtual std::string getSocketName() const = 0;
    virtual void touchTime() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void mute() = 0;
    virtual void unmute() = 0;
    virtual void reset() = 0;
    virtual void selectCcService(CcServiceType type, uint32_t iServiceId) = 0;
    virtual void selectTtxService(uint16_t page) = 0;
    virtual void selectDvbService(uint16_t compositionPageId, uint16_t ancillaryPageId) = 0;
    virtual void selectWebvttService(uint32_t iVideoWidth, uint32_t iVideoHeight) = 0;
    virtual void selectTtmlService(uint32_t iVideoWidth, uint32_t iVideoHeight) = 0;
    virtual void selectScteService() = 0;
    virtual void sendTimestamp(uint64_t iMediaTimestampMs) = 0;
    virtual bool sendData(DataType type, const std::string &data, int32_t offsetMs) = 0;
    virtual SessionType getSessionType() const = 0;
    virtual void setTextForClosedCaptionPreview(const std::string &text) = 0;
    virtual void setCustomTtmlStyling(const std::string& styling) = 0;
    virtual bool hasCustomTtmlStyling() const = 0;
    virtual void setCcAttributes(uint32_t fontColor, uint32_t fontOpacity, uint32_t fontStyle, uint32_t fontSize,
                                 uint32_t edgeType, uint32_t edgeColor, uint32_t backgroundColor,
                                 uint32_t backgroundOpacity, uint32_t windowColor, uint32_t windowOpacity) = 0;
};


