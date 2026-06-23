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

// @stubgen:skip
//
// gmock-based mock for the libpackage `packagemanager::IPackageImpl` interface.
//
// The production code (`PackageManagerImplementation.cpp`) selects either the
// real `IPackageImpl` (from libpackage.so) or the test fake `IPackageImplDummy`
// based on the `UNIT_TEST` macro. L1 builds add `-DUNIT_TEST` via the
// `ENABLE_UNIT_TESTS` option, so they pick `IPackageImplDummy`.
//
// `IPackageImplDummy` returns canned values; that is fine for smoke coverage
// but it cannot:
//   - assert that a given API was called with specific arguments,
//   - simulate failure paths (e.g. PERSISTENCE_FAILURE on Install),
//   - vary behavior per-test.
//
// This mock fixes those gaps. Wire it in like this:
//
//   #include "IPackageImplMock.h"
//   using ::testing::NiceMock;
//   using ::testing::Return;
//   using ::testing::_;
//
//   class PackageManagerTest : public ::testing::Test {
//   protected:
//       std::shared_ptr<NiceMock<packagemanager::IPackageImplMock>> mPkgImplMock;
//
//       void SetUp() override {
//           mPkgImplMock = std::make_shared<NiceMock<packagemanager::IPackageImplMock>>();
//           // Optionally fall back to the dummy's behavior for unprogrammed calls:
//           mPkgImplMock->DelegateToDefault();
//           packagemanager::IPackageImplDummy::setInstance(mPkgImplMock);
//       }
//
//       void TearDown() override {
//           packagemanager::IPackageImplDummy::clearInstance();
//           mPkgImplMock.reset();
//       }
//   };
//
//   TEST_F(PackageManagerTest, InstallFailsWhenPersistenceBroken) {
//       EXPECT_CALL(*mPkgImplMock,
//                   Install(::testing::StrEq("MyApp"), _, _, _, _))
//           .WillOnce(Return(packagemanager::PERSISTENCE_FAILURE));
//       // ... drive the SUT and assert ...
//   }

#include <gmock/gmock.h>

#include "IPackageImplDummy.h"

namespace packagemanager
{
    class IPackageImplMock : public packagemanager::IPackageImplDummy
    {
    public:
        IPackageImplMock() = default;
        ~IPackageImplMock() override = default;

        MOCK_METHOD(Result, Initialize,
                    (const std::string &configStr, ConfigMetadataArray &aConfigMetadata),
                    (override));

        MOCK_METHOD(Result, Install,
                    (const std::string &packageId,
                     const std::string &version,
                     const NameValues &additionalMetadata,
                     const std::string &fileLocator,
                     ConfigMetaData &configMetadata),
                    (override));

        MOCK_METHOD(Result, Uninstall,
                    (const std::string &packageId),
                    (override));

        MOCK_METHOD(Result, Lock,
                    (const std::string &packageId,
                     const std::string &version,
                     std::string &unpackedPath,
                     ConfigMetaData &configMetadata,
                     NameValues &additionalLocks),
                    (override));

        MOCK_METHOD(Result, Unlock,
                    (const std::string &packageId,
                     const std::string &version),
                    (override));

        // Legacy/back-compat overloads exposed by IPackageImplDummy.
        MOCK_METHOD(Result, GetLockInfo,
                    (const std::string &packageId,
                     const std::string &version,
                     std::string &unpackedPath,
                     bool &locked),
                    (override));

        MOCK_METHOD(Result, GetList,
                    (std::string &packageList),
                    (override));

        MOCK_METHOD(Result, Lock,
                    (const std::string &packageId,
                     const std::string &version,
                     std::string &unpackedPath,
                     ConfigMetaData &configMetadata),
                    (override));

        MOCK_METHOD(Result, GetFileMetadata,
                    (const std::string &fileLocator,
                     std::string &packageId,
                     std::string &version,
                     ConfigMetaData &configMetadata),
                    (override));

        /**
         * Wires ON_CALL default actions so that any unprogrammed call falls back to
         * the concrete IPackageImplDummy implementation. Useful for tests that only
         * care about overriding one or two specific APIs but want the rest to keep
         * behaving like the legacy fake (e.g. Initialize still pre-populates the
         * "YouTube" config entry).
         *
         * Call this once during SetUp(), after constructing the mock, *before*
         * setting any EXPECT_CALL on the same method.
         */
        void DelegateToDefault()
        {
            using ::testing::_;
            using ::testing::Invoke;

            ON_CALL(*this, Initialize(_, _))
                .WillByDefault(Invoke([this](const std::string& cfg, ConfigMetadataArray& out) {
                    return this->IPackageImplDummy::Initialize(cfg, out);
                }));

            ON_CALL(*this, Install(_, _, _, _, _))
                .WillByDefault(Invoke([this](const std::string& id,
                                             const std::string& ver,
                                             const NameValues& meta,
                                             const std::string& loc,
                                             ConfigMetaData& cfg) {
                    return this->IPackageImplDummy::Install(id, ver, meta, loc, cfg);
                }));

            ON_CALL(*this, Uninstall(_))
                .WillByDefault(Invoke([this](const std::string& id) {
                    return this->IPackageImplDummy::Uninstall(id);
                }));

            ON_CALL(*this, Lock(_, _, _, _, _))
                .WillByDefault(Invoke([this](const std::string& id,
                                             const std::string& ver,
                                             std::string& path,
                                             ConfigMetaData& cfg,
                                             NameValues& locks) {
                    return this->IPackageImplDummy::Lock(id, ver, path, cfg, locks);
                }));

            ON_CALL(*this, Unlock(_, _))
                .WillByDefault(Invoke([this](const std::string& id, const std::string& ver) {
                    return this->IPackageImplDummy::Unlock(id, ver);
                }));

            ON_CALL(*this, GetLockInfo(_, _, _, _))
                .WillByDefault(Invoke([this](const std::string& id,
                                             const std::string& ver,
                                             std::string& path,
                                             bool& locked) {
                    return this->IPackageImplDummy::GetLockInfo(id, ver, path, locked);
                }));

            ON_CALL(*this, GetList(_))
                .WillByDefault(Invoke([this](std::string& list) {
                    return this->IPackageImplDummy::GetList(list);
                }));

            ON_CALL(*this, Lock(_, _, _, _))
                .WillByDefault(Invoke([this](const std::string& id,
                                             const std::string& ver,
                                             std::string& path,
                                             ConfigMetaData& cfg) {
                    return this->IPackageImplDummy::Lock(id, ver, path, cfg);
                }));

            ON_CALL(*this, GetFileMetadata(_, _, _, _))
                .WillByDefault(Invoke([this](const std::string& loc,
                                             std::string& id,
                                             std::string& ver,
                                             ConfigMetaData& cfg) {
                    return this->IPackageImplDummy::GetFileMetadata(loc, id, ver, cfg);
                }));
        }
    };

} // namespace packagemanager

