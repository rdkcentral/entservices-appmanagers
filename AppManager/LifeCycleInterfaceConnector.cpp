/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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
#include "LifeCycleInterfaceConnector.h"
#include <string>
#include <memory>
#include <iostream>
#include <mutex>
#include <thread>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <plugins/System.h>

#include <interfaces/ILifeCycleManager.h>

#include "UtilsString.h"
#include "AppManagerImplementation.h"

using namespace std;
using namespace Utils;

namespace WPEFramework
{
    namespace Plugin
    {
        class NotificationHandler : public Exchange::ILifeCycleManager::INotification {

            public:
                NotificationHandler(LifeCycleInterfaceConnector& parent) : mParent(parent){}
                ~NotificationHandler(){}

                void OnAppStateChanged(const string& appId, Exchange::ILifeCycleManager::LifeCycleState state, const string& errorReason)
                {
                    mParent.OnAppStateChanged(appId, state, errorReason);
                }

                BEGIN_INTERFACE_MAP(NotificationHandler)
                INTERFACE_ENTRY(Exchange::ILifeCycleManager::INotification)
                END_INTERFACE_MAP

            private:
                LifeCycleInterfaceConnector& mParent;
        };

        LifeCycleInterfaceConnector* LifeCycleInterfaceConnector::_instance = nullptr;

        LifeCycleInterfaceConnector::LifeCycleInterfaceConnector(PluginHost::IShell* service)
		: mLifeCycleManagerRemoteObject(nullptr),
		  mCurrentservice(nullptr)
        {
            LOGINFO("Create LifeCycleInterfaceConnector Instance");
            LifeCycleInterfaceConnector::_instance = this;
            if (service != nullptr)
            {
                mCurrentservice = service;
                mCurrentservice->AddRef();
            }
        }

        LifeCycleInterfaceConnector::~LifeCycleInterfaceConnector()
        {
            if (nullptr != mCurrentservice)
            {
               mCurrentservice->Release();
               mCurrentservice = nullptr;
            }
            LifeCycleInterfaceConnector::_instance = nullptr;
        }

        Core::hresult LifeCycleInterfaceConnector::createLifeCycleManagerRemoteObject()
        {
            Core::hresult status = Core::ERROR_GENERAL;
            Core::Sink<NotificationHandler> notification(*this);

            if (nullptr == mCurrentservice)
            {
                LOGWARN("mCurrentservice is null \n");
            }
            else if (nullptr == (mLifeCycleManagerRemoteObject = mCurrentservice->QueryInterfaceByCallsign<WPEFramework::Exchange::ILifeCycleManager>("org.rdk.LifeCycleManager")))
            {
                LOGWARN("Failed to create LifeCycleManager Remote object\n");
            }
            else
            {
                LOGINFO("Created LifeCycleManager Remote Object \n");
                mLifeCycleManagerRemoteObject->AddRef();
                mLifeCycleManagerRemoteObject->Register(&notification);
                LOGINFO("LifeCycleManager notification registered");
                status = Core::ERROR_NONE;
            }
            return status;
        }

        void LifeCycleInterfaceConnector::releaseLifeCycleManagerRemoteObject()
        {
            ASSERT(nullptr != mLifeCycleManagerRemoteObject );
            if(mLifeCycleManagerRemoteObject )
            {
                mLifeCycleManagerRemoteObject ->Release();
                mLifeCycleManagerRemoteObject = nullptr;
            }
        }

/*
 * @brief LaunchApp invokes this to call LifecycleManager API.
 * @Params  : const string& appId , const string& intent , const string& launchArgs
 * @return  : Core::hresult
 */
        Core::hresult LifeCycleInterfaceConnector::launch(const string& appId, const string& intent, const string& launchArgs)
        {
            Core::hresult status = Core::ERROR_GENERAL;
            AppManagerImplementation::LoadedAppInfo loadedAppInfo;
            AppManagerImplementation*appManagerImplInstance = AppManagerImplementation::getInstance();

            string appPath = "";
            string appConfig = "";
            string runtimeAppId = "";
            string runtimePath = "";
            string runtimeConfig = "";
            string environmentVars = "";
            bool enableDebugger = false;
            string appInstanceId = "";
            string errorReason = "";
            bool success = true;

            if (appId.empty())
            {
                LOGERR("appId is empty");
            }
            else
            {
                mAdminLock.Lock();
                /* Checking if mLifeCycleManagerRemoteObject is not valid then create the object */
                if (nullptr == mLifeCycleManagerRemoteObject)
                {
                    LOGINFO("Create LifeCycleManager Remote store object");
                    if (Core::ERROR_NONE != createLifeCycleManagerRemoteObject())
                    {
                        LOGERR("Failed to create LifeCycleInterfaceConnector");
                    }
                }
                ASSERT (nullptr != mLifeCycleManagerRemoteObject);

                if (nullptr != mLifeCycleManagerRemoteObject)
                {
                    status = mLifeCycleManagerRemoteObject->SpawnApp(appId, appPath, appConfig, runtimeAppId, runtimePath, runtimeConfig, intent, environmentVars,
                                                                  enableDebugger, Exchange::ILifeCycleManager::LifeCycleState::ACTIVE, launchArgs, appInstanceId, errorReason, success);

                    if (status == Core::ERROR_NONE)
                    {
                        LOGINFO("Update App Info");
                        loadedAppInfo.appInstanceId = std::move(appInstanceId);
                        loadedAppInfo.type = AppManagerImplementation::APPLICATION_TYPE_INTERACTIVE;
                        loadedAppInfo.appIntent = intent;
                        loadedAppInfo.currentAppState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
                        loadedAppInfo.currentAppLifecycleState = Exchange::ILifeCycleManager::LOADING;
                        loadedAppInfo.targetAppState = Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;

                        /*Insert/update loaded app info*/
                        if (nullptr != appManagerImplInstance)
                        {
                            appManagerImplInstance->mLoadedAppInfo[appId] = std::move(loadedAppInfo);
                        }
                    }
                    else
                    {
                        LOGERR("App launch failed");
                    }
                }
                mAdminLock.Unlock();
            }
            return status;
        }

        /* PreloadApp invokes it */
        Core::hresult LifeCycleInterfaceConnector::preLoadApp(const string& appId, const string& launchArgs, string& error)
        {
            Core::hresult status = Core::ERROR_GENERAL;
            AppManagerImplementation::LoadedAppInfo loadedAppInfo;
            AppManagerImplementation *appManagerImplInstance = AppManagerImplementation::getInstance();

            string appPath = "";
            string intent = "";
            string appConfig = "";
            string runtimeAppId = "";
            string runtimePath = "";
            string runtimeConfig = "";
            string environmentVars = "";
            bool enableDebugger = false;
            string appInstanceId = "";
            bool success = true;

            if (appId.empty())
            {
                LOGERR("appId is empty");
            }
            else
            {
                mAdminLock.Lock();
                /* Checking if mLifeCycleManagerRemoteObject is not valid then create the object */
                if (nullptr == mLifeCycleManagerRemoteObject)
                {
                    LOGINFO("Create LifeCycleManager Remote store object");
                    if (Core::ERROR_NONE != createLifeCycleManagerRemoteObject())
                    {
                        LOGERR("Failed to create LifeCycleInterfaceConnector");
                    }
                }
                ASSERT (nullptr != mLifeCycleManagerRemoteObject);
                if (nullptr != mLifeCycleManagerRemoteObject)
                {
                    status = mLifeCycleManagerRemoteObject->SpawnApp(appId, appPath, appConfig, runtimeAppId, runtimePath, runtimeConfig, intent, environmentVars,
                          enableDebugger, Exchange::ILifeCycleManager::LifeCycleState::RUNNING, launchArgs, appInstanceId, error, success);
                    if (status == Core::ERROR_NONE)
                    {
                        LOGINFO("Update App Info");
                        loadedAppInfo.appInstanceId = std::move(appInstanceId);
                        loadedAppInfo.type = AppManagerImplementation::APPLICATION_TYPE_INTERACTIVE;
                        loadedAppInfo.currentAppState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
                        loadedAppInfo.currentAppLifecycleState = Exchange::ILifeCycleManager::LOADING;
                        loadedAppInfo.targetAppState = Exchange::IAppManager::AppLifecycleState::APP_STATE_RUNNING;

                        /*Insert/update loaded app info*/
                        if (nullptr != appManagerImplInstance)
                        {
                            appManagerImplInstance->mLoadedAppInfo[appId] = std::move(loadedAppInfo);
                        }
                    }
                    else
                    {
                        LOGERR("PreLoad failed");
                    }
                }
                mAdminLock.Unlock();
            }
            return status;
        }

        /* Close App invokes it */
        Core::hresult LifeCycleInterfaceConnector::closeApp(const string& appId)
        {
            uint32_t status = Core::ERROR_GENERAL;
            std::string appInstanceId = "";
            std::string appIntent = "";
            Exchange::IAppManager::AppLifecycleState targetLifecycleState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
            AppManagerImplementation* appManagerImplInstance = AppManagerImplementation::getInstance();

            /* Retrieve the appId from the parameters object */
            LOGINFO("AppId retrieved: %s", appId.c_str());

            mAdminLock.Lock();

            if (nullptr != appManagerImplInstance)
            {
                for ( std::map<std::string, AppManagerImplementation::LoadedAppInfo>::iterator appIterator = appManagerImplInstance->mLoadedAppInfo.begin(); appIterator != appManagerImplInstance->mLoadedAppInfo.end(); appIterator++)
                {
                    if (appIterator->first.compare(appId) == 0)
                    {
                        appInstanceId = appIterator->second.appInstanceId;
                        appIntent = appIterator->second.appIntent;
                        targetLifecycleState = appIterator->second.targetAppState;

                        /* Check targetLifecycleState is in ACTIVE state */
                        if(targetLifecycleState == Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE)
                        {
                            if (nullptr != mLifeCycleManagerRemoteObject)
                            {
                                /* Call setTargetAppState from Adapter file once ready */
                                 status = mLifeCycleManagerRemoteObject->SetTargetAppState(appInstanceId, Exchange::ILifeCycleManager::LifeCycleState::SUSPENDED, appIntent);
                                if(status == Core::ERROR_NONE)
                                {
                                    appIterator->second.targetAppState = Exchange::IAppManager::AppLifecycleState::APP_STATE_SUSPENDED;
                                }
                                else
                                {
                                    LOGERR("Fail to closeApp AppId: %s status:%u", appId.c_str(), status);
                                }
                            }
                        }
                        break;
                    }
                    else
                    {
                        LOGERR("appId not found");
                    }
                }
            }
            else
            {
                LOGERR("appManagerImplInstance is null");
            }
            mAdminLock.Unlock();
            return status;
        }

        /* Terminate App invokes it */
        Core::hresult LifeCycleInterfaceConnector::terminateApp(const string& appId)
        {
            uint32_t status = Core::ERROR_GENERAL;
            std::string appInstanceId = "";
            Exchange::IAppManager::AppLifecycleState targetLifecycleState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
            AppManagerImplementation* appManagerImplInstance = AppManagerImplementation::getInstance();
            bool success = false;
            std::string errorReason = "";
            bool foundAppId = false;

            /* Retrieve the appId from the parameters object */
            LOGINFO("AppId retrieved: %s", appId.c_str());

            mAdminLock.Lock();
            if (nullptr != appManagerImplInstance)
            {
                for ( std::map<std::string, AppManagerImplementation::LoadedAppInfo>::iterator appIterator = appManagerImplInstance->mLoadedAppInfo.begin(); appIterator != appManagerImplInstance->mLoadedAppInfo.end(); appIterator++)
                {
                    if (appIterator->first.compare(appId) == 0)
                    {
                        foundAppId = true;
                        appInstanceId = appIterator->second.appInstanceId;
                        targetLifecycleState = appIterator->second.currentAppState;

                        /* Check targetLifecycleState is in RUNNING state */
                        if(targetLifecycleState == Exchange::IAppManager::AppLifecycleState::APP_STATE_RUNNING)
                        {
                            if (nullptr != mLifeCycleManagerRemoteObject)
                            {
                                 status = mLifeCycleManagerRemoteObject->UnloadApp(appInstanceId, errorReason, success);
                                if (status != Core::ERROR_NONE)
                                {
                                    if (!errorReason.empty())
                                    {
                                        LOGERR("UnloadApp failed with error reason: %s", errorReason.c_str());
                                    }
                                    else
                                    {
                                        LOGERR("UnloadApp failed with an unknown error.");
                                    }
                                }
                                else
                                {
                                    /* Remove the appId from the database */
                                    appManagerImplInstance->mLoadedAppInfo.erase(appIterator);
                                    LOGINFO("AppId %s and its corresponding data removed from database.", appId.c_str());
                                }
                            }
                        }
                        break;
                    }
                }
                if (!foundAppId)
                {
                    LOGERR("AppId %s not found in database", appId.c_str());
                }
            }
            else
            {
                LOGERR("appManagerImplInstance is null");
            }
            mAdminLock.Unlock();
            return status;
        }

        /* Kill App invokes it */
        Core::hresult LifeCycleInterfaceConnector::killApp(const string& appId)
        {
            LOGINFO("killApp entered");
            Core::hresult result = Core::ERROR_GENERAL;

            if (appId.empty())
            {
                LOGERR("appId is empty");
                return result;
            }

            if (nullptr == mLifeCycleManagerRemoteObject)
            {
                LOGINFO("Create LifeCycleManager Remote store object");
                if (Core::ERROR_NONE != createLifeCycleManagerRemoteObject())
                {
                    LOGERR("Failed to create LifeCycleInterfaceConnector");
                }
            }
            ASSERT (nullptr != mLifeCycleManagerRemoteObject);
            if (nullptr != mLifeCycleManagerRemoteObject)
            {
                mAdminLock.Lock();
                string appInstanceId = GetAppInstanceId(appId);
                if (appInstanceId.empty())
                {
                    LOGERR("appInstanceId not found for appId '%s'", appId.c_str());
                }
                else
                {
                    string errorReason{};
                    bool success{};

                    result = mLifeCycleManagerRemoteObject->KillApp(appInstanceId, errorReason, success);

                    if (!(result == Core::ERROR_NONE && success))
                    {
                        LOGERR("killApp failed, result: %d, success: %d, errorReason: %s", result, success, errorReason.c_str());
                    }
                    else // killApp succedded
                    {
                        RemoveApp(appId);
                    }
                }
                mAdminLock.Unlock();
            }

            return result;
        }

        /* Send Intent invokes it */
        Core::hresult LifeCycleInterfaceConnector::sendIntent(const string& appId, const string& intent)
        {
            LOGINFO("sendIntent Entered");
            Core::hresult result = Core::ERROR_GENERAL;

            if (appId.empty())
            {
                LOGERR("appId is empty");
                return result;
            }

            if (nullptr == mLifeCycleManagerRemoteObject)
            {
                LOGINFO("Create LifeCycleManager Remote store object");
                if (Core::ERROR_NONE != createLifeCycleManagerRemoteObject())
                {
                    LOGERR("Failed to create LifeCycleInterfaceConnector");
                }
            }

            if (nullptr != mLifeCycleManagerRemoteObject)
            {
                mAdminLock.Lock();
                string appInstanceId = GetAppInstanceId(appId);
                if (appInstanceId.empty())
                {
                    LOGERR("appInstanceId not found for appId '%s'", appId.c_str());
                }
                else
                {
                    string errorReason{};
                    bool success{};

                    result = mLifeCycleManagerRemoteObject->SendIntentToActiveApp(appInstanceId, intent, errorReason, success);

                    if (!(result == Core::ERROR_NONE && success))
                    {
                        LOGERR("sentIntent failed, result: %d, success: %d, errorReason: %s", result, success, errorReason.c_str());
                    }
                }
                mAdminLock.Unlock();
            }

            return result;
        }

        /* GetLoadedApps invokes it */
        Core::hresult LifeCycleInterfaceConnector::getLoadedApps(string& apps)
        {
            LOGINFO("getLoadedApps Entered");
            Core::hresult result = Core::ERROR_GENERAL;
            AppManagerImplementation *appManagerImplInstance = AppManagerImplementation::getInstance();

            mAdminLock.Lock();
            /* Checking if mLifeCycleManagerRemoteObject is not valid then create the object */
            if (nullptr == mLifeCycleManagerRemoteObject)
            {
                LOGINFO("Create LifeCycleManager Remote store object");
                if (Core::ERROR_NONE != createLifeCycleManagerRemoteObject())
                {
                    LOGERR("Failed to create LifeCycleInterfaceConnector");
                }
            }

            ASSERT (nullptr != mLifeCycleManagerRemoteObject);
            if (nullptr != mLifeCycleManagerRemoteObject)
            {
                JsonArray loadedAppsArray;
                if (nullptr != appManagerImplInstance)
                {
                    for (const auto& appEntry :appManagerImplInstance->mLoadedAppInfo)
                    {
                        JsonObject loadedAppObject;

                        loadedAppObject["appId"] = appEntry.first;
                        loadedAppObject["type"] = static_cast<int>(appEntry.second.type);
                        loadedAppObject["lifecycleState"] = static_cast<int>(appEntry.second.currentAppState);
                        loadedAppObject["targetLifecycleState"] = static_cast<int>(appEntry.second.targetAppState);
                        loadedAppObject["activeSessionId"] = appEntry.second.activeSessionId;
                        loadedAppObject["appInstanceId"] = appEntry.second.appInstanceId;

                        loadedAppsArray.Add(loadedAppObject);
                    }
                    loadedAppsArray.ToString(apps);
                    LOGINFO("getLoadedApps: %s", apps.c_str());
                    result = Core::ERROR_NONE;
                }
                else
                {
                    LOGERR("appManagerImplementation instance is nullptr");
                }
            }
            mAdminLock.Unlock();
            return result;
        }

        void LifeCycleInterfaceConnector::OnAppStateChanged(const string& appId, Exchange::ILifeCycleManager::LifeCycleState state, const string& errorReason)
        {
            string appInstanceId = "";
            Exchange::IAppManager::AppLifecycleState oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
            Exchange::IAppManager::AppLifecycleState newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
            AppManagerImplementation*appManagerImplInstance = AppManagerImplementation::getInstance();

            LOGINFO("onAppLifecycleStateChanged event triggered ***\n");

            switch(state)
            {
                case Exchange::ILifeCycleManager::LifeCycleState::INITIALIZING:
                case Exchange::ILifeCycleManager::LifeCycleState::TERMINATEREQUESTED:
                case Exchange::ILifeCycleManager::LifeCycleState::TERMINATING:
                case Exchange::ILifeCycleManager::LifeCycleState::RUNNING:
                    newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_RUNNING;
                    break;

                case Exchange::ILifeCycleManager::LifeCycleState::ACTIVATEREQUESTED:
                case Exchange::ILifeCycleManager::LifeCycleState::ACTIVE:
                    newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
                    break;

                case Exchange::ILifeCycleManager::LifeCycleState::SUSPENDREQUESTED:
                case Exchange::ILifeCycleManager::LifeCycleState::SUSPENDED:
                    newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_SUSPENDED;

                default:
                    LOGWARN("Unknown state %u", state);
                    break;
            }

            mAdminLock.Lock();
            if (nullptr != appManagerImplInstance)
            {
                for ( std::map<std::string, AppManagerImplementation::LoadedAppInfo>::iterator it = appManagerImplInstance->mLoadedAppInfo.begin(); it != appManagerImplInstance->mLoadedAppInfo.end(); it++)
                {
                    if (it->first.compare(appId) == 0)
                    {
                        appInstanceId = it->second.appInstanceId;
                        oldState = it->second.currentAppState;
                        it->second.currentAppLifecycleState = state;
                        if (oldState != newState)
                        {
                            it->second.currentAppState = newState;
                        }
                        break;
                    }
                }
            }
            /*TODO: AppManager event notification to be handled in upcoming phase*/
            mAdminLock.Unlock();
        }

        /* Returns appInstanceId for appId. Does not synchronize access to LoadedAppInfo.
         * Caller should take care about the synchronization.
         */
        string LifeCycleInterfaceConnector::GetAppInstanceId(const string& appId) const
        {
            AppManagerImplementation* appManagerImpl = AppManagerImplementation::getInstance();
            if (!appManagerImpl)
                return {};

            auto it = appManagerImpl->mLoadedAppInfo.find(appId);
            if (it == appManagerImpl->mLoadedAppInfo.end())
                return {};
            else
                return it->second.appInstanceId;
        }

        void LifeCycleInterfaceConnector::RemoveApp(const string& appId)
        {
            AppManagerImplementation* appManagerImpl = AppManagerImplementation::getInstance();
            if (!appManagerImpl)
                return;

            auto it = appManagerImpl->mLoadedAppInfo.find(appId);
            if (it != appManagerImpl->mLoadedAppInfo.end())
                appManagerImpl->mLoadedAppInfo.erase(it);
            else
                LOGERR("AppInfo for appId '%s' not found", appId.c_str());
        }      

     } /* namespace Plugin */
} /* namespace WPEFramework */
