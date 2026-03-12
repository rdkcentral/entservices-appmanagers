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

#include <iomanip>      /* for std::setw, std::setfill */
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>   /* for stat(), S_ISREG - used by IsHibernationSupported() */
#include "AppManagerImplementation.h"
#include "MemoryMonitor.h"
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
#include "AppManagerTelemetryReporting.h"
#endif

#define TIME_DATA_SIZE           200
static bool sRunning = false;

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(AppManagerImplementation, 1, 0);
AppManagerImplementation* AppManagerImplementation::_instance = nullptr;

AppManagerImplementation::AppManagerImplementation()
: mAdminLock()
, mAppManagerNotification()
, mLifecycleInterfaceConnector(nullptr)
, mPersistentStoreRemoteStoreObject(nullptr)
, mPackageManagerHandlerObject(nullptr)
, mPackageManagerInstallerObject(nullptr)
, mCurrentservice(nullptr)
, mPackageManagerNotification(*this)
, mAppManagerWorkerThread()
{
    LOGINFO("Create AppManagerImplementation Instance");
    if (nullptr == AppManagerImplementation::_instance)
    {
        AppManagerImplementation::_instance = this;
    }
}

AppManagerImplementation* AppManagerImplementation::getInstance()
{
    return _instance;
}

AppManagerImplementation::~AppManagerImplementation()
{
    LOGINFO("Delete AppManagerImplementation Instance");
    _instance = nullptr;
    sRunning = false;

    /* Stop MemoryMonitor before other cleanup */
    if (mMemoryMonitor)
    {
        mMemoryMonitor->Stop();
        mMemoryMonitor.reset();
    }

    mAppRequestListCV.notify_all();
    if (mAppManagerWorkerThread.joinable())
    {
        mAppManagerWorkerThread.join();
        LOGINFO("App Manager Worker Thread joined successfully");
    }

    if (nullptr != mLifecycleInterfaceConnector)
    {
        mLifecycleInterfaceConnector->releaseLifecycleManagerRemoteObject();
        delete mLifecycleInterfaceConnector;
        mLifecycleInterfaceConnector = nullptr;
    }
    releasePersistentStoreRemoteStoreObject();
    releasePackageManagerObject();
    releaseStorageManagerRemoteObject();
    if (nullptr != mCurrentservice)
    {
       mCurrentservice->Release();
       mCurrentservice = nullptr;
    }
}

/**
 * Register a notification callback
 */
Core::hresult AppManagerImplementation::Register(Exchange::IAppManager::INotification *notification)
{
    ASSERT (nullptr != notification);

    mAdminLock.Lock();

    if (std::find(mAppManagerNotification.begin(), mAppManagerNotification.end(), notification) == mAppManagerNotification.end())
    {
        LOGINFO("Register notification");
        mAppManagerNotification.push_back(notification);
        notification->AddRef();
    }

    mAdminLock.Unlock();

    return Core::ERROR_NONE;
}

void AppManagerImplementation::AppManagerWorkerThread(void)
{
    while (sRunning)
    {
        std::shared_ptr<AppManagerRequest> request = nullptr;
        std::unique_lock<std::mutex> lock(mAppManagerLock);
        mAppRequestListCV.wait(lock, [this] {return !mAppRequestList.empty() || !sRunning;});

        if (!mAppRequestList.empty() && sRunning)
        {
            Core::hresult status = Core::ERROR_GENERAL;
            request = mAppRequestList.front();
            mAppRequestList.pop_front();

            if (request != nullptr)
            {
                CurrentAction action = request->mRequestAction;

                switch (action)
                {
                    case APP_ACTION_LAUNCH:
                    case APP_ACTION_PRELOAD:
                    {
                        if (nullptr != request->mRequestParam)
                        {
                            if (auto appRequestParam = std::static_pointer_cast<AppLaunchRequestParam>(request->mRequestParam))
                            {
                                string appId = appRequestParam->appId;
                                PackageInfo packageData;
                                Exchange::IPackageHandler::LockReason lockReason = Exchange::IPackageHandler::LockReason::LAUNCH;

                                status = packageLock(appId, packageData, lockReason);
                                if (status == Core::ERROR_NONE)
                                {
                                    WPEFramework::Exchange::RuntimeConfig runtimeConfig = packageData.configMetadata;
                                    runtimeConfig.unpackedPath = packageData.unpackedPath;
#ifdef RALF_PACKAGE_SUPPORT_ENABLED
                                    runtimeConfig.userId = packageData.userId;
                                    runtimeConfig.groupId = packageData.groupId;
#endif // RALF_PACKAGE_SUPPORT_ENABLED
                                    getCustomValues(runtimeConfig);
                                    string launchArgs = appRequestParam->launchArgs;

                                    if (action == APP_ACTION_LAUNCH)
                                    {
                                        /* TODO: Mark app as user-launched (not preloaded) ?? */
                                        // mAdminLock.Lock();
                                        // mAppInfo[appId].wasPreloaded = false;
                                        // mAdminLock.Unlock();

                                        status = mLifecycleInterfaceConnector->launch(appId, appRequestParam->intent, launchArgs, runtimeConfig);
                                        LOGINFO("Application Launch from thread returns with status %d", status);

                                        if (status != Core::ERROR_NONE)
                                        {
                                            LOGERR("launch failed status %d", status);
                                        }
                                        else
                                        {
                                            LOGINFO("launchApp successful for appId %s", appId.c_str());
                                            /* Trigger memory reconciliation for the app if enabled, to ensure it has enough memory to launch successfully. */
                                            if (mMemoryMonitor)
                                            {
                                                uint32_t launchTargetKB = 0;
                                                uint32_t preloadTargetKB = 0;
                                                GetAppMemoryConfig(appId, launchTargetKB, preloadTargetKB);
                                                LOGINFO("[MemoryMonitor] Triggering launching reconciliation for appId %s, target %u KB",
                                                        appId.c_str(), launchTargetKB);
                                                mMemoryMonitor->RequestReconciliation(appId, launchTargetKB);
                                            }
                                        }
                                    }
                                    else if (action == APP_ACTION_PRELOAD)
                                    {
                                        /* Mark app as preloaded (protected from kill during reclamation) */
                                        mAdminLock.Lock();
                                        mAppInfo[appId].wasPreloaded = true;
                                        mAdminLock.Unlock();

                                        string errorReason;
                                        status = mLifecycleInterfaceConnector->preLoadApp(appId, launchArgs, runtimeConfig, errorReason);
                                        LOGINFO("Application preLoad from thread returns with status %d error %s", status, errorReason.c_str());

                                        if ((!errorReason.empty()) || (status != Core::ERROR_NONE))
                                        {
                                            LOGERR("preLoadApp failed reason %s status %d", errorReason.c_str(), status);
                                        }
                                    }
                                }
                                else
                                {
                                    bool installed = false;

                                    IsInstalled(appId, installed);
                                    LOGERR("Failed to PackageManager Lock %s installed %d", appId.c_str(), installed);
                                    handleOnAppLifecycleStateChanged(appId, "",
                                        Exchange::IAppManager::APP_STATE_UNKNOWN,
                                        Exchange::IAppManager::APP_STATE_UNLOADED,
                                        (installed == true)?Exchange::IAppManager::APP_ERROR_PACKAGE_LOCK: Exchange::IAppManager::APP_ERROR_NOT_INSTALLED);
                                }
                            }
                        }
                    }
                    break; /*APP_ACTION_LAUNCH | APP_ACTION_PRELOAD*/

                    default:
                    {
                        LOGERR("action type is invalid");
                    }
                    break; /* defult*/
                }
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(mAppManagerLock);
        mAppRequestList.clear();
    }
}

/**
 * Unregister a notification callback
 */
Core::hresult AppManagerImplementation::Unregister(Exchange::IAppManager::INotification *notification )
{
    Core::hresult status = Core::ERROR_GENERAL;

    ASSERT (nullptr != notification);

    mAdminLock.Lock();

    auto itr = std::find(mAppManagerNotification.begin(), mAppManagerNotification.end(), notification);
    if (itr != mAppManagerNotification.end())
    {
        (*itr)->Release();
        LOGINFO("Unregister notification");
        mAppManagerNotification.erase(itr);
        status = Core::ERROR_NONE;
    }
    else
    {
        LOGERR("notification not found");
    }

    mAdminLock.Unlock();

    return status;
}

void AppManagerImplementation::dispatchEvent(EventNames event, const JsonObject &params)
{
    Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
}

void AppManagerImplementation::Dispatch(EventNames event, const JsonObject params)
{
    switch(event)
    {
        case APP_EVENT_LIFECYCLE_STATE_CHANGED:
        {
            string appId = "";
            string appInstanceId = "";
            AppLifecycleState newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
            AppLifecycleState oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
            AppErrorReason errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE;

            if (!(params.HasLabel("appId") && !(appId = params["appId"].String()).empty()))
            {
                LOGERR("appId not present or empty");
            }
            else if (!params.HasLabel("newState"))
            {
                LOGERR("newState not present");
            }
            else if (!params.HasLabel("oldState"))
            {
                LOGERR("oldState not present");
            }
            else if (!params.HasLabel("errorReason"))
            {
                LOGERR("errorReason not present");
            }
            else
            {
                appInstanceId = params.HasLabel("appInstanceId") ? params["appInstanceId"].String() : "";
                newState = static_cast<AppLifecycleState>(params["newState"].Number());
                oldState = static_cast<AppLifecycleState>(params["oldState"].Number());
                errorReason = static_cast<AppErrorReason>(params["errorReason"].Number());
                mAdminLock.Lock();
                for (auto& notification : mAppManagerNotification)
                {
                    notification->OnAppLifecycleStateChanged(appId, appInstanceId, newState, oldState, errorReason);
                }
                if ((Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING == oldState) && (Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING == newState))
                {
                    LOGERR("Transition from loading state failed. Killing the application ....");
                    Core::hresult status = Core::ERROR_UNAVAILABLE;
                    if (mLifecycleInterfaceConnector != nullptr)
                    {
                        status = mLifecycleInterfaceConnector->killApp(appId);
                    }
                    else
                    {
                        LOGERR("mLifecycleInterfaceConnector is null, cannot kill app %s", appId.c_str());
                    }
                    LOGERR("Kill app status in loading state [%d]....", static_cast<int>(status));
                }
                mAdminLock.Unlock();
            }
            break;
        }
        case APP_EVENT_INSTALLATION_STATUS:
        {
            string appId = "";
            string version = "";
            string installStatus = "";
            /* Check if 'packageId' exists and is not empty */
            appId = params.HasLabel("packageId") ? params["packageId"].String() : "";
            if (appId.empty())
            {
                LOGERR("appId is missing or empty");
            }
            else
            {
                installStatus = params.HasLabel("state") ? params["state"].String() : "";
                mAdminLock.Lock();
                for (auto& notification : mAppManagerNotification)
                {
                    if (installStatus == "INSTALLED")
                    {
                        version = params.HasLabel("version") ? params["version"].String() : "";
                        if (version.empty())
                        {
                            LOGERR("version is empty for installed app");
                        }
                        LOGINFO("OnAppInstalled appId %s version %s", appId.c_str(), version.c_str());
                        notification->OnAppInstalled(appId, version);
                    }
                    else if (installStatus == "UNINSTALLED")
                    {
                        LOGINFO("OnAppUninstalled appId %s", appId.c_str());
                        notification->OnAppUninstalled(appId);
                    }
                    else
                    {
                        LOGWARN("install status '%s' for appId %s", installStatus.c_str(), appId.c_str());
                    }
                }
                mAdminLock.Unlock();
            }
            break;
        }
        case APP_EVENT_LAUNCH_REQUEST:
        {
            string appId = "";
            string intent = "";
            string source = "";

            appId = params.HasLabel("appId") ? params["appId"].String() : "";
            if (appId.empty())
            {
                LOGERR("appId is missing or empty");
            }
            else
            {
                intent = params.HasLabel("intent") ? params["intent"].String() : "";
                if (intent.empty())
                {
                    LOGERR("intent is empty for Launch app");
                }
                source = params.HasLabel("source") ? params["source"].String() : "";
                if (source.empty())
                {
                    LOGERR("source is empty for Launch app");
                }
                mAdminLock.Lock();
                for (auto& notification : mAppManagerNotification)
                {
                    notification->OnAppLaunchRequest(appId, intent, source);
                }
                mAdminLock.Unlock();
            }
            break;
        }
        case APP_EVENT_UNLOADED:
        {
            string appId = "";
            string appInstanceId = "";

            appId = params.HasLabel("appId") ? params["appId"].String() : "";
            if (appId.empty())
            {
                LOGERR("appId is missing or empty");
            }
            else
            {
                appInstanceId = params.HasLabel("appInstanceId") ? params["appInstanceId"].String() : "";
                if (appInstanceId.empty())
                {
                    LOGERR("appInstanceId is empty for Unloaded app");
                }

                mAdminLock.Lock();
                Core::hresult unlockStatus = packageUnLock(appId);
                LOGINFO("package unlock triggered while sending unloaded event for appId %s status %d", appId.c_str(), unlockStatus);
                for (auto& notification : mAppManagerNotification)
                {
                    notification->OnAppUnloaded(appId, appInstanceId);
                }
                mAdminLock.Unlock();
                removeAppInfoByAppId(appId);
            }
            break;
        }
        default:
        LOGERR("Unknown event: %d", static_cast<int>(event));
        break;
    }

}

/*
 * @brief Notify client on App state change.
 * @Params[in]  : const string& appId, const string& appInstanceId, const AppLifecycleState newState, const AppLifecycleState oldState, const AppErrorReason errorReason
 * @Params[out] : None
 * @return      : void
 */
void AppManagerImplementation::handleOnAppLifecycleStateChanged(const string& appId, const string& appInstanceId, const Exchange::IAppManager::AppLifecycleState newState,
                                                 const Exchange::IAppManager::AppLifecycleState oldState, const Exchange::IAppManager::AppErrorReason errorReason)
{
    JsonObject eventDetails;

    eventDetails["appId"] = appId;
    eventDetails["appInstanceId"] = appInstanceId;
    eventDetails["newState"] = static_cast<int>(newState);
    eventDetails["oldState"] = static_cast<int>(oldState);
    eventDetails["errorReason"] = static_cast<int>(errorReason);

    LOGINFO("Notify App Lifecycle state change for appId %s: appInstanceId %s oldState=%d, newState=%d",
        appId.c_str(), (appInstanceId.empty()?" ":appInstanceId.c_str()), static_cast<int>(oldState), static_cast<int>(newState));

    dispatchEvent(APP_EVENT_LIFECYCLE_STATE_CHANGED, eventDetails);

    /* Notify MemoryMonitor of state changes so it can stop waiting */
    if (mMemoryMonitor)
    {
        mMemoryMonitor->OnAppStateChanged(appId, newState);
    }
}

/*
 * @brief Notify client on App Unloaded
 * @Params[in]  : const string& appId, const string& appInstanceId
 * @Params[out] : None
 * @return      : void
 */
void AppManagerImplementation::handleOnAppUnloaded(const string& appId, const string& appInstanceId)
{
    JsonObject eventDetails;

    if(appId.empty())
    {
        LOGERR("appId not present or empty");
    }
    else
    {
        eventDetails["appId"] = appId;
        eventDetails["appInstanceId"] = appInstanceId;

        LOGINFO("Notify App Lifecycle state change for appId %s: appInstanceId %s",
        appId.c_str(), appInstanceId.c_str());

        dispatchEvent(APP_EVENT_UNLOADED, eventDetails);
    }
}

/*
 * @brief Notify client on App LaunchRequest
 * @Params[in]  : const string& appId, const string& intent, const string &source
 * @Params[out] : None
 * @return      : void
 */
void AppManagerImplementation::handleOnAppLaunchRequest(const string& appId, const string& intent, const string& source)
{
    JsonObject eventDetails;

    if(appId.empty())
    {
        LOGERR("appId not present or empty");
    }
    else
    {
        eventDetails["appId"] = appId;
        eventDetails["intent"] = intent;
        eventDetails["source"] = source;

        LOGINFO("Notify onAppLaunchRequest for appId %s: intent=%s",
        appId.c_str(),intent.c_str());

        dispatchEvent(APP_EVENT_LAUNCH_REQUEST, eventDetails);
    }
}

uint32_t AppManagerImplementation::Configure(PluginHost::IShell* service)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (service != nullptr)
    {
        mCurrentservice = service;
        mCurrentservice->AddRef();

        if (nullptr == (mLifecycleInterfaceConnector = new LifecycleInterfaceConnector(mCurrentservice)))
        {
            LOGERR("Failed to create LifecycleInterfaceConnector");
        }
        else if (Core::ERROR_NONE != mLifecycleInterfaceConnector->createLifecycleManagerRemoteObject())
        {
            LOGERR("Failed to create LifecycleInterfaceConnector");
        }
        else
        {
            LOGINFO("created LifecycleManagerRemoteObject");
        }

        if (Core::ERROR_NONE != createPersistentStoreRemoteStoreObject())
        {
            LOGERR("Failed to create createPersistentStoreRemoteStoreObject");
        }
        else
        {
            LOGINFO("created createPersistentStoreRemoteStoreObject");
        }

        if (Core::ERROR_NONE != createPackageManagerObject())
        {
            LOGERR("Failed to create createPackageManagerObject");
        }
        else
        {
            LOGINFO("created createPackageManagerObject");
        }

        if (Core::ERROR_NONE != createStorageManagerRemoteObject())
        {
            LOGERR("Failed to create createStorageManagerRemoteObject");
        }
        else
        {
            LOGINFO("created createStorageManagerRemoteObject");
        }
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
        AppManagerTelemetryReporting::getInstance().initialize(service);
#endif
        /* Create and start MemoryMonitor */
        mMemoryMonitor = std::unique_ptr<MemoryMonitor>(new MemoryMonitor(this));
        mMemoryMonitor->Start();

        sRunning = true;
        /* Create the worker thread */
        try
        {
            mAppManagerWorkerThread = std::thread(&AppManagerImplementation::AppManagerWorkerThread, AppManagerImplementation::getInstance());
            LOGINFO("App Manager Worker thread created");
            result = Core::ERROR_NONE;
        }
        catch (const std::system_error& e)
        {
            LOGERR("Failed to create App Manager worker thread: %s", e.what());
        }
    }
    else
    {
        LOGERR("service is null \n");
    }

    return result;
}

Core::hresult AppManagerImplementation::createPersistentStoreRemoteStoreObject()
{
    Core::hresult status = Core::ERROR_GENERAL;

    if (nullptr == mCurrentservice)
    {
        LOGERR("mCurrentservice is null \n");
    }
    else if (nullptr == (mPersistentStoreRemoteStoreObject = mCurrentservice->QueryInterfaceByCallsign<WPEFramework::Exchange::IStore2>("org.rdk.PersistentStore")))
    {
        LOGERR("mPersistentStoreRemoteStoreObject is null \n");
    }
    else
    {
        LOGINFO("created PersistentStore Object\n");
        status = Core::ERROR_NONE;
    }
    return status;
}

void AppManagerImplementation::releasePersistentStoreRemoteStoreObject()
{
    ASSERT(nullptr != mPersistentStoreRemoteStoreObject);
    if(mPersistentStoreRemoteStoreObject)
    {
        mPersistentStoreRemoteStoreObject->Release();
        mPersistentStoreRemoteStoreObject = nullptr;
    }
}

Core::hresult AppManagerImplementation::createPackageManagerObject()
{
    Core::hresult status = Core::ERROR_GENERAL;

    if (nullptr == mCurrentservice)
    {
        LOGERR("mCurrentservice is null \n");
    }
    else if (nullptr == (mPackageManagerHandlerObject = mCurrentservice->QueryInterfaceByCallsign<WPEFramework::Exchange::IPackageHandler>("org.rdk.PackageManagerRDKEMS")))
    {
        LOGERR("mPackageManagerHandlerObject is null \n");
    }
    else if (nullptr == (mPackageManagerInstallerObject = mCurrentservice->QueryInterfaceByCallsign<WPEFramework::Exchange::IPackageInstaller>("org.rdk.PackageManagerRDKEMS")))
    {
        LOGERR("mPackageManagerInstallerObject is null \n");
    }
    else
    {
        LOGINFO("created PackageManager Object\n");
        mPackageManagerInstallerObject->AddRef();
        mPackageManagerInstallerObject->Register(&mPackageManagerNotification);
        status = Core::ERROR_NONE;
    }
    return status;
}

void AppManagerImplementation::releasePackageManagerObject()
{
    ASSERT(nullptr != mPackageManagerHandlerObject);
    if(mPackageManagerHandlerObject)
    {
        mPackageManagerHandlerObject->Release();
        mPackageManagerHandlerObject = nullptr;
    }
    ASSERT(nullptr != mPackageManagerInstallerObject);
    if(mPackageManagerInstallerObject)
    {
        mPackageManagerInstallerObject->Unregister(&mPackageManagerNotification);
        mPackageManagerInstallerObject->Release();
        mPackageManagerInstallerObject = nullptr;
    }
}

Core::hresult AppManagerImplementation::createStorageManagerRemoteObject()
{
     #define MAX_STORAGE_MANAGER_OBJECT_CREATION_RETRIES 2

    Core::hresult status = Core::ERROR_GENERAL;
    uint8_t retryCount = 0;

    if (nullptr == mCurrentservice)
    {
        LOGERR("mCurrentservice is null");
    }
    else
    {
        do
        {
            mStorageManagerRemoteObject = mCurrentservice->QueryInterfaceByCallsign<WPEFramework::Exchange::IAppStorageManager>("org.rdk.AppStorageManager");

            if (nullptr == mStorageManagerRemoteObject)
            {
                LOGERR("storageManagerRemoteObject is null (Attempt %d)", retryCount + 1);
                retryCount++;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            else
            {
                LOGINFO("Successfully created Storage Manager Object");
                status = Core::ERROR_NONE;
                break;
            }
        } while (retryCount < MAX_STORAGE_MANAGER_OBJECT_CREATION_RETRIES);

        if (status != Core::ERROR_NONE)
        {
            LOGERR("Failed to create Storage Manager Object after %d attempts", MAX_STORAGE_MANAGER_OBJECT_CREATION_RETRIES);
        }
    }
    return status;
}

void AppManagerImplementation::releaseStorageManagerRemoteObject()
{
    ASSERT(nullptr != mStorageManagerRemoteObject);
    if(mStorageManagerRemoteObject)
    {
        mStorageManagerRemoteObject->Release();
        mStorageManagerRemoteObject = nullptr;
    }
}

bool AppManagerImplementation::createOrUpdatePackageInfoByAppId(const string& appId, PackageInfo &packageData)
{
    bool result = false;
    if( packageData.version.empty() || packageData.unpackedPath.empty())
    {
        LOGWARN("AppId[%s] unpacked path is empty! or version is empty ", appId.c_str());
    }
    else
    {
        /* Check if the appId exists in the map */
        auto it = mAppInfo.find(appId);
        if (it != mAppInfo.end())
        {
            /* Update existing entry (PackageInfo inside LoadedAppInfo) */
            it->second.packageInfo.version         = packageData.version;
            it->second.packageInfo.lockId          = packageData.lockId;
            it->second.packageInfo.unpackedPath    = packageData.unpackedPath;
            it->second.packageInfo.configMetadata  = packageData.configMetadata;
            it->second.packageInfo.appMetadata     = packageData.appMetadata;

            LOGINFO("Existing package entry updated for appId: %s " \
                    "version: %s lockId: %d unpackedPath: %s appMetadata: %s",
                    appId.c_str(), it->second.packageInfo.version.c_str(),
                    it->second.packageInfo.lockId, it->second.packageInfo.unpackedPath.c_str(),
                    it->second.packageInfo.appMetadata.c_str());
        }
        else
        {
            /* Create new entry (PackageInfo inside LoadedAppInfo) */
            mAppInfo[appId].packageInfo.version         = packageData.version;
            mAppInfo[appId].packageInfo.lockId          = packageData.lockId;
            mAppInfo[appId].packageInfo.unpackedPath    = packageData.unpackedPath;
            mAppInfo[appId].packageInfo.configMetadata  = packageData.configMetadata;
            mAppInfo[appId].packageInfo.appMetadata     = packageData.appMetadata;

            LOGINFO("Created new package entry for appId: %s " \
                    "version: %s lockId: %d unpackedPath: %s appMetadata: %s",
                    appId.c_str(), mAppInfo[appId].packageInfo.version.c_str(),
                    mAppInfo[appId].packageInfo.lockId, mAppInfo[appId].packageInfo.unpackedPath.c_str(),
                    mAppInfo[appId].packageInfo.appMetadata.c_str());
        }
        result = true;
    }

    return result;
}

bool AppManagerImplementation::removeAppInfoByAppId(const string &appId)
{
    bool result = false;

    /* Check if appId StorageInfo is found, erase it */
    auto it = mAppInfo.find(appId);
    if (it != mAppInfo.end())
    {
        LOGINFO("Existing package entry updated for appId: %s " \
                "version: %s lockId: %d unpackedPath: %s appMetadata: %s",
                appId.c_str(), it->second.packageInfo.version.c_str(), it->second.packageInfo.lockId, it->second.packageInfo.unpackedPath.c_str(), it->second.packageInfo.appMetadata.c_str());
        mAppInfo.erase(appId);
        result = true;
    }
    else
    {
        LOGWARN("AppId[%s] PackageInfo not found", appId.c_str());
    }
    return result;
}
Core::hresult AppManagerImplementation::packageLock(const string& appId, PackageInfo &packageData, Exchange::IPackageHandler::LockReason lockReason)
{
    Core::hresult status = Core::ERROR_GENERAL;
    bool result = false;
    bool installed = false;
    bool loaded = false;
    std::vector<WPEFramework::Exchange::IPackageInstaller::Package> packageList;
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    AppManagerTelemetryReporting& appManagerTelemetryReporting =AppManagerTelemetryReporting::getInstance();
#endif

    if (nullptr != mLifecycleInterfaceConnector)
    {
        status = mLifecycleInterfaceConnector->isAppLoaded(appId, loaded);
    }

    /* Check if appId exists in the mAppInfo map */
    auto it = mAppInfo.find(appId);

    if ((status == Core::ERROR_NONE) &&
        (!loaded ||
         it == mAppInfo.end() ||
         (it->second.appNewState != Exchange::IAppManager::AppLifecycleState::APP_STATE_SUSPENDED && it->second.appNewState != Exchange::IAppManager::AppLifecycleState::APP_STATE_PAUSED && it->second.appNewState != Exchange::IAppManager::AppLifecycleState::APP_STATE_HIBERNATED)))
    {
        /* Fetch list of App packages */
        status = fetchAppPackageList(packageList);

        if (status == Core::ERROR_NONE)
        {
            /* Check if appId is installed */
            checkIsInstalled(appId, installed, packageList);

            if (installed)
            {
                /* Check if the packageId matches the provided appId */
                for (const auto& package : packageList)
                {
                    if (!package.packageId.empty() && package.packageId == appId && package.state == Exchange::IPackageInstaller::InstallState::INSTALLED)
                    {
                        packageData.version = std::string(package.version);
                        break;
                    }
                }
                LOGINFO("packageData call lock  %s", packageData.version.c_str());
                /* Ensure package version is valid before proceeding with the Lock */
                if ((nullptr != mPackageManagerHandlerObject) && !packageData.version.empty())
                {
                    Exchange::IPackageHandler::ILockIterator *appMetadata = nullptr;
                    status = mPackageManagerHandlerObject->Lock(appId, packageData.version, lockReason, packageData.lockId, packageData.unpackedPath, packageData.configMetadata, appMetadata);
                    if (status == Core::ERROR_NONE)
                    {
                        LOGINFO("Fetching package entry updated for appId: %s version: %s lockId: %d unpackedPath: %s appMetadata: %s",
                                 appId.c_str(), packageData.version.c_str(), packageData.lockId, packageData.unpackedPath.c_str(), packageData.appMetadata.c_str());
                        result = createOrUpdatePackageInfoByAppId(appId, packageData);

                        /* If package info update failed, set error status */
                        if (!result)
                        {
                            LOGERR("Failed to createOrUpdate the PackageInfo");
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
                            appManagerTelemetryReporting.reportTelemetryErrorData(appId, AppManagerImplementation::APP_ACTION_LAUNCH, AppManagerImplementation::ERROR_PACKAGE_INVALID);
#endif
                            status = Core::ERROR_GENERAL;
                        }
                    }
                    else
                    {
                        LOGERR("Failed to PackageManager Lock %s", appId.c_str());
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
                        appManagerTelemetryReporting.reportTelemetryErrorData(appId, AppManagerImplementation::APP_ACTION_LAUNCH, AppManagerImplementation::ERROR_PACKAGE_LOCK);
#endif
                        packageData.version.clear();  /* Clear version on failure */
                    }
                }
                else
                {
                    LOGERR("PackageManager handler is %s", (nullptr != mPackageManagerHandlerObject) ? "valid, but package version is empty" : "null");
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
                    CurrentActionError errorCode = (packageData.version.empty()?AppManagerImplementation::ERROR_PACKAGE_INVALID:AppManagerImplementation::ERROR_INTERNAL);
                    appManagerTelemetryReporting.reportTelemetryErrorData(appId, AppManagerImplementation::APP_ACTION_LAUNCH, errorCode);
#endif
                    status = Core::ERROR_GENERAL;
                }
            }
            else
            {
                LOGERR("isInstalled Failed for appId: %s", appId.c_str());
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
                appManagerTelemetryReporting.reportTelemetryErrorData(appId, AppManagerImplementation::APP_ACTION_LAUNCH, AppManagerImplementation::ERROR_PACKAGE_NOT_INSTALLED);
#endif
                status = Core::ERROR_GENERAL;
            }
        }
        else
        {
            LOGERR("Failed to Get the list of Packages");
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
            appManagerTelemetryReporting.reportTelemetryErrorData(appId, AppManagerImplementation::APP_ACTION_LAUNCH, AppManagerImplementation::ERROR_PACKAGE_LIST_FETCH);
#endif
            status = Core::ERROR_GENERAL;
        }
    }
    else
    {
        LOGERR("Skipping packageLock for appId %s: ", appId.c_str());
    }

    return status;
}

Core::hresult AppManagerImplementation::packageUnLock(const string& appId)
{
        Core::hresult status = Core::ERROR_GENERAL;
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
        AppManagerTelemetryReporting& appManagerTelemetryReporting =AppManagerTelemetryReporting::getInstance();
#endif

        auto it = mAppInfo.find(appId);
        if (it != mAppInfo.end())
        {
            if (nullptr != mPackageManagerHandlerObject)
            {
                status = mPackageManagerHandlerObject->Unlock(appId, it->second.packageInfo.version);

                if(Core::ERROR_NONE != status)
                {
                    LOGERR("Failed to PackageManager Unlock ");
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
                    appManagerTelemetryReporting.reportTelemetryErrorData(appId, AppManagerImplementation::APP_ACTION_CLOSE, AppManagerImplementation::ERROR_PACKAGE_UNLOCK);
#endif
                }
            }
            else
            {
                LOGERR("PackageManager handler is %s",((nullptr != mPackageManagerHandlerObject) ? "valid": "null"));
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
                appManagerTelemetryReporting.reportTelemetryErrorData(appId, AppManagerImplementation::APP_ACTION_CLOSE, AppManagerImplementation::ERROR_INTERNAL);
#endif
            }
        }
        else
        {
            LOGERR("AppId not found in map to get the version");
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
            appManagerTelemetryReporting.reportTelemetryErrorData(appId, AppManagerImplementation::APP_ACTION_CLOSE, AppManagerImplementation::ERROR_INVALID_PARAMS);
#endif
        }
        return status;
}
/*
 * @brief Launch App requested by client.
 * @Params[in]  : const string& appId , const string& intent, const string& launchArgs
 * @Params[out] : None
 * @return      : Core::hresult
 */
Core::hresult AppManagerImplementation::LaunchApp(const string& appId , const string& intent , const string& launchArgs)
{
    Core::hresult status = Core::ERROR_GENERAL;
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    AppManagerTelemetryReporting& appManagerTelemetryReporting =AppManagerTelemetryReporting::getInstance();
    time_t requestTime = appManagerTelemetryReporting.getCurrentTimestamp();
#endif
    LOGINFO(" LaunchApp enter with appId %s", appId.c_str());
    bool installed = false;
    Core::hresult result = IsInstalled(appId, installed);
    //IsInstalled(appId, installed);
    mAdminLock.Lock();
    if (appId.empty())
    {
        LOGERR("application Id is empty");
        status = Core::ERROR_INVALID_PARAMETER;
    }
    else if (result == Core::ERROR_NONE && !installed) {
        LOGERR("App %s is not installed. Cannot launch.", appId.c_str());
        status = Core::ERROR_GENERAL;
    }
    else if (result != Core::ERROR_NONE ) {
        LOGERR("fetchAppPackagelist is returing error for app %s.", appId.c_str());
        status = Core::ERROR_GENERAL;
    }
    else if (nullptr == mLifecycleInterfaceConnector) {
        LOGERR("LifecycleInterfaceConnector is null");
        status = Core::ERROR_GENERAL;
    }
    else if (nullptr != mLifecycleInterfaceConnector) {
        std::shared_ptr<AppManagerRequest> request = std::make_shared<AppManagerRequest>();

        if (request != nullptr)
        {
            LOGINFO("LaunchApp enter with appId %s", appId.c_str());
            request->mRequestAction = APP_ACTION_LAUNCH;
            request->mRequestParam = std::make_shared<AppLaunchRequestParam>(AppLaunchRequestParam{appId, launchArgs, intent});
            if (request->mRequestParam != nullptr)
            {
                mAppManagerLock.lock();
                mAppRequestList.push_back(request);
                mAppManagerLock.unlock();
                mAppRequestListCV.notify_one();
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("Failed to perform operation due to no memory");
            }
        }
        else
        {
            LOGERR("Failed to perform operation due to no memory");
        }
    }
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    if(status == Core::ERROR_NONE)
    {
        updateCurrentActionTime(appId, requestTime, AppManagerImplementation::APP_ACTION_LAUNCH);
        appManagerTelemetryReporting.reportTelemetryData(appId, AppManagerImplementation::APP_ACTION_LAUNCH);
    }
#endif

    mAdminLock.Unlock();

    LOGINFO(" LaunchApp returns with status %d", status);
    return status;
}

/*
 * This function handles the process of closing an app by moving it from the ACTIVE state to the RUNNING state.
 * The state change is performed by the Lifecycle Manager.
 * If the app's state is not ACTIVE, the function does nothing.
 *
 * Input:
 * - const string& appId: The ID of the application to be closed.
 *
 * Output:
 * - Returns a Core::hresult status code:
 *     - Core::ERROR_NONE on success.
 *     - Core::ERROR_GENERAL on error.
 */
Core::hresult AppManagerImplementation::CloseApp(const string& appId)
{
    Core::hresult status = Core::ERROR_GENERAL;
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    AppManagerTelemetryReporting& appManagerTelemetryReporting =AppManagerTelemetryReporting::getInstance();
    time_t requestTime = appManagerTelemetryReporting.getCurrentTimestamp();
#endif
    LOGINFO("CloseApp Entered with appId %s", appId.c_str());

    if (!appId.empty())
    {
        mAdminLock.Lock();
        if (nullptr != mLifecycleInterfaceConnector)
        {
            status = mLifecycleInterfaceConnector->closeApp(appId);
        }
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
        if(status == Core::ERROR_NONE)
        {
            updateCurrentActionTime(appId, requestTime, AppManagerImplementation::APP_ACTION_CLOSE);
            appManagerTelemetryReporting.reportTelemetryData(appId, AppManagerImplementation::APP_ACTION_CLOSE);
        }
#endif
        mAdminLock.Unlock();
    }
    else
    {
        LOGERR("appId is empty");
    }
    return status;
}

/*
 * This function Starts terminating a running app. Lifecycle Manager will move the state into
 * TERMINATE_REQUESTED
 *.This will trigger a clean shutdown
 *
 * Input:
 * - const string& appId: The ID of the application to be closed.
 *
 * Output:
 * - Returns a Core::hresult status code:
 *     - Core::ERROR_NONE on success.
 *     - Core::ERROR_GENERAL on error.
 */
Core::hresult AppManagerImplementation::TerminateApp(const string& appId )
{
    Core::hresult status = Core::ERROR_GENERAL;
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    AppManagerTelemetryReporting& appManagerTelemetryReporting =AppManagerTelemetryReporting::getInstance();
    time_t requestTime = appManagerTelemetryReporting.getCurrentTimestamp();
#endif
    LOGINFO(" TerminateApp Entered with appId %s", appId.c_str());

    if (!appId.empty())
    {
        mAdminLock.Lock();
        if (nullptr != mLifecycleInterfaceConnector)
        {
            status = mLifecycleInterfaceConnector->terminateApp(appId);
        }
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
        if(status == Core::ERROR_NONE)
        {
            updateCurrentActionTime(appId, requestTime, AppManagerImplementation::APP_ACTION_TERMINATE);
            appManagerTelemetryReporting.reportTelemetryData(appId, AppManagerImplementation::APP_ACTION_TERMINATE);
        }
#endif
        mAdminLock.Unlock();
    }
    else
    {
        LOGERR("appId is empty");
    }
    return status;
}

Core::hresult AppManagerImplementation::KillApp(const string& appId)
{
    Core::hresult status = Core::ERROR_NONE;
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    AppManagerTelemetryReporting& appManagerTelemetryReporting =AppManagerTelemetryReporting::getInstance();
    time_t requestTime = appManagerTelemetryReporting.getCurrentTimestamp();
#endif
    LOGINFO("KillApp entered appId: '%s'", appId.c_str());

    mAdminLock.Lock();
    if (nullptr != mLifecycleInterfaceConnector)
    {
        status = mLifecycleInterfaceConnector->killApp(appId);
    }
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    if(status == Core::ERROR_NONE)
    {
        updateCurrentActionTime(appId, requestTime, AppManagerImplementation::APP_ACTION_KILL);
        appManagerTelemetryReporting.reportTelemetryData(appId, AppManagerImplementation::APP_ACTION_KILL);
    }
#endif
    mAdminLock.Unlock();

    LOGINFO("KillApp exited");
    return status;
}

Core::hresult AppManagerImplementation::GetLoadedApps(Exchange::IAppManager::ILoadedAppInfoIterator*& appData)
{
    Core::hresult status = Core::ERROR_GENERAL;
    LOGINFO("GetLoadedApps Entered");
    mAdminLock.Lock();
    if (nullptr != mLifecycleInterfaceConnector)
    {
        status = mLifecycleInterfaceConnector->getLoadedApps(appData);
    }
    mAdminLock.Unlock();

    LOGINFO(" GetLoadedApps returns with status %d", status);
    return status;
}

Core::hresult AppManagerImplementation::SendIntent(const string& appId , const string& intent)
{
    Core::hresult status = Core::ERROR_NONE;

    LOGINFO("SendIntent entered with appId '%s' and intent '%s'", appId.c_str(), intent.c_str());

    mAdminLock.Lock();
    if (nullptr != mLifecycleInterfaceConnector)
    {
        status = mLifecycleInterfaceConnector->sendIntent(appId, intent);
    }
    mAdminLock.Unlock();

    LOGINFO("SendIntent exited");
    return status;
}

/***
 * @brief Preloads an Application.
 * Preloads an Application and app will be in the RUNNING state (hidden).
 *
 * @param[in] appId     : App identifier for the application
 * @param[in] launchArgs(optional) : Additional parameters passed to the application
 * @param[out] error : if success = false it holds the appropriate error reason
 *
 * @return              : Core::<StatusCode>
 */
Core::hresult AppManagerImplementation::PreloadApp(const string& appId , const string& launchArgs ,string& error)
{
    Core::hresult status = Core::ERROR_GENERAL;
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    AppManagerTelemetryReporting& appManagerTelemetryReporting = AppManagerTelemetryReporting::getInstance();
    time_t requestTime = appManagerTelemetryReporting.getCurrentTimestamp();
#endif
    LOGINFO(" PreloadApp enter with appId %s", appId.c_str());

    /* Early validation (no lock needed) */
    if (appId.empty())
    {
        LOGERR("application Id is empty");
        error = "application Id is empty";
        LOGINFO(" PreloadApp returns with status %d", Core::ERROR_INVALID_PARAMETER);
        return Core::ERROR_INVALID_PARAMETER;
    }

    /* MemoryMonitor: Blocking pre-preload memory preparation.
     * Ensures sufficient MemoryAvailable (from /proc/meminfo) before scheduling the preload.
     * Uses maxSuspendedSystemMemory from app config as the target.
     * Blocks until the required memory is available or all reclamation options are exhausted. */
    if (mMemoryMonitor)
    {
        uint32_t launchTargetKB = 0;
        uint32_t preloadTargetKB = 0;
        GetAppMemoryConfig(appId, launchTargetKB, preloadTargetKB);
        LOGINFO("[MemoryMonitor] Triggering pre-preload blocking reconciliation for appId %s, target %u KB",
                appId.c_str(), preloadTargetKB);
        mMemoryMonitor->RequestReconciliationAndWait(appId, preloadTargetKB);
    }

    mAdminLock.Lock();
    if (nullptr != mLifecycleInterfaceConnector)
    {
        std::shared_ptr<AppManagerRequest> request = std::make_shared<AppManagerRequest>();

        if (request != nullptr)
        {
            LOGINFO(" PreloadApp scheduling with appId %s", appId.c_str());
            request->mRequestAction = APP_ACTION_PRELOAD;
            request->mRequestParam = std::make_shared<AppLaunchRequestParam>(AppLaunchRequestParam{appId, launchArgs});
            if (request->mRequestParam != nullptr)
            {
                mAppManagerLock.lock();
                mAppRequestList.push_back(request);
                mAppManagerLock.unlock();
                mAppRequestListCV.notify_one();
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("Failed to perform operation due to no memory");
                error = "Failed to perform operation due to no memory";
            }
        }
        else
        {
            LOGERR("Failed to perform operation due to no memory");
            error = "Failed to perform operation due to no memory";
        }
    }
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    if(status == Core::ERROR_NONE)
    {
        updateCurrentActionTime(appId, requestTime, AppManagerImplementation::APP_ACTION_PRELOAD);
        appManagerTelemetryReporting.reportTelemetryData(appId, AppManagerImplementation::APP_ACTION_PRELOAD);
    }
#endif

    mAdminLock.Unlock();

    LOGINFO(" PreloadApp returns with status %d", status);
    return status;
}

/***
 * @brief Gets a property for a given app.
 * Gets a property for a given app based on the appId and key.
 *
 * @param[in] appId     : App identifier for the application
 * @param[in] key       : the name of the property to get
 * @param[out] value    : value of the key queried, this can be a boolean, number, string or object type
 *
 * @return              : Core::<StatusCode>
 */
Core::hresult AppManagerImplementation::GetAppProperty(const string& appId , const string& key , string& value )
{
    LOGINFO("GetAppProperty Entered");
    Core::hresult status = Core::ERROR_GENERAL;
    uint32_t ttl = 0;
    if(appId.empty())
    {
        LOGERR("Empty appId");
    }
    else if(key.empty())
    {
        LOGERR("Empty Key");
    }
    else
    {
        mAdminLock.Lock();
        /* Checking if mPersistentStoreRemoteStoreObject is not valid then create the object, not required to destroy this object as it is handled in Destructor */
        if (nullptr == mPersistentStoreRemoteStoreObject)
        {
            LOGINFO("Create PersistentStore Remote store object");
            if (Core::ERROR_NONE != createPersistentStoreRemoteStoreObject())
            {
                LOGERR("Failed to create createPersistentStoreRemoteStoreObject");
            }
        }
        ASSERT (nullptr != mPersistentStoreRemoteStoreObject);
        if (nullptr != mPersistentStoreRemoteStoreObject)
        {
            status = mPersistentStoreRemoteStoreObject->GetValue(Exchange::IStore2::ScopeType::DEVICE, appId, key, value, ttl);
            LOGINFO("Key[%s] value[%s] status[%d]", key.c_str(), value.c_str(), status);
            if (Core::ERROR_NONE != status)
            {
                LOGERR("GetValue Failed");
            }
        }
        else
        {
            LOGERR("PersistentStore object is not valid");
        }
        mAdminLock.Unlock();
    }
    LOGINFO("GetAppProperty Exited");

    return status;
}

/***
 * @brief Sets a property for a given app.
 * Sets a property for a given app based with given namespace, key and Value to persistentStore.
 *
 * @param[in] appId     : App identifier for the application
 * @param[in] key       : the name of the property to get
 * @param[in] value     : the property value(json string format) to set, this can be a boolean, number, string or object type
 *
 * @return              : Core::<StatusCode>
 */
Core::hresult AppManagerImplementation::SetAppProperty(const string& appId, const string& key, const string& value)
{
    Core::hresult status = Core::ERROR_GENERAL;

    if (appId.empty())
    {
        LOGERR("appId is empty");
    }
    else if (key.empty())
    {
        LOGERR("key is empty");
    }
    else
    {
        mAdminLock.Lock();

        /* Checking if mPersistentStoreRemoteStoreObject is not valid then create the object */
        if (nullptr == mPersistentStoreRemoteStoreObject)
        {
            LOGINFO("Create PersistentStore Remote store object");
            if (Core::ERROR_NONE != createPersistentStoreRemoteStoreObject())
            {
                LOGERR("Failed to create createPersistentStoreRemoteStoreObject");
            }
        }

        ASSERT (nullptr != mPersistentStoreRemoteStoreObject);
        if (nullptr != mPersistentStoreRemoteStoreObject)
        {
            status = mPersistentStoreRemoteStoreObject->SetValue(Exchange::IStore2::ScopeType::DEVICE, appId, key, value, 0);
        }
        else
        {
            LOGERR("PersistentStore object is not valid");
        }

        mAdminLock.Unlock();
    }
    return status;
}

Core::hresult AppManagerImplementation::fetchAppPackageList(std::vector<WPEFramework::Exchange::IPackageInstaller::Package>& packageList)
{
    Core::hresult status = Core::ERROR_GENERAL;
    packageList.clear();
    if (nullptr == mPackageManagerInstallerObject)
    {
        LOGINFO("Create PackageManager Remote store object");
        if (Core::ERROR_NONE != createPackageManagerObject())
        {
            LOGERR("Failed to create PackageManagerObject");
        }
    }
    ASSERT (nullptr != mPackageManagerInstallerObject);
    if (mPackageManagerInstallerObject != nullptr)
    {
        Exchange::IPackageInstaller::IPackageIterator* packages = nullptr;

        if (mPackageManagerInstallerObject->ListPackages(packages) != Core::ERROR_NONE || packages == nullptr)
        {
            LOGERR("ListPackage is returning Error or Packages is nullptr");
            goto End;
        }

        WPEFramework::Exchange::IPackageInstaller::Package package;
        while (packages->Next(package)) {
            packageList.push_back(package);
        }
        status = Core::ERROR_NONE;
        packages->Release();
    }
    else
    {
        LOGERR("mPackageManagerInstallerObject is null ");
    }
End:
    return status;
}

std::string AppManagerImplementation::getInstallAppType(ApplicationType type)
{
    switch (type)
    {
        case ApplicationType::APPLICATION_TYPE_INTERACTIVE : return "INTERACTIVE_APP";
        case ApplicationType::APPLICATION_TYPE_SYSTEM : return "SYSTEM_APP";
        default: return "";
    }
}

Core::hresult AppManagerImplementation::GetInstalledApps(std::string& apps)
{
    Core::hresult status = Core::ERROR_GENERAL;
    apps.clear();
    std::vector<WPEFramework::Exchange::IPackageInstaller::Package> packageList;
    JsonArray installedAppsArray;
    char timeData[TIME_DATA_SIZE];

    mAdminLock.Lock();

    status = fetchAppPackageList(packageList);
    if (status == Core::ERROR_NONE)
    {
        for (const auto& pkg : packageList)
        {
            /* Proceed only if the package is in the INSTALLED state */
            if(pkg.state == Exchange::IPackageInstaller::InstallState::INSTALLED)
            {
                JsonObject package;
                package["appId"] = pkg.packageId;
                package["versionString"] = pkg.version;
                package["type"] = getInstallAppType(APPLICATION_TYPE_INTERACTIVE);
                auto it = mAppInfo.find(pkg.packageId);
                if (it != mAppInfo.end())
                {
                    const auto& timestamp = it->second.lastActiveStateChangeTime;

                    if (strftime(timeData, sizeof(timeData), "%D %T", gmtime(&timestamp.tv_sec)))
                    {
                        std::ostringstream lastActiveTime;
                        lastActiveTime << timeData << "." << std::setw(9) << std::setfill('0') << timestamp.tv_nsec;
                        package["lastActiveTime"] = lastActiveTime.str();
                    }
                    else
                    {
                        package["lastActiveTime"] = "";
                    }
                    package["lastActiveIndex"]=it->second.lastActiveIndex;
                }
                else
                {
                    package["lastActiveTime"] ="";
                    package["lastActiveIndex"]="";
                }
                installedAppsArray.Add(package);
            }
        }
        installedAppsArray.ToString(apps);
        LOGINFO("getInstalledApps: %s", apps.c_str());
    }

    mAdminLock.Unlock();

    return status;
}

/* Method to check if the app is installed */
void AppManagerImplementation::checkIsInstalled(const std::string& appId, bool& installed, const std::vector<WPEFramework::Exchange::IPackageInstaller::Package>& packageList)
{
    installed = false;

    for (const auto& package : packageList)
    {
        /* Check if the package matches the appId and is in the INSTALLED state */
        if ((!package.packageId.empty()) && (package.packageId == appId) && (package.state == Exchange::IPackageInstaller::InstallState::INSTALLED))
        {
            LOGINFO("%s is installed ",appId.c_str());
            installed = true;
            break;
        }
    }
    return;
}

Core::hresult AppManagerImplementation::IsInstalled(const std::string& appId, bool& installed)
{
    Core::hresult status = Core::ERROR_GENERAL;
    installed = false;

    mAdminLock.Lock();
    if(appId.empty())
    {
        LOGERR("appId not present or empty");
    }
    else
    {
        std::vector<WPEFramework::Exchange::IPackageInstaller::Package> packageList;
        status = fetchAppPackageList(packageList);
        if (status == Core::ERROR_NONE)
        {
            checkIsInstalled(appId, installed, packageList);
            if(installed)
            {
                LOGINFO("%s is installed ",appId.c_str());
            }
            else
            {
                LOGINFO("%s is not installed ",appId.c_str());
            }
        }
    }
    mAdminLock.Unlock();
    return status;
}

Core::hresult AppManagerImplementation::StartSystemApp(const string& appId)
{
    Core::hresult status = Core::ERROR_NONE;

    LOGINFO("StartSystemApp Entered");

    return status;
}

Core::hresult AppManagerImplementation::StopSystemApp(const string& appId)
{
    Core::hresult status = Core::ERROR_NONE;

    LOGINFO("StopSystemApp Entered");

    return status;
}

Core::hresult AppManagerImplementation::ClearAppData(const string& appId)
{
    Core::hresult status = Core::ERROR_NONE;

    LOGINFO("ClearAppData Entered");

    if (appId.empty())
    {
        LOGERR("appId is empty");
        status = Core::ERROR_GENERAL;
        return status;
    }

    mAdminLock.Lock(); //required??
    if (nullptr != mStorageManagerRemoteObject)
    {
        std::string errorReason;
        status = mStorageManagerRemoteObject->Clear(appId,errorReason);
        if (status != Core::ERROR_NONE)
        {
            LOGERR("Failed to clear app data for appId: %s errorReason : %s", appId.c_str(), errorReason.c_str());
        }
    }
    else
    {
        LOGERR("StorageManager Remote Object is null");
        status = Core::ERROR_GENERAL;
    }
    mAdminLock.Unlock();

    return status;
}

Core::hresult AppManagerImplementation::ClearAllAppData()
{
    Core::hresult status = Core::ERROR_NONE;

    LOGINFO("ClearAllAppData Entered");

    mAdminLock.Lock(); //required??
    if (nullptr != mStorageManagerRemoteObject)
    {
        std::string errorReason;
        std::string exemptedAppIds = "";
        status = mStorageManagerRemoteObject->ClearAll(exemptedAppIds,errorReason);
        if (status != Core::ERROR_NONE)
        {
            LOGERR("Failed to clear all app data errorReason : %s", errorReason.c_str());
        }
    }
    else
    {
        LOGERR("StorageManager Remote Object is null");
        status = Core::ERROR_GENERAL;
    }
    mAdminLock.Unlock();

    return status;
}

Core::hresult AppManagerImplementation::GetAppMetadata(const string& appId, const string& metaData, string& result)
{
    Core::hresult status = Core::ERROR_NONE;

    LOGINFO("GetAppMetadata Entered");

    return status;
}

Core::hresult AppManagerImplementation::GetMaxRunningApps(int32_t& maxRunningApps) const
{
    LOGINFO("GetMaxRunningApps Entered");

    maxRunningApps = -1;

    return Core::ERROR_NONE;
}

Core::hresult AppManagerImplementation::GetMaxHibernatedApps(int32_t& maxHibernatedApps) const
{
    LOGINFO("GetMaxHibernatedApps Entered");

    maxHibernatedApps = -1;

    return Core::ERROR_NONE;
}

Core::hresult AppManagerImplementation::GetMaxHibernatedFlashUsage(int32_t& maxHibernatedFlashUsage) const
{
    LOGINFO("GetMaxHibernatedFlashUsage Entered");

    maxHibernatedFlashUsage = -1;

    return Core::ERROR_NONE;
}

Core::hresult AppManagerImplementation::GetMaxInactiveRamUsage(int32_t& maxInactiveRamUsage) const
{
    LOGINFO("GetMaxInactiveRamUsage Entered");

    maxInactiveRamUsage = -1;

    return Core::ERROR_NONE;
}

void AppManagerImplementation::OnAppInstallationStatus(const string& jsonresponse)
{
    LOGINFO("Received JSON response: %s", jsonresponse.c_str());

    if (!jsonresponse.empty())
    {
        JsonArray list;
        if (list.FromString(jsonresponse))
        {
            for (size_t i = 0; i < list.Length(); ++i)
            {
                JsonObject params = list[i].Object();
                dispatchEvent(APP_EVENT_INSTALLATION_STATUS, params);
            }
        }
        else
        {
            LOGERR("Failed to parse JSON string\n");
        }
    }
    else
    {
        LOGERR("jsonresponse string is empty");
    }
}

void AppManagerImplementation::getCustomValues(WPEFramework::Exchange::RuntimeConfig& runtimeConfig)
{
        FILE* fp = fopen("/tmp/aipath", "r");
        bool aipathchange = false;
        std::string apppath, runtimepath, command;
        if (fp != NULL)
        {
            aipathchange = true;
            char* line = NULL;
            size_t len = 0;
            bool first = true, second = true, third = true;
            while ((getline(&line, &len, fp)) != -1)
            {
                if (first)
                {
                    apppath = line;
                    first = false;
                }
                else if (second)
                {
                    runtimepath = line;
                    second = false;
                }
                else if (third)
                {
                    command = line;
                    third = false;
                }
            }
            fclose(fp);
        }
        apppath.pop_back();
        runtimepath.pop_back();
        command.pop_back();

        if (aipathchange)
        {
            runtimeConfig.appPath = apppath;
            runtimeConfig.runtimePath = runtimepath;
            runtimeConfig.command = command;
            runtimeConfig.appType = 1;
            runtimeConfig.resourceManagerClientEnabled = true;
        }
}

void AppManagerImplementation::updateCurrentAction(const std::string& appId, CurrentAction action)
{
    auto it = mAppInfo.find(appId);
    if(it != mAppInfo.end())
    {
        it->second.currentAction = action;
        LOGINFO("Updated currentAction for appId %s to %d", appId.c_str(), static_cast<int>(action));
    }
    else
    {
        LOGERR("App ID %s not found while updating currentAction", appId.c_str());
    }
}
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
void AppManagerImplementation::updateCurrentActionTime(const std::string& appId, time_t currentActionTime, CurrentAction currentAction)
{
    auto it = mAppInfo.find(appId);
    if((it != mAppInfo.end()) && (currentAction == it->second.currentAction))
    {
        it->second.currentActionTime = currentActionTime;
        LOGINFO("Updated currentActionTime for appId %s", appId.c_str());
    }
    else
    {
        LOGERR("Failed to updating currentActionTime for appId %s", appId.c_str());
    }
}
#endif

bool AppManagerImplementation::checkInstallUninstallBlock(const std::string& appId)
{
    bool blocked = false;
    int duplicateCount = 0;

    std::vector<WPEFramework::Exchange::IPackageInstaller::Package> packageList;
    Core::hresult status = fetchAppPackageList(packageList);

    if (status != Core::ERROR_NONE) {
        LOGERR("Failed to fetch package list for appId: %s", appId.c_str());
        return false;
    }

    for (const auto& package : packageList) {
        if ((!package.packageId.empty()) && (package.packageId == appId)) {
            duplicateCount++;
            if (package.state == Exchange::IPackageInstaller::InstallState::INSTALLATION_BLOCKED ||
                package.state == Exchange::IPackageInstaller::InstallState::UNINSTALL_BLOCKED) {
                blocked = true;
                LOGINFO("checkInstallUninstallBlock: appId=%s,package_state=%d", appId.c_str(), static_cast<int>(package.state));
                break;
            }
        }
    }
    LOGINFO("checkInstallUninstallBlock: appId=%s duplicateCount=%d blocked=%d", appId.c_str(), duplicateCount, blocked);
    return blocked;
}

/*
 * @brief Read MemoryAvailable from /proc/meminfo.
 * @return Available memory in KB, or 0 on failure.
 */
uint32_t AppManagerImplementation::ReadMemAvailable()
{
    uint32_t memAvailableKB = 0;
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open())
    {
        std::string line;
        while (std::getline(meminfo, line))
        {
            if (line.find("MemAvailable:") == 0)
            {
                std::istringstream iss(line);
                std::string label;
                uint32_t value;
                iss >> label >> value;
                memAvailableKB = value;
                break;
            }
        }
        meminfo.close();
    }
    else
    {
        LOGERR("[MemoryMonitor] Failed to open /proc/meminfo");
    }
    return memAvailableKB;
}

/*
 * @brief Log a snapshot of all managed apps and their states.
 */
void AppManagerImplementation::LogManagedAppsSnapshot()
{
    mAdminLock.Lock();
    LOGINFO("[MemoryMonitor] --- Managed Apps Snapshot ---");
    for (const auto& entry : mAppInfo)
    {
        long elapsed = GetElapsedTimeSeconds(entry.second.lastActiveStateChangeTime);
        LOGINFO("  - App: %s, State: %d, Seconds in state: %ld",
                entry.first.c_str(), static_cast<int>(entry.second.appNewState), elapsed);
    }
    LOGINFO("[MemoryMonitor] ------------------------------");
    mAdminLock.Unlock();
}

/*
 * @brief Get sorted list of app IDs in the given state, oldest first (by lastActiveStateChangeTime).
 * @param state The lifecycle state to filter by.
 * @return Vector of app IDs sorted oldest-first.
 */
std::vector<string> AppManagerImplementation::GetSortedCandidates(Exchange::IAppManager::AppLifecycleState state)
{
    std::vector<std::pair<string, struct timespec>> candidates;

    mAdminLock.Lock();
    for (const auto& entry : mAppInfo)
    {
        if (entry.second.appNewState == state)
        {
            candidates.push_back(std::make_pair(entry.first, entry.second.lastActiveStateChangeTime));
        }
    }
    mAdminLock.Unlock();

    /* Sort by time, oldest first (smallest tv_sec first) */
    std::sort(candidates.begin(), candidates.end(),
        [](const std::pair<string, struct timespec>& a, const std::pair<string, struct timespec>& b) {
            if (a.second.tv_sec != b.second.tv_sec)
                return a.second.tv_sec < b.second.tv_sec;
            return a.second.tv_nsec < b.second.tv_nsec;
        });

    std::vector<string> result;
    result.reserve(candidates.size());
    for (const auto& c : candidates)
    {
        result.push_back(c.first);
    }
    return result;
}

/*
 * @brief Get the lastActiveStateChangeTime for an app.
 * @param appId The application ID.
 * @return The timespec of the last state change, or {0,0} if not found.
 */
struct timespec AppManagerImplementation::GetAppStateTime(const string& appId)
{
    struct timespec ts = {0, 0};
    mAdminLock.Lock();
    auto it = mAppInfo.find(appId);
    if (it != mAppInfo.end())
    {
        ts = it->second.lastActiveStateChangeTime;
    }
    mAdminLock.Unlock();
    return ts;
}

/*
 * @brief Get elapsed seconds since a given timespec.
 * @param startTime The reference start time.
 * @return Elapsed seconds.
 */
long AppManagerImplementation::GetElapsedTimeSeconds(struct timespec startTime)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return (now.tv_sec - startTime.tv_sec);
}

/*
 * @brief Get the current lifecycle state of an app.
 * @param appId The application ID.
 * @return The current AppLifecycleState, or APP_STATE_UNKNOWN if not found.
 */
Exchange::IAppManager::AppLifecycleState AppManagerImplementation::GetAppState(const string& appId)
{
    Exchange::IAppManager::AppLifecycleState state = Exchange::IAppManager::APP_STATE_UNKNOWN;
    mAdminLock.Lock();
    auto it = mAppInfo.find(appId);
    if (it != mAppInfo.end())
    {
        state = it->second.appNewState;
    }
    mAdminLock.Unlock();
    return state;
}

/*
 * @brief Request hibernation transaction for an app via LifecycleManager.
 *        Sets the app's target state to HIBERNATED using SetTargetAppState.
 * @param appId The application ID.
 * @return Core::hresult status code.
 */
Core::hresult AppManagerImplementation::HibernateApp(const string& appId)
{
    Core::hresult status = Core::ERROR_GENERAL;

    LOGINFO("[MemoryMonitor] HibernateApp entered for appId: %s", appId.c_str());

    if (!appId.empty())
    {
        mAdminLock.Lock();
        if (nullptr != mLifecycleInterfaceConnector)
        {
            status = mLifecycleInterfaceConnector->hibernateApp(appId);
        }
        else
        {
            LOGERR("[MemoryMonitor] LifecycleInterfaceConnector is null");
        }
        mAdminLock.Unlock();
    }
    else
    {
        LOGERR("[MemoryMonitor] appId is empty");
    }
    return status;
}

/*
 * @brief Request suspension transition for an app via LifecycleManager.
 *        Sets the app's target state to SUSPENDED using SetTargetAppState.
 * @param appId The application ID.
 * @return Core::hresult status code.
 */
Core::hresult AppManagerImplementation::SuspendApp(const string& appId)
{
    Core::hresult status = Core::ERROR_GENERAL;

    LOGINFO("[MemoryMonitor] SuspendApp entered for appId: %s", appId.c_str());

    if (!appId.empty())
    {
        mAdminLock.Lock();
        if (nullptr != mLifecycleInterfaceConnector)
        {
            status = mLifecycleInterfaceConnector->suspendApp(appId);
        }
        else
        {
            LOGERR("[MemoryMonitor] LifecycleInterfaceConnector is null");
        }
        mAdminLock.Unlock();
    }
    else
    {
        LOGERR("[MemoryMonitor] appId is empty");
    }
    return status;
}

/*
 * @brief Check if an app was loaded via PreloadApp (not LaunchApp).
 *        Preloaded apps are protected from being killed during memory reclamation;
 *        they may only be hibernated (if supported).
 * @param appId The application ID.
 * @return true if the app was preloaded.
 */
bool AppManagerImplementation::IsPreloadedApp(const string& appId)
{
    bool result = false;
    mAdminLock.Lock();
    auto it = mAppInfo.find(appId);
    if (it != mAppInfo.end())
    {
        result = it->second.wasPreloaded;
    }
    mAdminLock.Unlock();
    return result;
}

/*
 * @brief Check if hibernation is supported on this platform.
 *        Hibernation support is determined by the existence of a policy file:
 *        /tmp/AI2.0Hibernatable  (same mechanism used by LifecycleInterfaceConnector).
 * @return true if hibernation is supported.
 */
bool AppManagerImplementation::IsHibernationSupported()
{
    struct stat fileStat;
    return (stat("/tmp/AI2.0Hibernatable", &fileStat) == 0 && S_ISREG(fileStat.st_mode));
}

/*
 * @brief Parse a memory size string (e.g. "256M", "16M", "1G", "512K") to KB.
 *        Supported suffixes: K/k (kilobytes), M/m (megabytes), G/g (gigabytes).
 *        If no suffix, the value is assumed to be in megabytes.
 * @param memStr The memory size string to parse.
 * @return The size in KB, or 0 if parsing fails.
 */
uint32_t AppManagerImplementation::ParseMemorySizeToKB(const string& memStr)
{
    if (memStr.empty())
    {
        return 0;
    }

    uint32_t value = 0;
    char suffix = 'M'; /* Default to megabytes */

    /* Try to parse the numeric portion */
    std::istringstream iss(memStr);
    if (!(iss >> value))
    {
        LOGERR("[MemoryMonitor] Failed to parse memory size string: %s", memStr.c_str());
        return 0;
    }

    /* Check for suffix */
    if (!iss.eof())
    {
        iss >> suffix;
    }

    switch (suffix)
    {
        case 'K':
        case 'k':
            /* Already in KB */
            break;
        case 'M':
        case 'm':
            value *= 1024;
            break;
        case 'G':
        case 'g':
            value *= 1024 * 1024;
            break;
        default:
            /* Assume megabytes for unknown suffix */
            value *= 1024;
            LOGWARN("[MemoryMonitor] Unknown memory suffix '%c' in '%s', assuming MB", suffix, memStr.c_str());
            break;
    }

    return value;
}

/*
 * @brief Retrieve memory configuration for an app from the PackageManager config JSON.
 *        The config JSON (from PackageManager::Config) contains:
 *          - configuration."urn:rdk:config:memory"."system" (e.g. "256M") -> launch target
 *          - configuration."urn:rdk:config:application-lifecycle"."maxSuspendedSystemMemory" (e.g. "16M") -> preload target
 *
 *        This method:
 *          1. Gets the app version from the installed package list
 *          2. Calls IPackageInstaller::Config() to retrieve RuntimeConfig
 *          3. Uses RuntimeConfig.systemMemoryLimit for the launch target
 *          4. Reads the raw package config JSON (from unpackedPath) for maxSuspendedSystemMemory
 *
 * @param appId           The application ID.
 * @param launchTargetKB  [out] Launch memory target in KB (from urn:rdk:config:memory.system).
 * @param preloadTargetKB [out] Preload memory target in KB (from urn:rdk:config:application-lifecycle.maxSuspendedSystemMemory).
 * @return true if at least the launch target was retrieved successfully.
 */
bool AppManagerImplementation::GetAppMemoryConfig(const string& appId, uint32_t& launchTargetKB, uint32_t& preloadTargetKB)
{
    /* Defaults */
    const uint32_t DEFAULT_LAUNCH_TARGET_KB  = 256 * 1024;  /* 256 MB */
    const uint32_t DEFAULT_PRELOAD_TARGET_KB = 16 * 1024;   /* 16 MB  */
    launchTargetKB  = DEFAULT_LAUNCH_TARGET_KB;
    preloadTargetKB = DEFAULT_PRELOAD_TARGET_KB;

    bool result = false;

    if (appId.empty())
    {
        LOGERR("[MemoryMonitor] GetAppMemoryConfig: appId is empty");
        return false;
    }

    /* Step 1: Get the app version from the installed packages list */
    std::string version;
    std::vector<WPEFramework::Exchange::IPackageInstaller::Package> packageList;
    Core::hresult status = fetchAppPackageList(packageList);
    if (status == Core::ERROR_NONE)
    {
        for (const auto& package : packageList)
        {
            if (!package.packageId.empty() && package.packageId == appId
                && package.state == Exchange::IPackageInstaller::InstallState::INSTALLED)
            {
                version = std::string(package.version);
                break;
            }
        }
    }

    if (version.empty())
    {
        LOGWARN("[MemoryMonitor] GetAppMemoryConfig: Could not find installed version for appId %s, using defaults", appId.c_str());
        return false;
    }

    /* Step 2: Call Config() to retrieve RuntimeConfig (has systemMemoryLimit from urn:rdk:config:memory.system) */
    if (nullptr != mPackageManagerInstallerObject)
    {
        Exchange::RuntimeConfig runtimeConfig = {};
        status = mPackageManagerInstallerObject->Config(appId, version, runtimeConfig);
        if (status == Core::ERROR_NONE && runtimeConfig.systemMemoryLimit > 0)
        {
            /*
             * RuntimeConfig.systemMemoryLimit is the parsed value of
             * configuration."urn:rdk:config:memory"."system" (e.g. "256M" -> 256).
             * The PackageManager library parses "256M" to int32_t value in MB.
             * Convert to KB: multiply by 1024.
             */
            launchTargetKB = static_cast<uint32_t>(runtimeConfig.systemMemoryLimit) * 1024;
            LOGINFO("[MemoryMonitor] GetAppMemoryConfig: appId %s systemMemoryLimit=%d MB -> launchTargetKB=%u KB",
                    appId.c_str(), runtimeConfig.systemMemoryLimit, launchTargetKB);
            result = true;
        }
        else
        {
            LOGWARN("[MemoryMonitor] GetAppMemoryConfig: Config() returned status %d or systemMemoryLimit=%d for appId %s, using default launch target",
                    status, runtimeConfig.systemMemoryLimit, appId.c_str());
        }
    }
    else
    {
        LOGWARN("[MemoryMonitor] GetAppMemoryConfig: PackageManager not available, using defaults");
    }

    /*
     * Step 3: Retrieve maxSuspendedSystemMemory from the raw package config JSON.
     *
     * The package config JSON (from PackageManager::Config) contains:
     *   "configuration": {
     *     "urn:rdk:config:application-lifecycle": {
     *       "maxSuspendedSystemMemory": "16M",
     *       ...
     *     },
     *     "urn:rdk:config:memory": {
     *       "system": "256M",
     *       "gpu": "128M"
     *     }
     *   }
     *
     * Read the package's config.json from unpackedPath (obtained after Lock)
     * to extract maxSuspendedSystemMemory, which is not available in RuntimeConfig.
     */
    mAdminLock.Lock();
    auto it = mAppInfo.find(appId);
    if (it != mAppInfo.end() && !it->second.packageInfo.unpackedPath.empty())
    {
        std::string configFilePath = it->second.packageInfo.unpackedPath + "/config.json";
        std::ifstream configFile(configFilePath);
        if (configFile.is_open())
        {
            std::string configContent((std::istreambuf_iterator<char>(configFile)),
                                       std::istreambuf_iterator<char>());
            configFile.close();

            JsonObject configJson;
            if (configJson.FromString(configContent))
            {
                /* Navigate: configuration -> urn:rdk:config:application-lifecycle -> maxSuspendedSystemMemory */
                if (configJson.HasLabel("configuration"))
                {
                    JsonObject configuration = configJson["configuration"].Object();
                    if (configuration.HasLabel("urn:rdk:config:application-lifecycle"))
                    {
                        JsonObject lifecycle = configuration["urn:rdk:config:application-lifecycle"].Object();
                        if (lifecycle.HasLabel("maxSuspendedSystemMemory"))
                        {
                            string maxSuspendedStr = lifecycle["maxSuspendedSystemMemory"].String();
                            uint32_t parsed = ParseMemorySizeToKB(maxSuspendedStr);
                            if (parsed > 0)
                            {
                                preloadTargetKB = parsed;
                                LOGINFO("[MemoryMonitor] GetAppMemoryConfig: appId %s maxSuspendedSystemMemory=%s -> preloadTargetKB=%u KB",
                                        appId.c_str(), maxSuspendedStr.c_str(), preloadTargetKB);
                            }
                        }
                    }

                    /* Also try to read urn:rdk:config:memory.system from JSON as a fallback / cross-check */
                    if (!result && configuration.HasLabel("urn:rdk:config:memory"))
                    {
                        JsonObject memory = configuration["urn:rdk:config:memory"].Object();
                        if (memory.HasLabel("system"))
                        {
                            string systemMemStr = memory["system"].String();
                            uint32_t parsed = ParseMemorySizeToKB(systemMemStr);
                            if (parsed > 0)
                            {
                                launchTargetKB = parsed;
                                LOGINFO("[MemoryMonitor] GetAppMemoryConfig: appId %s urn:rdk:config:memory.system=%s -> launchTargetKB=%u KB (from JSON fallback)",
                                        appId.c_str(), systemMemStr.c_str(), launchTargetKB);
                                result = true;
                            }
                        }
                    }
                }
            }
            else
            {
                LOGWARN("[MemoryMonitor] GetAppMemoryConfig: Failed to parse config.json at %s", configFilePath.c_str());
            }
        }
        else
        {
            LOGINFO("[MemoryMonitor] GetAppMemoryConfig: config.json not found at %s, using default preload target", configFilePath.c_str());
        }
    }
    else
    {
        LOGINFO("[MemoryMonitor] GetAppMemoryConfig: unpackedPath not available for appId %s (package may not be locked yet), using default preload target",
                appId.c_str());
    }
    mAdminLock.Unlock();

    LOGINFO("[MemoryMonitor] GetAppMemoryConfig: appId %s final launchTargetKB=%u KB, preloadTargetKB=%u KB",
            appId.c_str(), launchTargetKB, preloadTargetKB);

    return result;
}

} /* namespace Plugin */
} /* namespace WPEFramework */
