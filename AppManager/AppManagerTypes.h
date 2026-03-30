/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
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
*/

#pragma once

#include "Module.h"
#include <interfaces/ILifecycleManager.h>
#include <string>
#include <stdint.h>

namespace WPEFramework {
namespace Plugin {
namespace AppManagerTypes {

    /**
     * @brief Classifies the application type (interactive vs. system).
     */
    typedef enum _applicationType
    {
        APPLICATION_TYPE_UNKNOWN     = 0,
        APPLICATION_TYPE_INTERACTIVE = 1,
        APPLICATION_TYPE_SYSTEM      = 2
    } ApplicationType;

    /**
     * @brief Package metadata obtained from the PackageManager.
     */
    typedef struct _packageInfo
    {
        std::string version      = "";
        uint32_t    lockId       = 0;
        std::string unpackedPath = "";
        WPEFramework::Exchange::RuntimeConfig configMetadata;
        std::string appMetadata  = "";
        ApplicationType type     = APPLICATION_TYPE_UNKNOWN;
    } PackageInfo;

    /**
     * @brief Tracks the current outstanding lifecycle action for an app.
     */
    enum CurrentAction
    {
        APP_ACTION_NONE      = 0,
        APP_ACTION_LAUNCH    = 1,
        APP_ACTION_PRELOAD   = 2,
        APP_ACTION_SUSPEND   = 3,
        APP_ACTION_RESUME    = 4,
        APP_ACTION_CLOSE     = 5,
        APP_ACTION_TERMINATE = 6,
        APP_ACTION_HIBERNATE = 7,
        APP_ACTION_KILL      = 8
    };

    /**
     * @brief Error codes reported to telemetry for action failures.
     */
    enum CurrentActionError
    {
        ERROR_NONE                = 0,
        ERROR_INVALID_PARAMS      = 1,
        ERROR_INTERNAL            = 2,
        ERROR_PACKAGE_LOCK        = 3,
        ERROR_PACKAGE_UNLOCK      = 4,
        ERROR_PACKAGE_LIST_FETCH  = 5,
        ERROR_PACKAGE_NOT_INSTALLED = 6,
        ERROR_PACKAGE_INVALID     = 7,
        ERROR_SPAWN_APP           = 8,
        ERROR_UNLOAD_APP          = 9,
        ERROR_KILL_APP            = 10,
        ERROR_SET_TARGET_APP_STATE = 11
    };

} // namespace AppManagerTypes
} // namespace Plugin
} // namespace WPEFramework
