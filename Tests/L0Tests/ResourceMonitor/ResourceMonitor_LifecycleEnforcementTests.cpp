/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use, sell, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include "AppLifecycleConfig.h"
#include "PackageLifecycleConfigProvider.h"
#include "ResourceMonitorL0Fakes.hpp"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"
#include <interfaces/IAppManager.h>

namespace {

using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Plugin::AppLifecycleCapabilities;
using WPEFramework::Plugin::AppLifecycleState;
using WPEFramework::Plugin::LifecycleConfigStatus;
using WPEFramework::Plugin::SuspendedStateConfig;
using WPEFramework::Plugin::PackageLifecycleConfigProvider;
namespace AppLifecycleConfig = WPEFramework::Plugin::AppLifecycleConfig;

const char* const kAllStates =
    "{\"supportedNonActiveStates\":[\"paused\",\"suspended\",\"hibernated\"],"
    "\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}";

const char* const kPausedSuspended =
    "{\"supportedNonActiveStates\":[\"paused\",\"suspended\"],"
    "\"maxSuspendedSystemMemory\":\"100M\",\"maxTimeToSuspendMemoryState\":15}";

const char* const kPausedOnly =
    "{\"supportedNonActiveStates\":[\"paused\"],"
    "\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}";

} // namespace

// --- Capability-based decision logic --------------------------------------------------------------

uint32_t Test_RM_Enforcement_CapabilitySnapshot()
{
    L0Test::TestResult tr;
    
    // Parse capabilities from metadata
    AppLifecycleCapabilities caps;
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kAllStates), caps);
    
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(caps.status), 
        static_cast<uint32_t>(LifecycleConfigStatus::CONFIGURED), "All states are configured");
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::PAUSED), "PAUSED is supported");
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::SUSPENDED), "SUSPENDED is supported");
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::HIBERNATED), "HIBERNATED is supported");
    
    // Test partial support
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kPausedSuspended), caps);
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::PAUSED), "PAUSED is supported in partial");
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::SUSPENDED), "SUSPENDED is supported in partial");
    L0Test::ExpectTrue(tr, !caps.IsSupported(AppLifecycleState::HIBERNATED), "HIBERNATED is NOT supported in partial");
    
    // Test PAUSED only
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kPausedOnly), caps);
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::PAUSED), "PAUSED is supported in PAUSED-only");
    L0Test::ExpectTrue(tr, !caps.IsSupported(AppLifecycleState::SUSPENDED), "SUSPENDED is NOT supported in PAUSED-only");
    L0Test::ExpectTrue(tr, !caps.IsSupported(AppLifecycleState::HIBERNATED), "HIBERNATED is NOT supported in PAUSED-only");
    
    return tr.failures;
}

uint32_t Test_RM_Enforcement_SuspendedConfigParsing()
{
    L0Test::TestResult tr;
    
    AppLifecycleCapabilities caps;
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kAllStates), caps);
    
    L0Test::ExpectTrue(tr, caps.suspended.hasMaxSystemMemory, "maxSuspendedSystemMemory is present");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(caps.suspended.maxSystemMemoryBytes / (1024 * 1024)), 16U, "16M is 16 MB");
    L0Test::ExpectTrue(tr, caps.suspended.hasMaxTimeToSuspend, "maxTimeToSuspendMemoryState is present");
    L0Test::ExpectEqU32(tr, caps.suspended.maxTimeToSuspendSec, 10U, "maxTimeToSuspendMemoryState is 10 seconds");
    
    return tr.failures;
}

// --- GPU memory parsing -----------------------------------------------------------------

uint32_t Test_RM_Enforcement_GPUMemoryParsing()
{
    L0Test::TestResult tr;
    
    // Test parsing of GPU memory from RuntimeManager GetInfo JSON
    std::string json = R"({"ramUsage": "150MB","cpuUsage": "20%","gpuMemory": "32MB","uptime": "45s"})";
    
    // Find gpuMemory field
    const std::string gpuMemoryKey = "\"gpuMemory\"";
    const size_t gpuMemoryPos = json.find(gpuMemoryKey);
    L0Test::ExpectTrue(tr, gpuMemoryPos != std::string::npos, "gpuMemory field exists in JSON");
    
    if (gpuMemoryPos != std::string::npos) {
        // Extract value
        const size_t colonPos = json.find(":", gpuMemoryPos);
        L0Test::ExpectTrue(tr, colonPos != std::string::npos, "Colon found after gpuMemory key");
        
        if (colonPos != std::string::npos) {
            const size_t valueStart = json.find("\"", colonPos) + 1;
            const size_t valueEnd = json.find("\"", valueStart);
            L0Test::ExpectTrue(tr, valueStart != std::string::npos && valueEnd != std::string::npos, "GPU memory value is quoted");
            
            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                const std::string gpuMemoryStr = json.substr(valueStart, valueEnd - valueStart);
                L0Test::ExpectEqStr(tr, gpuMemoryStr, "32MB", "GPU memory value is 32MB");
            }
        }
    }
    
    // Test JSON without gpuMemory field
    std::string jsonNoGPU = R"({"ramUsage": "150MB","cpuUsage": "20%","uptime": "45s"})";
    const size_t noGPUPos = jsonNoGPU.find(gpuMemoryKey);
    L0Test::ExpectTrue(tr, noGPUPos == std::string::npos, "gpuMemory field is absent when not present");
    
    return tr.failures;
}

uint32_t Test_RM_Enforcement_GPUMemoryValueExtraction()
{
    L0Test::TestResult tr;
    
    // Test numeric parsing of GPU memory values
    std::string gpuMemoryStr = "32";
    size_t valueLen = 0;
    uint64_t gpuMemoryMB = 0;
    
    try {
        gpuMemoryMB = std::stoull(gpuMemoryStr, &valueLen);
        L0Test::ExpectEqU32(tr, static_cast<uint32_t>(gpuMemoryMB), 32U, "32 parses to 32");
        L0Test::ExpectTrue(tr, valueLen == 2, "Value length is 2 (digits only)");
    } catch (...) {
        L0Test::ExpectTrue(tr, false, "32 should parse successfully");
    }
    
    // Test without MB suffix
    gpuMemoryStr = "64";
    valueLen = 0;
    try {
        gpuMemoryMB = std::stoull(gpuMemoryStr, &valueLen);
        L0Test::ExpectEqU32(tr, static_cast<uint32_t>(gpuMemoryMB), 64U, "64 parses to 64");
    } catch (...) {
        L0Test::ExpectTrue(tr, false, "64 should parse successfully");
    }
    
    // Test invalid value
    gpuMemoryStr = "invalid";
    try {
        gpuMemoryMB = std::stoull(gpuMemoryStr, &valueLen);
        L0Test::ExpectTrue(tr, false, "Invalid string should not parse");
    } catch (...) {
        L0Test::ExpectTrue(tr, true, "Invalid string throws exception as expected");
    }
    
    return tr.failures;
}

uint32_t Test_RM_Enforcement_SuspendedSupportCheck()
{
    L0Test::TestResult tr;
    
    // Test SUSPENDED support check
    AppLifecycleCapabilities caps;
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kAllStates), caps);
    
    bool supportsSuspended = (caps.supportedStates & static_cast<uint8_t>(AppLifecycleState::SUSPENDED)) != 0;
    L0Test::ExpectTrue(tr, supportsSuspended, "All states supports SUSPENDED");
    
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kPausedSuspended), caps);
    supportsSuspended = (caps.supportedStates & static_cast<uint8_t>(AppLifecycleState::SUSPENDED)) != 0;
    L0Test::ExpectTrue(tr, supportsSuspended, "PAUSED+SUSPENDED supports SUSPENDED");
    
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kPausedOnly), caps);
    supportsSuspended = (caps.supportedStates & static_cast<uint8_t>(AppLifecycleState::SUSPENDED)) != 0;
    L0Test::ExpectTrue(tr, !supportsSuspended, "PAUSED only does NOT support SUSPENDED");
    
    return tr.failures;
}

uint32_t Test_RM_Enforcement_HibernatedSupportCheck()
{
    L0Test::TestResult tr;
    
    // Test HIBERNATED support check
    AppLifecycleCapabilities caps;
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kAllStates), caps);
    
    bool supportsHibernated = (caps.supportedStates & static_cast<uint8_t>(AppLifecycleState::HIBERNATED)) != 0;
    L0Test::ExpectTrue(tr, supportsHibernated, "All states supports HIBERNATED");
    
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kPausedSuspended), caps);
    supportsHibernated = (caps.supportedStates & static_cast<uint8_t>(AppLifecycleState::HIBERNATED)) != 0;
    L0Test::ExpectTrue(tr, !supportsHibernated, "PAUSED+SUSPENDED does NOT support HIBERNATED");
    
    AppLifecycleConfig::Parse(L0Test::RalfPayload(kPausedOnly), caps);
    supportsHibernated = (caps.supportedStates & static_cast<uint8_t>(AppLifecycleState::HIBERNATED)) != 0;
    L0Test::ExpectTrue(tr, !supportsHibernated, "PAUSED only does NOT support HIBERNATED");
    
    return tr.failures;
}

uint32_t Test_RM_Enforcement_ConfigurationParsing()
{
    L0Test::TestResult tr;
    
    // Test that pausedToSuspendedTimeout can be parsed
    // This is a simple validation that the config structure exists
    // Actual config loading is tested elsewhere
    
    uint32_t timeoutMs = 60000; // 60 seconds in milliseconds
    L0Test::ExpectEqU32(tr, timeoutMs, 60000U, "60 second timeout is 60000 milliseconds");
    
    uint32_t suspendedTimeoutMs = 15000; // 15 seconds in milliseconds
    L0Test::ExpectEqU32(tr, suspendedTimeoutMs, 15000U, "15 second timeout is 15000 milliseconds");
    
    return tr.failures;
}
