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

#include <chrono>
#include <thread>
#include <utility>

#include "PreinstallManagerImplementation.h"


namespace WPEFramework
{
    namespace Plugin
    {

    SERVICE_REGISTRATION(PreinstallManagerImplementation, 1, 0);
    PreinstallManagerImplementation *PreinstallManagerImplementation::_instance = nullptr;

    PreinstallManagerImplementation::PreinstallManagerImplementation()
        : mAdminLock(), mAppPreinstallDirectory(""), mPreinstallManagerNotifications(), mCurrentservice(nullptr),
          mPreinstallState(State::NOT_STARTED), mInstallThread()
    {
        LOGINFO("Create PreinstallManagerImplementation Instance");
        if (nullptr == PreinstallManagerImplementation::_instance)
        {
            PreinstallManagerImplementation::_instance = this;
        }
    }

    PreinstallManagerImplementation *PreinstallManagerImplementation::getInstance()
    {
        return _instance;
    }

    PreinstallManagerImplementation::~PreinstallManagerImplementation()
    {
        LOGINFO("Delete PreinstallManagerImplementation Instance");

        if (mInstallThread.joinable())
        {
            LOGWARN("mInstallThread still joinable in destructor; detaching to avoid blocking destruction");
            mInstallThread.detach();
        }

        _instance = nullptr;
        if (nullptr != mCurrentservice)
        {
            mCurrentservice->Release();
            mCurrentservice = nullptr;
        }
    }

    /**
     * Register a notification callback
     */
    Core::hresult PreinstallManagerImplementation::Register(Exchange::IPreinstallManager::INotification *notification)
    {
        ASSERT(nullptr != notification);

        mAdminLock.Lock();

        if (std::find(mPreinstallManagerNotifications.begin(), mPreinstallManagerNotifications.end(), notification) == mPreinstallManagerNotifications.end())
        {
            LOGINFO("Register notification");
            mPreinstallManagerNotifications.push_back(notification);
            notification->AddRef();
        }

        mAdminLock.Unlock();

        return Core::ERROR_NONE;
    }

    /**
     * Unregister a notification callback
     */
    Core::hresult PreinstallManagerImplementation::Unregister(Exchange::IPreinstallManager::INotification *notification)
    {
        Core::hresult status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);

        mAdminLock.Lock();

        auto itr = std::find(mPreinstallManagerNotifications.begin(), mPreinstallManagerNotifications.end(), notification);
        if (itr != mPreinstallManagerNotifications.end())
        {
            (*itr)->Release();
            LOGINFO("Unregister notification");
            mPreinstallManagerNotifications.erase(itr);
            status = Core::ERROR_NONE;
        }
        else
        {
            LOGERR("notification not found");
        }

        mAdminLock.Unlock();

        return status;
    }

    /**
     * Initialize the implementation with the current service
     */
    uint32_t PreinstallManagerImplementation::Configure(PluginHost::IShell* service)
    {
        uint32_t result = Core::ERROR_GENERAL;
        if (service != nullptr)
        {
            mCurrentservice = service;
            mCurrentservice->AddRef();
            PreinstallManagerImplementation::Configuration config;
            config.FromString(service->ConfigLine());
            if (!config.appPreinstallDirectory.Value().empty())
            {
                mAppPreinstallDirectory = config.appPreinstallDirectory.Value();
            }
            LOGINFO("appPreinstallDirectory=%s", mAppPreinstallDirectory.c_str());
            result = Core::ERROR_NONE;
            LOGINFO("PreinstallManagerImplementation service configured successfully");
        }
        else
        {
            LOGERR("service is null \n");
        }

        return result;
    }

    void PreinstallManagerImplementation::dispatchEvent(EventNames event, const JsonObject &params)
    {
        Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
    }

    void PreinstallManagerImplementation::Dispatch(EventNames event, const JsonObject params)
    {
        switch (event)
        {
        case PREINSTALL_MANAGER_ONPREINSTALLATIONCOMPLETE:
        {
            LOGINFO("Sending OnPreinstallationComplete event");
            mAdminLock.Lock();
            for (auto notification : mPreinstallManagerNotifications)
            {
                notification->OnPreinstallationComplete();
                LOGTRACE();
            }
            mAdminLock.Unlock();
            break;
        }
        default:
            LOGERR("Unknown event: %d", static_cast<int>(event));
            break;
        }
    }

    /**
     * Send OnPreinstallationComplete event to all registered listeners
     */
    void PreinstallManagerImplementation::sendOnPreinstallationCompleteEvent()
    {
        LOGINFO("Dispatching OnPreinstallationComplete event");
        JsonObject eventDetails; // OnPreinstallationComplete doesn't need any params
        dispatchEvent(PREINSTALL_MANAGER_ONPREINSTALLATIONCOMPLETE, eventDetails);
    }

    Core::hresult PreinstallManagerImplementation::createPackageManagerObject(Exchange::IPackageInstaller*& packageInstaller)
    {
        Core::hresult status = Core::ERROR_GENERAL;
        packageInstaller = nullptr;

        if (nullptr == mCurrentservice)
        {
            LOGERR("mCurrentservice is null \n");
        }
        else if (nullptr == (packageInstaller = mCurrentservice->QueryInterfaceByCallsign<WPEFramework::Exchange::IPackageInstaller>("org.rdk.PackageManagerRDKEMS")))
        {
            LOGERR("PackageManager installer object is null \n");
        }
        else
        {
            LOGINFO("created PackageInstaller Object\n");
            status = Core::ERROR_NONE;
        }
        return status;
    }

    void PreinstallManagerImplementation::releasePackageManagerObject(Exchange::IPackageInstaller*& packageInstaller)
    {
        if (packageInstaller)
        {
            packageInstaller->Release();
            packageInstaller = nullptr;
        }
    }

    //compare package versions
    bool PreinstallManagerImplementation::isNewerVersion(const std::string &v1, const std::string &v2)
    {
        // Strip at first '-' or '+'
        std::string::size_type pos1 = std::min(v1.find('-'), v1.find('+'));
        std::string::size_type pos2 = std::min(v2.find('-'), v2.find('+'));

        std::string base1 = (pos1 == std::string::npos) ? v1 : v1.substr(0, pos1);
        std::string base2 = (pos2 == std::string::npos) ? v2 : v2.substr(0, pos2);

        int maj1 = 0, min1 = 0, patch1 = 0, build1 = 0;
        int maj2 = 0, min2 = 0, patch2 = 0, build2 = 0;

        if (std::sscanf(base1.c_str(), "%d.%d.%d.%d", &maj1, &min1, &patch1, &build1) < 3)
        {
            LOGERR("Version string '%s' is not in valid format", v1.c_str());
            return false;
        }
        if (std::sscanf(base2.c_str(), "%d.%d.%d.%d", &maj2, &min2, &patch2, &build2) < 3)
        {
            LOGERR("Version string '%s' is not in valid format", v2.c_str());
            return false;
        }

        if (maj1 != maj2)
            return maj1 > maj2;
        if (min1 != min2)
            return min1 > min2;
        if (patch1 != patch2)
            return patch1 > patch2;
        if (build1 != build2)
            return build1 > build2;

        return false; // equal
    }

    // bool packageWgtExists(const std::string& folderPath) //required??
    // {
    //     std::string packageWgtPath = folderPath + "/package.wgt";
    //     struct stat st;
    //     return (stat(packageWgtPath.c_str(), &st) == 0) && S_ISREG(st.st_mode);
    // }

    /**
     * Traverse the preinstall directory and populate the list of packages to be preinstalled,
     * also fetches the package details
     */
    bool PreinstallManagerImplementation::readPreinstallDirectory(Exchange::IPackageInstaller* packageInstaller, std::list<PackageInfo> &packages)
    {
        ASSERT(nullptr != packageInstaller);
        DIR *dir = opendir(mAppPreinstallDirectory.c_str());
        if (!dir)
        {
            LOGINFO("Failed to open directory: %s", mAppPreinstallDirectory.c_str());
            return false;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            std::string filename(entry->d_name);

            // Skip "." and ".."
            if (filename == "." || filename == "..")
                continue;

            std::string filepath = mAppPreinstallDirectory + "/" + filename;

            PackageInfo packageInfo;
            packageInfo.fileLocator = filepath;
            LOGDBG("Found package folder: %s", filepath.c_str());
            if (packageInstaller->GetConfigForPackage(packageInfo.fileLocator, packageInfo.packageId, packageInfo.version, packageInfo.configMetadata) == Core::ERROR_NONE)
            {
                LOGINFO("Found package: %s, version: %s", packageInfo.packageId.c_str(), packageInfo.version.c_str());
            }
            else
            {
                LOGINFO("Skipping invalid package file: %s", filename.c_str());
                packageInfo.installStatus = "SKIPPED: getConfig failed for [" + filename + "]";
                // continue; -> so that it is printed as skipped and not go undetected
            }
            packages.push_back(packageInfo);
        }

        closedir(dir);
        return true;
    }

    string PreinstallManagerImplementation::getFailReason(FailReason reason) {
        switch (reason) {
            case FailReason::SIGNATURE_VERIFICATION_FAILURE : return "SIGNATURE_VERIFICATION_FAILURE";
            case FailReason::PACKAGE_MISMATCH_FAILURE : return "PACKAGE_MISMATCH_FAILURE";
            case FailReason::INVALID_METADATA_FAILURE : return "INVALID_METADATA_FAILURE";
            case FailReason::PERSISTENCE_FAILURE : return "PERSISTENCE_FAILURE";
            default: return "NONE";
        }
    }

    void PreinstallManagerImplementation::installPackages(std::list<PackageInfo> preinstallPackages)
    {
        Exchange::IPackageInstaller* packageInstaller = nullptr;
        mAdminLock.Lock();
        mPreinstallState = State::IN_PROGRESS;
        mAdminLock.Unlock();

        if (Core::ERROR_NONE != createPackageManagerObject(packageInstaller))
        {
            LOGERR("Failed to create PackageManagerObject for install");
            mAdminLock.Lock();
            mPreinstallState = State::COMPLETED;
            mAdminLock.Unlock();
            sendOnPreinstallationCompleteEvent();
            return;
        }

        auto installStart = std::chrono::steady_clock::now();
        bool installError = false;
        int failedApps = 0;
        const int totalApps = preinstallPackages.size();

        for (auto &pkg : preinstallPackages)
        {
            if ((pkg.packageId.empty() || pkg.version.empty() || pkg.fileLocator.empty()))
            {
                LOGERR("Skipping invalid package with empty fields: %s", pkg.fileLocator.empty() ? "NULL" : pkg.fileLocator.c_str());
                if (pkg.installStatus.empty())
                {
                    pkg.installStatus = "FAILED: empty fields";
                }
                pkg.fileLocator = pkg.fileLocator.empty() ? "NULL" : pkg.fileLocator;
                pkg.packageId = pkg.packageId.empty() ? pkg.fileLocator : pkg.packageId;
                pkg.version = pkg.version.empty() ? "NULL" : pkg.version;
                failedApps++;
                continue;
            }

            LOGINFO("Installing package: %s, version: %s", pkg.packageId.c_str(), pkg.version.c_str());

            FailReason failReason;
            Exchange::IPackageInstaller::IKeyValueIterator* additionalMetadata = nullptr;
            Core::hresult installResult = packageInstaller->Install(pkg.packageId, pkg.version, additionalMetadata, pkg.fileLocator, failReason);

            if (Core::ERROR_NONE != installResult)
            {
                LOGERR("Failed to install package: %s, version: %s, failReason: %s", pkg.packageId.c_str(), pkg.version.c_str(), getFailReason(failReason).c_str());
                installError = true;
                failedApps++;
                pkg.installStatus = "FAILED: reason " + getFailReason(failReason);
                continue;
            }

            LOGINFO("Successfully installed package: %s, version: %s, fileLocator: %s", pkg.packageId.c_str(), pkg.version.c_str(), pkg.fileLocator.c_str());
            pkg.installStatus = "SUCCESS";
        }

        const auto installEnd = std::chrono::steady_clock::now();
        const auto installDuration = std::chrono::duration_cast<std::chrono::seconds>(installEnd - installStart).count();
        const auto installDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(installEnd - installStart).count();
        LOGDBG("Process completed in %lld seconds (%lld ms)", installDuration, installDurationMs);
        LOGINFO("Installation summary: %d/%d packages installed successfully. %d apps failed.", totalApps - failedApps, totalApps, failedApps);
        for (const auto &pkg : preinstallPackages)
        {
            LOGINFO("Package: %s [version:%s]............status:[ %s ]", pkg.packageId.c_str(), pkg.version.c_str(), pkg.installStatus.c_str());
        }

        releasePackageManagerObject(packageInstaller);

        mAdminLock.Lock();
        mPreinstallState = State::COMPLETED;
        mAdminLock.Unlock();

        if (installError)
        {
            LOGWARN("Preinstall completed with failures");
        }
        sendOnPreinstallationCompleteEvent();
    }

    /*
     * @brief Checks the preinstall directory for packages to be preinstalled and installs them as needed.
     * @Params[in]  : bool forceInstall
     * @Params[out] : None
     * @return      : Core::hresult
     */
    Core::hresult PreinstallManagerImplementation::StartPreinstall(bool forceInstall)
    {
        Core::hresult result = Core::ERROR_GENERAL;
        std::thread previousInstallThread;
        Exchange::IPackageInstaller* packageInstaller = nullptr;

        mAdminLock.Lock();
        if (mInstallThread.joinable())
        {
            if (State::COMPLETED != mPreinstallState)
            {
                mAdminLock.Unlock();
                LOGERR("Preinstall is already in progress");
                return result;
            }
            // Take ownership of the previous install thread so only this caller can join it.
            previousInstallThread = std::move(mInstallThread);
        }
        else if (State::IN_PROGRESS == mPreinstallState)
        {
            mAdminLock.Unlock();
            LOGERR("Preinstall is already in progress");
            return result;
        }
        mAdminLock.Unlock();

        if (previousInstallThread.joinable())
        {
            previousInstallThread.join();
        }

        LOGINFO("Create PackageManager object for preinstall listing");
        if (Core::ERROR_NONE != createPackageManagerObject(packageInstaller))
        {
            LOGERR("Failed to create PackageManagerObject");
            return result;
        }
        ASSERT(nullptr != packageInstaller);

        // read the preinstall directory and populate packages
        std::list<PackageInfo> preinstallPackages; // all apps in preinstall directory
        if (!readPreinstallDirectory(packageInstaller, preinstallPackages))
        {
            LOGERR("Failed to read preinstall directory");
            releasePackageManagerObject(packageInstaller);
            return result;
        }

        // Check if the packages list is empty
        if (preinstallPackages.empty())
        {
            LOGINFO("No packages to preinstall. Sending OnPreinstallationComplete event");
            mAdminLock.Lock();
            mPreinstallState = State::COMPLETED;
            mAdminLock.Unlock();
            releasePackageManagerObject(packageInstaller);
            sendOnPreinstallationCompleteEvent();
            result = Core::ERROR_NONE;
            return result;
        }

        if (!forceInstall)  // if false, we need to check installed packages
        {
            LOGWARN("forceInstall is disabled");
            Exchange::IPackageInstaller::IPackageIterator *packageList = nullptr;

            // fetch installed packages
            Core::hresult listResult = packageInstaller->ListPackages(packageList);
            if (listResult != Core::ERROR_NONE || packageList == nullptr)
            {
                 LOGERR("ListPackages failed or package list is null");
                if (packageList != nullptr)
                {
                    packageList->Release();
                    packageList = nullptr;
                }
                releasePackageManagerObject(packageInstaller);
                return result;
            }

            WPEFramework::Exchange::IPackageInstaller::Package package;
            std::unordered_map<std::string, std::string> existingApps; // packageId -> version

            while (packageList->Next(package) && package.state == InstallState::INSTALLED) // only consider installed apps
            {
                existingApps[package.packageId] = package.version;
                // todo check for installState if needed
                // multiple apps possible with same packageId but different version
            }

            packageList->Release();
            packageList = nullptr;

            // filter to-be-installed apps
            for (auto toBeInstalledApp = preinstallPackages.begin(); toBeInstalledApp != preinstallPackages.end(); /* skip */)
            {
                bool remove = false;

                // check if app is already installed
                auto it = existingApps.find(toBeInstalledApp->packageId);
                if (it != existingApps.end())
                {
                    const std::string &installedVersion = it->second;

                    // check if to-be-installed version is newer
                    if (!isNewerVersion(toBeInstalledApp->version, installedVersion))
                    {
                        // not newer (equal or older) → skip install
                        LOGINFO("Not installing package: %s, version: %s (installed version: %s)",
                                    toBeInstalledApp->packageId.c_str(),
                                    toBeInstalledApp->version.c_str(),
                                    installedVersion.c_str());
                        remove = true;
                    }
                    else
                    {
                        LOGINFO("Installing newer version of package: %s, version: %s (installed version: %s)",
                                    toBeInstalledApp->packageId.c_str(),
                                    toBeInstalledApp->version.c_str(),
                                    installedVersion.c_str());
                    }
                }

                if (remove)
                {
                    toBeInstalledApp = preinstallPackages.erase(toBeInstalledApp); // advances automatically
                }
                else
                {
                    ++toBeInstalledApp;
                }
            }

        }

        // Check if all packages were filtered out after forceInstall=false filtering
        if (preinstallPackages.empty())
        {
            LOGINFO("No packages to preinstall after filtering. Sending OnPreinstallationComplete event");
            mAdminLock.Lock();
            mPreinstallState = State::COMPLETED;
            mAdminLock.Unlock();
            releasePackageManagerObject(packageInstaller);
            sendOnPreinstallationCompleteEvent();
            result = Core::ERROR_NONE;
            return result;
        }

        releasePackageManagerObject(packageInstaller);

        // Set state to IN_PROGRESS before starting the worker thread to avoid race with GetPreinstallState()
        mAdminLock.Lock();
        mPreinstallState = State::IN_PROGRESS;
        mAdminLock.Unlock();

        try
        {
            mInstallThread = std::thread(&PreinstallManagerImplementation::installPackages, this, std::move(preinstallPackages));
            result = Core::ERROR_NONE;
        }
        catch (...)
        {
            LOGERR("Failed to start preinstall worker thread");
            mAdminLock.Lock();
            mPreinstallState = State::NOT_STARTED;
            mAdminLock.Unlock();
        }
        return result;
    }

    /*
     * @brief Provides the state of preinstallation process
     * @Params[out] : State& state - Value can be NOT_STARTED/IN_PROGRESS/COMPLETED
     * @return      : Core::hresult
     */
    Core::hresult PreinstallManagerImplementation::GetPreinstallState(State& state)
    {
        LOGINFO("PreinstallState API called");
        mAdminLock.Lock();
        state = mPreinstallState;
        mAdminLock.Unlock();
        return Core::ERROR_NONE;
    }

    } /* namespace Plugin */
} /* namespace WPEFramework */
