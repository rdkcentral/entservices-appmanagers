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

#include "omi_proxy.hpp"
#include <gmock/gmock.h>

class MockOmiProxy : public omi::IOmiProxy {
public:
    virtual ~MockOmiProxy() = default;

    MOCK_METHOD(bool, mountCryptedBundle, (const std::string& id, const std::string& rootfs_file_path, const std::string& config_json_path, std::string& bundlePath), (override));
    MOCK_METHOD(bool, umountCryptedBundle, (const std::string& id), (override));
    MOCK_METHOD(long unsigned, registerListener, (const OmiErrorListener& listener, const void* cbParams), (override));
    MOCK_METHOD(void, unregisterListener, (long unsigned tag), (override));
};
