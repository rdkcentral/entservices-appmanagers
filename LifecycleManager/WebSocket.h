/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <map>
#include <string>
#include <semaphore.h>

#pragma once

namespace WPEFramework {
namespace Plugin {

    typedef websocketpp::client<websocketpp::config::asio_client> WebSocketAsioClient;

    class ConnectionMetaData
    {
        public:
            typedef websocketpp::lib::shared_ptr<ConnectionMetaData> sharedPtr;
            ConnectionMetaData(int id, websocketpp::connection_hdl handle, std::string uri);
            ~ConnectionMetaData();
            void onOpen(WebSocketAsioClient * c, websocketpp::connection_hdl handle);
            void onFail(WebSocketAsioClient * c, websocketpp::connection_hdl handle);
            void onClose(WebSocketAsioClient * c, websocketpp::connection_hdl handle);
            void onMessage(websocketpp::connection_hdl, WebSocketAsioClient::message_ptr msg);
            websocketpp::connection_hdl getHandle() const;
            int getIdentifier() const;
            std::string getStatus() const;
            std::string getLastMessage() const;
            std::string getErrorReason() const;
            std::string getURL() const;
            void waitForEvent();

        private:
            int mIdentifier;
            websocketpp::connection_hdl mHandle;
            std::string mStatus;
            std::string mURI;
            std::string mServerResponse;
            std::string mLastMessage;
            std::string mErrorReason;
            sem_t mEventSem;
    };

    class WebSocketEndPoint
    {
        public:
            WebSocketEndPoint();
            ~WebSocketEndPoint();
	    void initialize();
            void deinitialize();
            int connect(std::string const & uri, bool wait=false);
	    bool send(int id, std::string message, std::string& response);
            void close(int id, websocketpp::close::status::value code);
            ConnectionMetaData::sharedPtr getMetadata(int id) const;

        private:
            typedef std::map<int,ConnectionMetaData::sharedPtr> connectionList;
            WebSocketAsioClient mEndPoint;
            websocketpp::lib::shared_ptr<websocketpp::lib::thread> mThread;
            connectionList mConnectionList;
            int mNextIdentifier;
    };

} // namespace Plugin
} // namespace WPEFramework
