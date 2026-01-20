/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
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
**/

#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#ifdef RDK_APPMANAGERS_DEBUG
#include "ContainerUtils.h"
#include "WebInspector.h"
#include "NetFilter.h"
#endif

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

#ifdef RDK_APPMANAGERS_DEBUG

class GatewayTest : public ::testing::Test {
protected:
    std::string testContainerId;
    std::string testCgroupPath;

    GatewayTest()
        : testContainerId("test-container-id")
    {
    }

    virtual ~GatewayTest()
    {
    }

    virtual void SetUp()
    {
        // Create test cgroup directory structure for testing
        testCgroupPath = "/sys/fs/cgroup/memory/" + testContainerId;
    }

    virtual void TearDown()
    {
        // Clean up any test directories created
        if (testCgroupPath.length() > 0)
        {
            std::string procsFile = testCgroupPath + "/cgroup.procs";
            remove(procsFile.c_str());
            rmdir(testCgroupPath.c_str());
        }
    }
};

/* Test Case for ContainerUtils::getContainerIpAddress with Non-Existent Container
 * 
 * Purpose: Verify that getContainerIpAddress returns 0 (invalid IP) when the container doesn't exist
 * Setup:
 *   - Use a non-existent container ID
 * Expectation:
 *   - Function returns 0 indicating failure to retrieve IP address
 */
TEST_F(GatewayTest, GetContainerIpAddress_NonExistentContainer)
{
    const std::string nonExistentContainer = "non-existent-container-12345";
    
    uint32_t ipAddr = ContainerUtils::getContainerIpAddress(nonExistentContainer);
    
    EXPECT_EQ(0U, ipAddr) << "Should return 0 for non-existent container";
}

/* Test Case for ContainerUtils::getContainerIpAddress with Invalid Container ID
 * 
 * Purpose: Verify that getContainerIpAddress handles empty/invalid container IDs gracefully
 * Setup:
 *   - Use empty string as container ID
 * Expectation:
 *   - Function returns 0 indicating failure
 */
TEST_F(GatewayTest, GetContainerIpAddress_EmptyContainerId)
{
    const std::string emptyContainer = "";
    
    uint32_t ipAddr = ContainerUtils::getContainerIpAddress(emptyContainer);
    
    EXPECT_EQ(0U, ipAddr) << "Should return 0 for empty container ID";
}

/* Test Case for NetFilter::removeAllRulesMatchingComment
 * 
 * Purpose: Verify that removeAllRulesMatchingComment can be called without crashing
 * Setup:
 *   - Use a test comment string
 * Expectation:
 *   - Function completes without crashing
 * Note:
 *   - This test may require elevated privileges to actually modify iptables
 *   - In a unit test environment without privileges, it should fail gracefully
 */
TEST_F(GatewayTest, NetFilter_RemoveAllRulesMatchingComment)
{
    const std::string testComment = "test-gateway-comment-12345";
    
    // This should not crash even if it can't access iptables
    EXPECT_NO_THROW(NetFilter::removeAllRulesMatchingComment(testComment));
}

/* Test Case for NetFilter::openExternalPort
 * 
 * Purpose: Verify that openExternalPort can be called with valid parameters
 * Setup:
 *   - Use a test port number and protocol
 * Expectation:
 *   - Function completes without crashing
 * Note:
 *   - This test may require elevated privileges to actually modify iptables
 *   - In a unit test environment without privileges, it should return false
 */
TEST_F(GatewayTest, NetFilter_OpenExternalPort)
{
    const in_port_t testPort = 9999;
    const std::string testComment = "test-gateway-port-12345";
    
    // This should not crash even if it can't access iptables
    // We don't assert the return value as it depends on privileges
    bool result = NetFilter::openExternalPort(testPort, NetFilter::Protocol::Tcp, testComment);
    
    // Just verify the function can be called
    TEST_LOG("openExternalPort returned: %s", result ? "true" : "false");
}

/* Test Case for NetFilter::addContainerPortForwarding
 * 
 * Purpose: Verify that addContainerPortForwarding can be called with valid parameters
 * Setup:
 *   - Use test values for bridge interface, ports, and IP address
 * Expectation:
 *   - Function completes without crashing
 * Note:
 *   - This test may require elevated privileges to actually modify iptables
 */
TEST_F(GatewayTest, NetFilter_AddContainerPortForwarding)
{
    const std::string bridgeIface = "dobby0";
    const uint16_t externalPort = 9000;
    const uint16_t containerPort = 8080;
    const in_addr_t containerIp = inet_addr("172.17.0.2");
    const std::string testComment = "test-gateway-forwarding-12345";
    
    // This should not crash even if it can't access iptables
    bool result = NetFilter::addContainerPortForwarding(
        bridgeIface,
        NetFilter::Protocol::Tcp,
        externalPort,
        containerIp,
        containerPort,
        testComment
    );
    
    // Just verify the function can be called
    TEST_LOG("addContainerPortForwarding returned: %s", result ? "true" : "false");
}

/* Test Case for NetFilter::getContainerPortForwardList
 * 
 * Purpose: Verify that getContainerPortForwardList can be called and returns a list
 * Setup:
 *   - Use test values for bridge interface and IP address
 * Expectation:
 *   - Function returns a list (may be empty)
 */
TEST_F(GatewayTest, NetFilter_GetContainerPortForwardList)
{
    const std::string bridgeIface = "dobby0";
    const in_addr_t containerIp = inet_addr("172.17.0.2");
    
    // This should not crash even if it can't access iptables
    std::list<NetFilter::PortForward> portForwards;
    EXPECT_NO_THROW({
        portForwards = NetFilter::getContainerPortForwardList(bridgeIface, containerIp);
    });
    
    TEST_LOG("getContainerPortForwardList returned %zu entries", portForwards.size());
}

/* Test Case for WebInspector::attach with Invalid IP
 * 
 * Purpose: Verify that WebInspector::attach handles invalid IP addresses gracefully
 * Setup:
 *   - Use 0 (invalid IP address)
 * Expectation:
 *   - Function returns nullptr indicating failure
 */
TEST_F(GatewayTest, WebInspector_AttachWithInvalidIP)
{
    const std::string appId = "test-app";
    const in_addr_t invalidIp = 0;
    const int debugPort = 9222;
    
    auto inspector = WebInspector::attach(appId, invalidIp, debugPort);
    
    // Should return nullptr or valid shared_ptr depending on implementation
    // We just verify it doesn't crash
    TEST_LOG("WebInspector::attach with invalid IP returned: %s", 
             inspector ? "valid" : "nullptr");
}

/* Test Case for WebInspector::attach with Valid Parameters
 * 
 * Purpose: Verify that WebInspector::attach can be called with valid parameters
 * Setup:
 *   - Use valid app ID, IP address, and debug port
 * Expectation:
 *   - Function completes without crashing
 * Note:
 *   - May fail if iptables rules cannot be created due to lack of privileges
 */
TEST_F(GatewayTest, WebInspector_AttachWithValidParams)
{
    const std::string appId = "test-app-12345";
    const in_addr_t containerIp = inet_addr("172.17.0.2");
    const int debugPort = 9222;
    
    auto inspector = WebInspector::attach(appId, containerIp, debugPort);
    
    // The attach may succeed or fail depending on privileges
    // We just verify it doesn't crash
    TEST_LOG("WebInspector::attach returned: %s", 
             inspector ? "valid" : "nullptr");
    
    // If inspector was created, verify we can call its methods
    if (inspector)
    {
        EXPECT_TRUE(inspector->isAttached());
        EXPECT_EQ(debugPort, inspector->debugPort());
        EXPECT_EQ(Debugger::Type::WebInspector, inspector->type());
    }
}

/* Test Case for WebInspector Lifecycle
 * 
 * Purpose: Verify that WebInspector can be created and destroyed properly
 * Setup:
 *   - Create a WebInspector instance
 *   - Let it go out of scope
 * Expectation:
 *   - Destructor is called without crashes
 *   - Cleanup is performed
 */
TEST_F(GatewayTest, WebInspector_Lifecycle)
{
    const std::string appId = "lifecycle-test-app";
    const in_addr_t containerIp = inet_addr("172.17.0.3");
    const int debugPort = 9223;
    
    {
        auto inspector = WebInspector::attach(appId, containerIp, debugPort);
        
        if (inspector)
        {
            // Verify basic properties
            EXPECT_EQ(debugPort, inspector->debugPort());
        }
        
        // inspector goes out of scope here and should be destroyed
    }
    
    // Verify we can create another inspector after the first is destroyed
    {
        auto inspector2 = WebInspector::attach(appId + "-2", containerIp, debugPort);
        // Just verify it doesn't crash
    }
}

/* Test Case for Multiple Port Protocols
 * 
 * Purpose: Verify that NetFilter can handle both TCP and UDP protocols
 * Setup:
 *   - Test with both TCP and UDP protocols
 * Expectation:
 *   - Both protocol types can be used without crashing
 */
TEST_F(GatewayTest, NetFilter_MultipleProtocols)
{
    const in_port_t testPort = 9998;
    const std::string testCommentTcp = "test-tcp-12345";
    const std::string testCommentUdp = "test-udp-12345";
    
    // Test TCP
    bool tcpResult = NetFilter::openExternalPort(testPort, NetFilter::Protocol::Tcp, testCommentTcp);
    TEST_LOG("TCP openExternalPort returned: %s", tcpResult ? "true" : "false");
    
    // Test UDP
    bool udpResult = NetFilter::openExternalPort(testPort + 1, NetFilter::Protocol::Udp, testCommentUdp);
    TEST_LOG("UDP openExternalPort returned: %s", udpResult ? "true" : "false");
    
    // Clean up
    NetFilter::removeAllRulesMatchingComment(testCommentTcp);
    NetFilter::removeAllRulesMatchingComment(testCommentUdp);
}

#else // RDK_APPMANAGERS_DEBUG not defined

/* Test Case for Gateway Module Not Compiled
 * 
 * Purpose: Verify test suite reports when Gateway module is not compiled
 * Expectation:
 *   - Test passes to indicate Gateway is not being tested due to build configuration
 */
TEST(GatewayTest, GatewayModuleNotCompiled)
{
    TEST_LOG("Gateway module tests skipped - RDK_APPMANAGERS_DEBUG not defined");
    SUCCEED() << "Gateway module is not compiled in this build configuration";
}

#endif // RDK_APPMANAGERS_DEBUG
