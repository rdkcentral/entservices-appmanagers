/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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

#include <gmock/gmock.h>

#include "IPackageImplDummy.h"

namespace packagemanager {

class LibPackageMock : public IPackageImplDummy {
public:
    LibPackageMock() = default;
    ~LibPackageMock() override = default;

    MOCK_METHOD(Result, Initialize, (const std::string& configStr, ConfigMetadataArray& aConfigMetadata), (override));
    MOCK_METHOD(Result, Install, (const std::string& packageId, const std::string& version, const NameValues& additionalMetadata, const std::string& fileLocator, ConfigMetaData& configMetadata), (override));
    MOCK_METHOD(Result, Uninstall, (const std::string& packageId), (override));

    MOCK_METHOD(Result, Lock, (const std::string& packageId, const std::string& version, std::string& unpackedPath, ConfigMetaData& configMetadata, NameValues& additionalLocks), (override));
    MOCK_METHOD(Result, Unlock, (const std::string& packageId, const std::string& version), (override));

    MOCK_METHOD(Result, GetLockInfo, (const std::string& packageId, const std::string& version, std::string& unpackedPath, bool& locked), (override));
    MOCK_METHOD(Result, GetList, (std::string& packageList), (override));
    MOCK_METHOD(Result, Lock, (const std::string& packageId, const std::string& version, std::string& unpackedPath, ConfigMetaData& configMetadata), (override));

    MOCK_METHOD(Result, GetFileMetadata, (const std::string& fileLocator, std::string& packageId, std::string& version, ConfigMetaData& configMetadata), (override));
};

} // namespace packagemanager

