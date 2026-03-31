/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

/**
 * @file Gateway_ContainerUtilsTests.cpp
 *
 * L0 tests for RuntimeManager/Gateway/ContainerUtils:
 *   - ContainerUtils::getContainerIpAddress() with an unknown/non-existent container
 *   - ContainerUtils::Namespace enum values are accessible
 */

#include <cstdint>
#include <iostream>
#include <string>
#include <netinet/in.h>

#include "Gateway/ContainerUtils.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// ContainerUtils tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_ContainerUtils_GetContainerIpAddress_UnknownContainerReturnsZero
 *
 * Verifies that getContainerIpAddress() returns 0 (INADDR_NONE or INADDR_ANY)
 * for a container ID that does not exist on the system.
 * The function reads /sys/fs/cgroup/memory/<containerId>/cgroup.procs, which
 * will not exist for a fake ID.
 */
uint32_t Test_ContainerUtils_GetContainerIpAddress_UnknownContainerReturnsZero()
{
    L0Test::TestResult tr;

    const in_addr_t ip =
        WPEFramework::Plugin::ContainerUtils::getContainerIpAddress(
            "ralf_l0test_fake_container_id_xyz");

    // Should fail to find the cgroup path → returns 0
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(ip), 0u,
                        "getContainerIpAddress() returns 0 for unknown container");

    return tr.failures;
}

/* Test_ContainerUtils_GetContainerIpAddress_EmptyContainerIdReturnsZero
 *
 * Verifies that getContainerIpAddress() returns 0 for an empty container ID.
 */
uint32_t Test_ContainerUtils_GetContainerIpAddress_EmptyContainerIdReturnsZero()
{
    L0Test::TestResult tr;

    const in_addr_t ip =
        WPEFramework::Plugin::ContainerUtils::getContainerIpAddress("");

    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(ip), 0u,
                        "getContainerIpAddress() returns 0 for empty container ID");

    return tr.failures;
}

/* Test_ContainerUtils_GetContainerIpAddress_MultipleCallsDoNotCrash
 *
 * Verifies that multiple successive calls to getContainerIpAddress() with
 * invalid IDs do not crash or leave resources open.
 */
uint32_t Test_ContainerUtils_GetContainerIpAddress_MultipleCallsDoNotCrash()
{
    L0Test::TestResult tr;

    for (int i = 0; i < 5; ++i) {
        const std::string fakeId = "l0test_fake_id_" + std::to_string(i);
        const in_addr_t ip =
            WPEFramework::Plugin::ContainerUtils::getContainerIpAddress(fakeId);
        L0Test::ExpectEqU32(tr, static_cast<uint32_t>(ip), 0u,
                            "getContainerIpAddress() returns 0 on repeated calls with fake IDs");
    }

    return tr.failures;
}

/* Test_ContainerUtils_NamespaceEnumValues
 *
 * Verifies that the Namespace enum values have the expected bit-flag values.
 */
uint32_t Test_ContainerUtils_NamespaceEnumValues()
{
    L0Test::TestResult tr;

    using NS = WPEFramework::Plugin::ContainerUtils::Namespace;

    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(NS::NetworkNamespace), 0x01u,
                        "NetworkNamespace == 0x01");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(NS::MountNamespace), 0x02u,
                        "MountNamespace == 0x02");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(NS::IpcNamespace), 0x04u,
                        "IpcNamespace == 0x04");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(NS::PidNamespace), 0x08u,
                        "PidNamespace == 0x08");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(NS::UserNamespace), 0x10u,
                        "UserNamespace == 0x10");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(NS::UtsNamespace), 0x20u,
                        "UtsNamespace == 0x20");

    return tr.failures;
}

/* Test_ContainerUtils_GetContainerIpAddress_VeryLongContainerIdDoesNotCrash
 *
 * Verifies that a very long container ID (e.g., 1000 characters) is handled
 * gracefully without buffer overflow or crash.
 */
uint32_t Test_ContainerUtils_GetContainerIpAddress_VeryLongContainerIdDoesNotCrash()
{
    L0Test::TestResult tr;

    const std::string longId(500, 'a');
    const in_addr_t ip =
        WPEFramework::Plugin::ContainerUtils::getContainerIpAddress(longId);

    // Should still return 0 (no such container)
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(ip), 0u,
                        "getContainerIpAddress() with very long ID returns 0 and does not crash");

    return tr.failures;
}
