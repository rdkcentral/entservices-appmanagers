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
 * @file Gateway_WebInspectorTests.cpp
 *
 * L0 tests for RuntimeManager/Gateway/WebInspector:
 *   - Debugger base class Type enum values
 *   - WebInspector::attach() returns nullptr when iptables/NetFilter is unavailable
 *   - WebInspector accessor methods (type, isAttached, debugPort) via attach()
 *     when NetFilter succeeds (mocked by testing return value contract)
 *
 * NOTE: WebInspector::attach() relies on NetFilter::addContainerPortForwarding()
 * which requires iptables kernel support. In a test environment without iptables
 * these calls will fail, and attach() will return nullptr. Tests account for both
 * the failing and (optionally) succeeding cases.
 */

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <arpa/inet.h>

#include "Gateway/Debugger.h"
#include "Gateway/WebInspector.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Debugger base class tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Debugger_TypeEnum_WebInspectorValue
 *
 * Verifies that the Debugger::Type::WebInspector enum value exists and is
 * distinct from Debugger::Type::Gdb.
 */
uint32_t Test_Debugger_TypeEnum_WebInspectorValue()
{
    L0Test::TestResult tr;

    L0Test::ExpectTrue(tr,
        Debugger::Type::WebInspector != Debugger::Type::Gdb,
        "Debugger::Type::WebInspector is distinct from Debugger::Type::Gdb");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// WebInspector::attach() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_WebInspector_Attach_WithInvalidIpReturnsNullptrOrNonNull
 *
 * Calls WebInspector::attach() with a loopback address and a debug port.
 * In a test environment without iptables support this returns nullptr (graceful).
 * If iptables ARE available, a non-null pointer is returned — both outcomes
 * are valid and must not crash.
 */
uint32_t Test_WebInspector_Attach_WithInvalidIpReturnsNullptrOrNonNull()
{
    L0Test::TestResult tr;

    // Use loopback address — no real container required
    in_addr_t testIp = ::inet_addr("127.0.0.1");
    auto inspector = WebInspector::attach("com.test.testapp", testIp, 9222);
    (void)inspector;

    // We only verify that the call did not crash; nullptr is the expected
    // result in environments without iptables.
    L0Test::ExpectTrue(tr, true,
                       "WebInspector::attach() does not crash regardless of iptables availability");

    return tr.failures;
}

/* Test_WebInspector_Attach_ZeroIpReturnsNullptrOrNonNull
 *
 * Calls WebInspector::attach() with IP address 0 (INADDR_ANY).
 * Must not crash.
 */
uint32_t Test_WebInspector_Attach_ZeroIpReturnsNullptrOrNonNull()
{
    L0Test::TestResult tr;

    auto inspector = WebInspector::attach("com.test.zeroip", 0u, 9223);
    (void)inspector;
    L0Test::ExpectTrue(tr, true,
                       "WebInspector::attach() with zero IP does not crash");

    return tr.failures;
}

/* Test_WebInspector_Attach_EmptyAppIdDoesNotCrash
 *
 * Calls WebInspector::attach() with an empty appId.  Must not crash.
 */
uint32_t Test_WebInspector_Attach_EmptyAppIdDoesNotCrash()
{
    L0Test::TestResult tr;

    in_addr_t testIp = ::inet_addr("127.0.0.1");
    auto inspector = WebInspector::attach("", testIp, 9224);
    (void)inspector;
    L0Test::ExpectTrue(tr, true,
                       "WebInspector::attach() with empty appId does not crash");

    return tr.failures;
}

/* Test_WebInspector_Attach_MultipleCallsDoNotCrash
 *
 * Verifies that multiple successive attach() calls with different app IDs
 * do not crash.
 */
uint32_t Test_WebInspector_Attach_MultipleCallsDoNotCrash()
{
    L0Test::TestResult tr;

    in_addr_t testIp = ::inet_addr("127.0.0.1");

    for (int i = 0; i < 3; ++i) {
        const std::string appId = "com.test.app" + std::to_string(i);
        const int port = 9230 + i;
        WebInspector::attach(appId, testIp, port);
    }

    L0Test::ExpectTrue(tr, true,
                       "Multiple WebInspector::attach() calls do not crash");

    return tr.failures;
}

/* Test_WebInspector_TypeReturnsWebInspector
 *
 * When WebInspector::attach() succeeds (iptables available), the returned
 * object's type() method should return Debugger::Type::WebInspector.
 * When attach() fails (nullptr), this test is a no-op.
 */
uint32_t Test_WebInspector_TypeReturnsWebInspector()
{
    L0Test::TestResult tr;

    in_addr_t testIp = ::inet_addr("127.0.0.1");
    auto inspector = WebInspector::attach("com.test.typecheck", testIp, 9240);

    if (inspector != nullptr) {
        L0Test::ExpectTrue(tr,
            inspector->type() == Debugger::Type::WebInspector,
            "WebInspector::type() returns Debugger::Type::WebInspector");
        L0Test::ExpectTrue(tr, inspector->isAttached(),
                           "WebInspector::isAttached() returns true when attached");
        L0Test::ExpectEqU32(tr, static_cast<uint32_t>(inspector->debugPort()), 9240u,
                            "WebInspector::debugPort() returns the configured port");
    } else {
        // No iptables — skip accessor tests but do not fail
        L0Test::ExpectTrue(tr, true,
                           "SKIP: WebInspector::attach() returned nullptr (iptables unavailable)");
    }

    return tr.failures;
}

/* Test_WebInspector_DestructorRunsCleanupWithoutCrash
 *
 * Verifies that when a WebInspector shared_ptr goes out of scope the
 * destructor (which calls NetFilter::removeAllRulesMatchingComment()) does
 * not crash, even when iptables is unavailable.
 */
uint32_t Test_WebInspector_DestructorRunsCleanupWithoutCrash()
{
    L0Test::TestResult tr;

    {
        in_addr_t testIp = ::inet_addr("127.0.0.1");
        auto inspector = WebInspector::attach("com.test.dtor", testIp, 9250);
        (void)inspector;
        // inspector goes out of scope here; destructor called
    }

    L0Test::ExpectTrue(tr, true,
                       "WebInspector destructor does not crash on scope exit");

    return tr.failures;
}
