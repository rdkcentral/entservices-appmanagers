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

#include <gmock/gmock.h>
#include "essos-resmgr.h"

class EssRMgrMock : public EssRMgrImpl {
public:
    EssRMgrMock()
        : EssRMgrImpl()
    {
	char fakeMemory[64] = {}; // 64 bytes is arbitrary, depends on actual size
        EssRMgr* EssRMgrObj = reinterpret_cast<EssRMgr*>(fakeMemory);
        ON_CALL(*this, EssRMgrCreate())
            .WillByDefault(::testing::Return(EssRMgrObj));
    
    }
    virtual ~EssRMgrMock() = default;

    MOCK_METHOD(bool, EssRMgrInit, (), (override));
    MOCK_METHOD(void, EssRMgrTerm, (), (override));
    MOCK_METHOD(EssRMgr*, EssRMgrCreate, (), (override));
    MOCK_METHOD(void, EssRMgrDestroy, (EssRMgr * rm), (override));
    MOCK_METHOD(bool, EssRMgrGetPolicyPriorityTie, (EssRMgr * rm), (override));
    MOCK_METHOD(bool, EssRMgrGetAVState, (EssRMgr * rm, int* state), (override));
    MOCK_METHOD(int, EssRMgrResourceGetCount, (EssRMgr * rm, int type), (override));
    MOCK_METHOD(bool, EssRMgrResourceGetOwner, (EssRMgr * rm, int type, int id, int* client, int* priority), (override));
    MOCK_METHOD(bool, EssRMgrResourceGetCaps, (EssRMgr * rm, int type, int id, EssRMgrCaps* caps), (override));
    MOCK_METHOD(bool, EssRMgrResourceGetState, (EssRMgr * rm, int type, int id, int* state), (override));
    MOCK_METHOD(bool, EssRMgrResourceSetState, (EssRMgr * rm, int type, int id, int state), (override));
    MOCK_METHOD(bool, EssRMgrRequestResource, (EssRMgr * rm, int type, EssRMgrRequest* req), (override));
    MOCK_METHOD(void, EssRMgrReleaseResource, (EssRMgr * rm, int type, int id), (override));
    MOCK_METHOD(bool, EssRMgrRequestSetPriority, (EssRMgr * rm, int type, int requestId, int priority), (override));
    MOCK_METHOD(bool, EssRMgrRequestSetUsage, (EssRMgr * rm, int type, int requestId, EssRMgrUsage* usage), (override));
    MOCK_METHOD(void, EssRMgrRequestCancel, (EssRMgr * rm, int type, int requestId), (override));
    MOCK_METHOD(bool, EssRMgrAddToBlackList, (EssRMgr * rm, const char* appId), (override));
    MOCK_METHOD(bool, EssRMgrRemoveFromBlackList, (EssRMgr * rm, const char* appId), (override));
    MOCK_METHOD(void, EssRMgrDumpState, (EssRMgr * rm), (override));
};
