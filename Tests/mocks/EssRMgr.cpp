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

#include "essos-resmgr.h"
#include <gtest/gtest.h>

EssRMgrImpl* EssRMgrApi::impl = nullptr;

EssRMgrApi::EssRMgrApi() {}

void EssRMgrApi::setImpl(EssRMgrImpl* newImpl) {
    EXPECT_TRUE((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

bool EssRMgrApi::EssRMgrInit() {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrInit();
}

void EssRMgrApi::EssRMgrTerm() {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrTerm();
}

EssRMgr* EssRMgrApi::EssRMgrCreate() {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrCreate();
}

void EssRMgrApi::EssRMgrDestroy(EssRMgr* rm) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrDestroy(rm);
}

bool EssRMgrApi::EssRMgrGetPolicyPriorityTie(EssRMgr* rm) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrGetPolicyPriorityTie(rm);
}

bool EssRMgrApi::EssRMgrGetAVState(EssRMgr* rm, int* state) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrGetAVState(rm, state);
}

int EssRMgrApi::EssRMgrResourceGetCount(EssRMgr* rm, int type) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrResourceGetCount(rm, type);
}

bool EssRMgrApi::EssRMgrResourceGetOwner(EssRMgr* rm, int type, int id, int* client, int* priority) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrResourceGetOwner(rm, type, id, client, priority);
}

bool EssRMgrApi::EssRMgrResourceGetCaps(EssRMgr* rm, int type, int id, EssRMgrCaps* caps) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrResourceGetCaps(rm, type, id, caps);
}

bool EssRMgrApi::EssRMgrResourceGetState(EssRMgr* rm, int type, int id, int* state) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrResourceGetState(rm, type, id, state);
}

bool EssRMgrApi::EssRMgrResourceSetState(EssRMgr* rm, int type, int id, int state) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrResourceSetState(rm, type, id, state);
}

bool EssRMgrApi::EssRMgrRequestResource(EssRMgr* rm, int type, EssRMgrRequest* req) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrRequestResource(rm, type, req);
}

void EssRMgrApi::EssRMgrReleaseResource(EssRMgr* rm, int type, int id) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrReleaseResource(rm, type, id);
}

bool EssRMgrApi::EssRMgrRequestSetPriority(EssRMgr* rm, int type, int requestId, int priority) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrRequestSetPriority(rm, type, requestId, priority);
}

bool EssRMgrApi::EssRMgrRequestSetUsage(EssRMgr* rm, int type, int requestId, EssRMgrUsage* usage) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrRequestSetUsage(rm, type, requestId, usage);
}

void EssRMgrApi::EssRMgrRequestCancel(EssRMgr* rm, int type, int requestId) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrRequestCancel(rm, type, requestId);
}

bool EssRMgrApi::EssRMgrAddToBlackList(EssRMgr* rm, const char* appId) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrAddToBlackList(rm, appId);
}

bool EssRMgrApi::EssRMgrRemoveFromBlackList(EssRMgr* rm, const char* appId) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrRemoveFromBlackList(rm, appId);
}

void EssRMgrApi::EssRMgrDumpState(EssRMgr* rm) {
    EXPECT_NE(impl, nullptr);
    return impl->EssRMgrDumpState(rm);
}


bool (*EssRMgrInit)() = &EssRMgrApi::EssRMgrInit;
void (*EssRMgrTerm)() = &EssRMgrApi::EssRMgrTerm;
EssRMgr* (*EssRMgrCreate)() = &EssRMgrApi::EssRMgrCreate;
void (*EssRMgrDestroy)( EssRMgr *rm ) = &EssRMgrApi::EssRMgrDestroy;
bool (*EssRMgrGetPolicyPriorityTie)( EssRMgr *rm ) = &EssRMgrApi::EssRMgrGetPolicyPriorityTie;
bool (*EssRMgrGetAVState)( EssRMgr *rm, int *state ) = &EssRMgrApi::EssRMgrGetAVState;
int (*EssRMgrResourceGetCount)( EssRMgr *rm, int type ) = &EssRMgrApi::EssRMgrResourceGetCount;
bool (*EssRMgrResourceGetOwner)( EssRMgr *rm, int type, int id, int *client, int *priority ) = &EssRMgrApi::EssRMgrResourceGetOwner;
bool (*EssRMgrResourceGetCaps)( EssRMgr *rm, int type, int id, EssRMgrCaps *caps ) = &EssRMgrApi::EssRMgrResourceGetCaps;
bool (*EssRMgrResourceGetState)( EssRMgr *rm, int type, int id, int *state ) = &EssRMgrApi::EssRMgrResourceGetState;
bool (*EssRMgrResourceSetState)( EssRMgr *rm, int type, int id, int state ) = &EssRMgrApi::EssRMgrResourceSetState;
bool (*EssRMgrRequestResource)( EssRMgr *rm, int type, EssRMgrRequest *req ) = &EssRMgrApi::EssRMgrRequestResource;
void (*EssRMgrReleaseResource)( EssRMgr *rm, int type, int id ) = &EssRMgrApi::EssRMgrReleaseResource;
bool (*EssRMgrRequestSetPriority)( EssRMgr *rm, int type, int requestId, int priority ) = &EssRMgrApi::EssRMgrRequestSetPriority;
bool (*EssRMgrRequestSetUsage)( EssRMgr *rm, int type, int requestId, EssRMgrUsage *usage ) = &EssRMgrApi::EssRMgrRequestSetUsage;
void (*EssRMgrRequestCancel)( EssRMgr *rm, int type, int requestId ) = &EssRMgrApi::EssRMgrRequestCancel;
bool (*EssRMgrAddToBlackList)( EssRMgr *rm, const char *appId ) = &EssRMgrApi::EssRMgrAddToBlackList;
bool (*EssRMgrRemoveFromBlackList)( EssRMgr *rm, const char *appId ) = &EssRMgrApi::EssRMgrRemoveFromBlackList;
void (*EssRMgrDumpState)( EssRMgr *rm ) = &EssRMgrApi::EssRMgrDumpState;