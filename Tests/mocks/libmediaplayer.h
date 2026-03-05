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

#ifndef __LIBMEDIAPLAYER_H__
#define __LIBMEDIAPLAYER_H__

#include <string>
#include <memory>

namespace libmediaplayer {

    static const int QAM = 0;
    static const int CAS_TYPE_ANYCAS = 0;

    struct notification_payload {
        std::string m_message;
        long long m_code;
        notification_payload(const std::string&, int, const std::string&, const std::string&) {}
        notification_payload() : m_message(""), m_code(0) {}
    };

    typedef void (*EventCallback)(notification_payload*, void*);
    typedef void (*ErrorCallback)(notification_payload*, void*);

    class CASService {
    public:
        virtual ~CASService() = default;
        virtual void sendCASData(std::string&) {}
    };

    class AnyCasCASServiceImpl : public CASService {
    public:
        AnyCasCASServiceImpl(const std::string&) {}
        bool initializeCasService(void*, void*) { return true; }
        bool registerCallbacks(EventCallback, ErrorCallback, void*) { return true; }
        bool stopCasService() { return true; }
        void sendCASData(std::string&) override {}
    };

    class mediaplayer {
    public:
        mediaplayer() : casService(std::make_shared<AnyCasCASServiceImpl>("dummy")) {}
        static int initialize(int, bool, bool) { return 0; }
        static mediaplayer* createMediaPlayer(int, const std::string&, int) { return new mediaplayer(); }
        int registerEventCallbacks(EventCallback, ErrorCallback, void*) { return 0; }
        int stop() { return 0; }
        std::weak_ptr<CASService> getCasServiceInstance() { return casService; }
    private:
        std::shared_ptr<CASService> casService;
    };

}

#endif // __LIBMEDIAPLAYER_H__