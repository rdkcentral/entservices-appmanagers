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

#include "Module.h"
#include "GStreamerRegistry.h"
#include "UtilsLogging.h"

#include <spawn.h>
#include <wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

namespace WPEFramework {
namespace Plugin {

namespace fs = std::filesystem;

GStreamerRegistry::GStreamerRegistry(std::filesystem::path gstLaunchPath,
                                     std::filesystem::path gstRegistryPath)
    : mGstLaunchPath(std::move(gstLaunchPath))
    , mRegistryPath(std::move(gstRegistryPath))
{
}

// -----------------------------------------------------------------------------
/**
 * Generates the GStreamer plugin registry file at mRegistryPath.
 *
 * Spawns gst-launch-1.0 --version with GST_REGISTRY set so GStreamer writes
 * the registry without starting a pipeline.  Blocks until the child exits
 * (typically < 2 s).
 *
 * Returns true if the registry file exists after the call.
 */
bool GStreamerRegistry::generate()
{
    std::error_code err;
    if (fs::exists(mRegistryPath, err))
        return true;

    const std::string registryEnvVar = "GST_REGISTRY=" + mRegistryPath.string();

    char *args[] = {
        strdup(mGstLaunchPath.filename().c_str()),
        strdup("--version"),
        nullptr
    };
    char *envs[] = {
        strdup(registryEnvVar.c_str()),
        nullptr
    };

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, "/dev/null", O_WRONLY | O_APPEND, 0);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY | O_APPEND, 0);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    pid_t pid = -1;
    int ret = posix_spawnp(&pid, mGstLaunchPath.c_str(), &actions, &attr, args, envs);

    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);

    for (char *p : args) free(p);
    for (char *p : envs) free(p);

    if (ret != 0 || pid <= 0)
    {
        LOGERR("GStreamerRegistry: failed to spawn %s (errno %d)", mGstLaunchPath.c_str(), ret);
        return false;
    }

    int wstatus = 0;
    if (TEMP_FAILURE_RETRY(waitpid(pid, &wstatus, 0)) < 0)
    {
        LOGERR("GStreamerRegistry: waitpid failed (errno %d)", errno);
        return false;
    }

    if (!WIFEXITED(wstatus))
    {
        LOGERR("GStreamerRegistry: %s did not exit normally (status 0x%04x)",
               mGstLaunchPath.c_str(), wstatus);
        return false;
    }

    LOGINFO("GStreamerRegistry: %s exited with status %d",
            mGstLaunchPath.c_str(), WEXITSTATUS(wstatus));

    if (!fs::exists(mRegistryPath, err))
    {
        LOGWARN("GStreamerRegistry: %s did not create registry file at %s",
                mGstLaunchPath.c_str(), mRegistryPath.c_str());
        return false;
    }

    // Make registry readable by all users inside containers.
    fs::permissions(mRegistryPath,
                    fs::perms::group_read | fs::perms::others_read,
                    fs::perm_options::add, err);
    if (err)
    {
        LOGERR("GStreamerRegistry: failed to set read permissions on %s: %s",
               mRegistryPath.c_str(), err.message().c_str());
        return false;
    }

    LOGINFO("GStreamerRegistry: registry created at %s", mRegistryPath.c_str());
    return true;
}

// -----------------------------------------------------------------------------
/**
 * Returns the registry path if the file exists, otherwise an empty path.
 */
std::filesystem::path GStreamerRegistry::path() const
{
    std::error_code err;
    return fs::exists(mRegistryPath, err) ? mRegistryPath : std::filesystem::path{};
}

} /* namespace Plugin */
} /* namespace WPEFramework */
