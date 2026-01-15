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

#pragma once

#include <syscall.h>
#include <time.h>
#include <stdio.h>
#include <string>

namespace WPEFramework {
namespace Logging {
    // Helper function to generate the timestamp prefix for logs
    static inline std::string _GetLogPrefix()
    {
        char buffer[128];

        // Get Monotonic Time (CLOCK_MONOTONIC)
        struct timespec monotonic_ts;
        clock_gettime(CLOCK_MONOTONIC, &monotonic_ts);

        // Calculate total seconds
        double total_seconds = static_cast<double>(monotonic_ts.tv_sec) + 
                               static_cast<double>(monotonic_ts.tv_nsec) / 1.0e9;

        // Format as minutes with 3 decimal places, padded for alignment
        snprintf(buffer, sizeof(buffer), "[%09.3f min]", total_seconds / 60.0);

        return std::string(buffer);
    }
} // namespace Logging
} // namespace WPEFramework


#ifdef __DEBUG__    // XXX: maybe use BUILD_TYPE
#define LOGTRACE(fmt, ...) do { fprintf(stderr, "%s [%d] TRACE [%s:%d] %s: " fmt "\n", WPEFramework::Logging::_GetLogPrefix().c_str(), (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#else
#define LOGTRACE(fmt, ...)
#endif
#define LOGDBG(fmt, ...) do { fprintf(stderr, "%s [%d] DEBUG [%s:%d] %s: " fmt "\n", WPEFramework::Logging::_GetLogPrefix().c_str(), (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGINFO(fmt, ...) do { fprintf(stderr, "%s [%d] INFO [%s:%d] %s: " fmt "\n", WPEFramework::Logging::_GetLogPrefix().c_str(), (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGWARN(fmt, ...) do { fprintf(stderr, "%s [%d] WARN [%s:%d] %s: " fmt "\n", WPEFramework::Logging::_GetLogPrefix().c_str(), (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGERR(fmt, ...) do { fprintf(stderr, "%s [%d] ERROR [%s:%d] %s: " fmt "\n", WPEFramework::Logging::_GetLogPrefix().c_str(), (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)

#define LOG_DEVICE_EXCEPTION0() LOGWARN("Exception caught: code=%d message=%s", err.getCode(), err.what());
#define LOG_DEVICE_EXCEPTION1(param1) LOGWARN("Exception caught" #param1 "=%s code=%d message=%s", param1.c_str(), err.getCode(), err.what());
#define LOG_DEVICE_EXCEPTION2(param1, param2) LOGWARN("Exception caught " #param1 "=%s " #param2 "=%s code=%d message=%s", param1.c_str(), param2.c_str(), err.getCode(), err.what());

