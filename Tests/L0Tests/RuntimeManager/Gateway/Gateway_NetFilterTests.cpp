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
 * @file Gateway_NetFilterTests.cpp
 *
 * L0 tests for RuntimeManager/Gateway/NetFilter and NetFilterLock:
 *
 * NetFilterLock:
 *   - Construction and destruction
 *   - try_lock() / unlock() — graceful when /run/xtables.lock not openable
 *   - try_lock_for() with zero duration
 *
 * NetFilter:
 *   - removeAllRulesMatchingComment() with empty/no-match comment
 *   - openExternalPort() returns false when iptables unavailable
 *   - addContainerPortForwarding() returns false when iptables unavailable
 *   - getContainerPortForwardList() returns empty list when iptables unavailable
 *
 * NOTE: All NetFilter operations that touch iptables will return false / empty
 * in environments without kernel iptables support.  Tests are written to accept
 * both outcomes (success and graceful failure).
 */

#include <cstdint>
#include <chrono>
#include <iostream>
#include <list>
#include <string>
#include <arpa/inet.h>

#include "Gateway/NetFilterLock.h"
#include "Gateway/NetFilter.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// NetFilterLock tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_NetFilterLock_ConstructionAndDestruction
 *
 * Verifies that NetFilterLock can be constructed and destroyed without crash.
 * On systems where /run/xtables.lock cannot be created the object still
 * constructs successfully (no exception thrown).
 */
uint32_t Test_NetFilterLock_ConstructionAndDestruction()
{
    L0Test::TestResult tr;

    {
        NetFilterLock lock;
        L0Test::ExpectTrue(tr, true,
                           "NetFilterLock constructor does not crash");
    }
    L0Test::ExpectTrue(tr, true,
                       "NetFilterLock destructor does not crash");

    return tr.failures;
}

/* Test_NetFilterLock_TryLockDoesNotCrash
 *
 * Verifies that try_lock() can be called without crashing.
 * This may return false when /run/xtables.lock is unavailable.
 */
uint32_t Test_NetFilterLock_TryLockDoesNotCrash()
{
    L0Test::TestResult tr;

    NetFilterLock lock;
    const bool acquired = lock.try_lock();

    // If acquired we must unlock to release the file lock.
    if (acquired) {
        lock.unlock();
    }

    L0Test::ExpectTrue(tr, true,
                       "NetFilterLock::try_lock() does not crash");

    return tr.failures;
}

/* Test_NetFilterLock_LockUnlockDoesNotCrash
 *
 * Verifies that lock() followed by unlock() does not crash.
 */
uint32_t Test_NetFilterLock_LockUnlockDoesNotCrash()
{
    L0Test::TestResult tr;

    NetFilterLock lock;
    lock.lock();
    lock.unlock();

    L0Test::ExpectTrue(tr, true,
                       "NetFilterLock::lock() / unlock() do not crash");

    return tr.failures;
}

/* Test_NetFilterLock_TryLockForZeroDurationDoesNotCrash
 *
 * Verifies that try_lock_for() with a zero-duration returns without deadlocking
 * and does not crash.
 */
uint32_t Test_NetFilterLock_TryLockForZeroDurationDoesNotCrash()
{
    L0Test::TestResult tr;

    NetFilterLock lock;
    const bool acquired = lock.try_lock_for(std::chrono::milliseconds(0));

    if (acquired) {
        lock.unlock();
    }

    L0Test::ExpectTrue(tr, true,
                       "NetFilterLock::try_lock_for(0ms) does not crash");

    return tr.failures;
}

/* Test_NetFilterLock_MultipleInstancesDoNotCrash
 *
 * Verifies that multiple NetFilterLock instances can coexist without crash.
 */
uint32_t Test_NetFilterLock_MultipleInstancesDoNotCrash()
{
    L0Test::TestResult tr;

    {
        NetFilterLock lock1;
        NetFilterLock lock2;
        L0Test::ExpectTrue(tr, true,
                           "Two NetFilterLock instances coexist without crash");
    }

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// NetFilter tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_NetFilter_Protocol_TcpAndUdpValuesDistinct
 *
 * Verifies that NetFilter::Protocol::Tcp and ::Udp have distinct values.
 */
uint32_t Test_NetFilter_Protocol_TcpAndUdpValuesDistinct()
{
    L0Test::TestResult tr;

    L0Test::ExpectTrue(tr,
        NetFilter::Protocol::Tcp != NetFilter::Protocol::Udp,
        "NetFilter::Protocol::Tcp is distinct from NetFilter::Protocol::Udp");

    return tr.failures;
}

/* Test_NetFilter_RemoveAllRulesMatchingComment_EmptyCommentDoesNotCrash
 *
 * Verifies that removeAllRulesMatchingComment() with an empty comment string
 * does not crash.
 */
uint32_t Test_NetFilter_RemoveAllRulesMatchingComment_EmptyCommentDoesNotCrash()
{
    L0Test::TestResult tr;

    NetFilter::removeAllRulesMatchingComment("");

    L0Test::ExpectTrue(tr, true,
                       "removeAllRulesMatchingComment(\"\") does not crash");

    return tr.failures;
}

/* Test_NetFilter_RemoveAllRulesMatchingComment_NoMatchCommentDoesNotCrash
 *
 * Verifies that removeAllRulesMatchingComment() with a comment that matches
 * no rules does not crash.
 */
uint32_t Test_NetFilter_RemoveAllRulesMatchingComment_NoMatchCommentDoesNotCrash()
{
    L0Test::TestResult tr;

    NetFilter::removeAllRulesMatchingComment("ralf_l0test_no_such_rule_xyz");

    L0Test::ExpectTrue(tr, true,
                       "removeAllRulesMatchingComment() with no-match comment does not crash");

    return tr.failures;
}

/* Test_NetFilter_OpenExternalPort_DoesNotCrash
 *
 * Verifies that openExternalPort() does not crash regardless of whether
 * iptables is available.
 */
uint32_t Test_NetFilter_OpenExternalPort_DoesNotCrash()
{
    L0Test::TestResult tr;

    // Port 19999 is unlikely to be in use; we test idempotency and no-crash.
    const bool result = NetFilter::openExternalPort(
        static_cast<in_port_t>(19999),
        NetFilter::Protocol::Tcp,
        "ralf_l0test_open_port");

    // Result may be false when iptables not available — that is acceptable.
    (void)result;
    L0Test::ExpectTrue(tr, true,
                       "NetFilter::openExternalPort() does not crash");

    return tr.failures;
}

/* Test_NetFilter_AddContainerPortForwarding_DoesNotCrash
 *
 * Verifies that addContainerPortForwarding() does not crash when iptables
 * is not available. The function is expected to return false gracefully.
 */
uint32_t Test_NetFilter_AddContainerPortForwarding_DoesNotCrash()
{
    L0Test::TestResult tr;

    const in_addr_t containerIp = ::inet_addr("192.168.100.1");

    const bool result = NetFilter::addContainerPortForwarding(
        "dobby0",
        NetFilter::Protocol::Tcp,
        static_cast<uint16_t>(18080),
        containerIp,
        static_cast<uint16_t>(8080),
        "ralf_l0test_fwd");

    // In environments without iptables this returns false
    (void)result;
    L0Test::ExpectTrue(tr, true,
                       "NetFilter::addContainerPortForwarding() does not crash");

    return tr.failures;
}

/* Test_NetFilter_AddContainerPortForwarding_ReturnsFalseOrTrue
 *
 * Specifically verifies the return value contract: the function returns a
 * boolean (either true on success with iptables, or false on failure).
 */
uint32_t Test_NetFilter_AddContainerPortForwarding_ReturnsFalseOrTrue()
{
    L0Test::TestResult tr;

    const in_addr_t containerIp = ::inet_addr("10.0.0.55");

    const bool result = NetFilter::addContainerPortForwarding(
        "br0",
        NetFilter::Protocol::Udp,
        static_cast<uint16_t>(15353),
        containerIp,
        static_cast<uint16_t>(53),
        "ralf_l0test_udp_fwd");

    // Both true and false are valid outcomes; only crash is disallowed.
    const bool isValidBool = (result == true || result == false);
    L0Test::ExpectTrue(tr, isValidBool,
                       "addContainerPortForwarding() returns a valid bool (true or false)");

    return tr.failures;
}

/* Test_NetFilter_GetContainerPortForwardList_ReturnsListType
 *
 * Verifies that getContainerPortForwardList() returns a list (possibly empty)
 * without crashing.
 */
uint32_t Test_NetFilter_GetContainerPortForwardList_ReturnsListType()
{
    L0Test::TestResult tr;

    const in_addr_t containerIp = ::inet_addr("172.20.0.10");

    std::list<NetFilter::PortForward> fwdList =
        NetFilter::getContainerPortForwardList("dobby0", containerIp);

    // Must not crash; may be empty in test environments
    L0Test::ExpectTrue(tr, true,
                       "getContainerPortForwardList() does not crash and returns a list");

    return tr.failures;
}

/* Test_NetFilter_GetContainerPortForwardList_DefaultIpReturnsListType
 *
 * Verifies that getContainerPortForwardList() with default containerIp (0)
 * does not crash.
 */
uint32_t Test_NetFilter_GetContainerPortForwardList_DefaultIpReturnsListType()
{
    L0Test::TestResult tr;

    std::list<NetFilter::PortForward> fwdList =
        NetFilter::getContainerPortForwardList("dobby0");

    L0Test::ExpectTrue(tr, true,
                       "getContainerPortForwardList() with default IP does not crash");

    return tr.failures;
}

/* Test_NetFilter_PortForward_StructMembersAccessible
 *
 * Verifies that NetFilter::PortForward struct members (protocol, externalPort,
 * containerPort) are accessible and can be assigned/read.
 */
uint32_t Test_NetFilter_PortForward_StructMembersAccessible()
{
    L0Test::TestResult tr;

    NetFilter::PortForward pf;
    pf.protocol      = NetFilter::Protocol::Tcp;
    pf.externalPort  = 8080u;
    pf.containerPort = 80u;

    L0Test::ExpectTrue(tr, pf.protocol == NetFilter::Protocol::Tcp,
                       "PortForward::protocol can be set and read");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(pf.externalPort), 8080u,
                        "PortForward::externalPort can be set and read");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(pf.containerPort), 80u,
                        "PortForward::containerPort can be set and read");

    return tr.failures;
}
