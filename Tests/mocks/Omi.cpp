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
#include "OmiMock.h"
#include <gmock/gmock.h>

namespace omi {
omi::IOmiProxy* OmiProxy::impl = nullptr;
OmiProxy::OmiProxy() {}

void OmiProxy::setImpl(omi::IOmiProxy* newImpl)
{
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

OmiProxy::OmiProxy(const std::shared_ptr<AI_IPC::IIpcService>&,
    const std::string&,
    const std::string&)
    : mIpcService(nullptr)
{
}

bool OmiProxy::mountCryptedBundle(const std::string& id,
    const std::string& rootfs_file_path,
    const std::string& config_json_path,
    std::string& bundlePath)
{
    EXPECT_NE(impl, nullptr);
    return impl->mountCryptedBundle(id, rootfs_file_path, config_json_path, bundlePath);
}

bool OmiProxy::umountCryptedBundle(const std::string& id)
{
    EXPECT_NE(impl, nullptr);
    return impl->umountCryptedBundle(id);
}

long unsigned OmiProxy::registerListener(const omi::IOmiProxy::OmiErrorListener& listener, const void* cbParams)
{
    EXPECT_NE(impl, nullptr);
    return impl->registerListener(listener, cbParams);
}

void OmiProxy::unregisterListener(long unsigned tag)
{
    EXPECT_NE(impl, nullptr);
    impl->unregisterListener(tag);
}
} // namespace omi
