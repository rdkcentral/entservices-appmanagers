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

#pragma once

/**
 * @file L0TestConfig.h
 * @brief Common configuration macros for L0 tests
 * 
 * This header defines compile-time configuration macros used across all
 * L0 test targets. These can override CMake-defined macros if needed.
 */

// Thunder/WPEFramework configuration
#ifndef USE_THUNDER_R4
#define USE_THUNDER_R4 1
#endif

#ifndef DISABLE_SECURITY_TOKEN
#define DISABLE_SECURITY_TOKEN 1
#endif

#ifndef THUNDER_PLATFORM_PC_UNIX
#define THUNDER_PLATFORM_PC_UNIX 1
#endif

// AppStorageManager API version
#ifndef STORAGE_MANAGER_API_VERSION_NUMBER_MAJOR
#define STORAGE_MANAGER_API_VERSION_NUMBER_MAJOR 1
#endif

#ifndef STORAGE_MANAGER_API_VERSION_NUMBER_MINOR
#define STORAGE_MANAGER_API_VERSION_NUMBER_MINOR 0
#endif

#ifndef STORAGE_MANAGER_API_VERSION_NUMBER_PATCH
#define STORAGE_MANAGER_API_VERSION_NUMBER_PATCH 0
#endif

// RuntimeManager API version
#ifndef RUNTIME_MANAGER_API_VERSION_NUMBER_MAJOR
#define RUNTIME_MANAGER_API_VERSION_NUMBER_MAJOR 1
#endif

#ifndef RUNTIME_MANAGER_API_VERSION_NUMBER_MINOR
#define RUNTIME_MANAGER_API_VERSION_NUMBER_MINOR 0
#endif

#ifndef RUNTIME_MANAGER_API_VERSION_NUMBER_PATCH
#define RUNTIME_MANAGER_API_VERSION_NUMBER_PATCH 0
#endif

// LifecycleManager API version
#ifndef LIFECYCLE_MANAGER_API_VERSION_NUMBER_MAJOR
#define LIFECYCLE_MANAGER_API_VERSION_NUMBER_MAJOR 1
#endif

#ifndef LIFECYCLE_MANAGER_API_VERSION_NUMBER_MINOR
#define LIFECYCLE_MANAGER_API_VERSION_NUMBER_MINOR 0
#endif

#ifndef LIFECYCLE_MANAGER_API_VERSION_NUMBER_PATCH
#define LIFECYCLE_MANAGER_API_VERSION_NUMBER_PATCH 0
#endif

// RDKWindowManager API version
#ifndef RDK_WINDOW_MANAGER_API_VERSION_NUMBER_MAJOR
#define RDK_WINDOW_MANAGER_API_VERSION_NUMBER_MAJOR 1
#endif

#ifndef RDK_WINDOW_MANAGER_API_VERSION_NUMBER_MINOR
#define RDK_WINDOW_MANAGER_API_VERSION_NUMBER_MINOR 0
#endif

#ifndef RDK_WINDOW_MANAGER_API_VERSION_NUMBER_PATCH
#define RDK_WINDOW_MANAGER_API_VERSION_NUMBER_PATCH 0
#endif

// PreinstallManager API version
#ifndef PREINSTALL_MANAGER_API_VERSION_NUMBER_MAJOR
#define PREINSTALL_MANAGER_API_VERSION_NUMBER_MAJOR 1
#endif

#ifndef PREINSTALL_MANAGER_API_VERSION_NUMBER_MINOR
#define PREINSTALL_MANAGER_API_VERSION_NUMBER_MINOR 0
#endif

#ifndef PREINSTALL_MANAGER_API_VERSION_NUMBER_PATCH
#define PREINSTALL_MANAGER_API_VERSION_NUMBER_PATCH 0
#endif

// Debug flags
#ifndef RDK_APPMANAGERS_DEBUG
#define RDK_APPMANAGERS_DEBUG 1
#endif

// Build type for AI configuration (RuntimeManager)
#ifndef AI_DEBUG
#define AI_DEBUG 1
#endif

#ifndef AI_RELEASE
#define AI_RELEASE 2
#endif

#ifndef AI_BUILD_TYPE
#define AI_BUILD_TYPE AI_DEBUG
#endif

// Feature flags
#ifndef RALF_PACKAGE_SUPPORT_ENABLED
#define RALF_PACKAGE_SUPPORT_ENABLED 1
#endif

#ifndef ENABLE_AIMANAGERS_TELEMETRY_METRICS
#define ENABLE_AIMANAGERS_TELEMETRY_METRICS 1
#endif

#ifndef UNIT_TEST
#define UNIT_TEST 1
#endif
