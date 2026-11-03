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

#include "Module.h"
#include <interfaces/ILifecycleManager.h>
#include <interfaces/ILifecycleManagerState.h>
#include <interfaces/IConfiguration.h>
#include "IEventHandler.h"
#include "UtilsLogging.h"
#include "tracing/Logging.h"
#include "ApplicationContext.h"
#include <deque>
#include <map>
#include <mutex>

namespace WPEFramework
{
    namespace Plugin
    {
        class LifecycleManagerImplementation : public Exchange::ILifecycleManager
					     , public Exchange::ILifecycleManagerState
					     , public Exchange::IConfiguration
					     , public IEventHandler
	{
            public:
                enum EventNames
                {
                    LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED,
                    LIFECYCLE_MANAGER_EVENT_RUNTIME,
                    LIFECYCLE_MANAGER_EVENT_WINDOW,
                    LIFECYCLE_MANAGER_EVENT_ONFAILURE
                };

                struct PendingRespawnRequest
	        {
                    PendingRespawnRequest()
	            : mLaunchParams()
	            {
	            }

                    ApplicationLaunchParams mLaunchParams;
	        };

                class EXTERNAL Job : public Core::IDispatch
	        {
                    protected:
                                 explicit Job(LifecycleManagerImplementation *lifeCycleManagerImplementation)
                            : mLifecycleManagerImplementation(lifeCycleManagerImplementation)
	                {
                            if (mLifecycleManagerImplementation != nullptr)
	            	    {
                                mLifecycleManagerImplementation->AddRef();
                            }
                        }

                    public:
                         Job() = delete;
                         Job(const Job&) = delete;
                         Job& operator=(const Job&) = delete;
                         ~Job()
	                 {
                             if (mLifecycleManagerImplementation != nullptr)
	            	     {
                                 mLifecycleManagerImplementation->Release();
                             }
                         }

                    public:
                         static Core::ProxyType<Core::IDispatch> Create(LifecycleManagerImplementation *lifeCycleManagerImplementation)
	                 {
#ifndef  USE_THUNDER_R4
                             return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(lifeCycleManagerImplementation)));
#else
                             return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(lifeCycleManagerImplementation)));
#endif
                         }

                         virtual void Dispatch()
	                 {
                             mLifecycleManagerImplementation->drainEvents();
                         }
                    private:
                        LifecycleManagerImplementation *mLifecycleManagerImplementation;
                };

                class EXTERNAL AppStateListenerJob : public Core::IDispatch
                {
                    protected:
                        AppStateListenerJob(Exchange::ILifecycleManager::INotification* notification,
                            const string& appId,
                            const Exchange::ILifecycleManager::LifecycleState newLifecycleState,
                            const string& errorReason)
                            : mNotification(notification)
                            , mAppId(appId)
                            , mNewLifecycleState(newLifecycleState)
                            , mErrorReason(errorReason)
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->AddRef();
                            }
                        }

                    public:
                        AppStateListenerJob() = delete;
                        AppStateListenerJob(const AppStateListenerJob&) = delete;
                        AppStateListenerJob& operator=(const AppStateListenerJob&) = delete;
                        ~AppStateListenerJob() override
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->Release();
                            }
                        }

                        static Core::ProxyType<Core::IDispatch> Create(Exchange::ILifecycleManager::INotification* notification,
                            const string& appId,
                            const Exchange::ILifecycleManager::LifecycleState newLifecycleState,
                            const string& errorReason)
                        {
#ifndef USE_THUNDER_R4
                            return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<AppStateListenerJob>::Create(notification, appId, newLifecycleState, errorReason)));
#else
                            return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<AppStateListenerJob>::Create(notification, appId, newLifecycleState, errorReason)));
#endif
                        }

                        void Dispatch() override
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->OnAppStateChanged(mAppId, mNewLifecycleState, mErrorReason);
                            }
                        }

                    private:
                        Exchange::ILifecycleManager::INotification* mNotification;
                        const string mAppId;
                        const Exchange::ILifecycleManager::LifecycleState mNewLifecycleState;
                        const string mErrorReason;
                };

                class EXTERNAL LifecycleStateListenerJob : public Core::IDispatch
                {
                    protected:
                        LifecycleStateListenerJob(Exchange::ILifecycleManagerState::INotification* notification,
                            const string& appId,
                            const string& appInstanceId,
                            const Exchange::ILifecycleManager::LifecycleState oldLifecycleState,
                            const Exchange::ILifecycleManager::LifecycleState newLifecycleState,
                            const string& navigationIntent)
                            : mNotification(notification)
                            , mAppId(appId)
                            , mAppInstanceId(appInstanceId)
                            , mOldLifecycleState(oldLifecycleState)
                            , mNewLifecycleState(newLifecycleState)
                            , mNavigationIntent(navigationIntent)
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->AddRef();
                            }
                        }

                class EXTERNAL AppStateListenerJob : public Core::IDispatch
                {
                    protected:
                        AppStateListenerJob(Exchange::ILifecycleManager::INotification* notification,
                            const string& appId,
                            const Exchange::ILifecycleManager::LifecycleState newLifecycleState,
                            const string& errorReason)
                            : mNotification(notification)
                            , mAppId(appId)
                            , mNewLifecycleState(newLifecycleState)
                            , mErrorReason(errorReason)
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->AddRef();
                            }
                        }

                    public:
                        AppStateListenerJob() = delete;
                        AppStateListenerJob(const AppStateListenerJob&) = delete;
                        AppStateListenerJob& operator=(const AppStateListenerJob&) = delete;
                        ~AppStateListenerJob() override
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->Release();
                            }
                        }

                        static Core::ProxyType<Core::IDispatch> Create(Exchange::ILifecycleManager::INotification* notification,
                            const string& appId,
                            const Exchange::ILifecycleManager::LifecycleState newLifecycleState,
                            const string& errorReason)
                        {
#ifndef USE_THUNDER_R4
                            return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<AppStateListenerJob>::Create(notification, appId, newLifecycleState, errorReason)));
#else
                            return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<AppStateListenerJob>::Create(notification, appId, newLifecycleState, errorReason)));
#endif
                        }

                        void Dispatch() override
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->OnAppStateChanged(mAppId, mNewLifecycleState, mErrorReason);
                            }
                        }

                    private:
                        Exchange::ILifecycleManager::INotification* mNotification;
                        const string mAppId;
                        const Exchange::ILifecycleManager::LifecycleState mNewLifecycleState;
                        const string mErrorReason;
                };

                class EXTERNAL LifecycleStateListenerJob : public Core::IDispatch
                {
                    protected:
                        LifecycleStateListenerJob(Exchange::ILifecycleManagerState::INotification* notification,
                            const string& appId,
                            const string& appInstanceId,
                            const Exchange::ILifecycleManager::LifecycleState oldLifecycleState,
                            const Exchange::ILifecycleManager::LifecycleState newLifecycleState,
                            const string& navigationIntent)
                            : mNotification(notification)
                            , mAppId(appId)
                            , mAppInstanceId(appInstanceId)
                            , mOldLifecycleState(oldLifecycleState)
                            , mNewLifecycleState(newLifecycleState)
                            , mNavigationIntent(navigationIntent)
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->AddRef();
                            }
                        }

                    public:
                        LifecycleStateListenerJob() = delete;
                        LifecycleStateListenerJob(const LifecycleStateListenerJob&) = delete;
                        LifecycleStateListenerJob& operator=(const LifecycleStateListenerJob&) = delete;
                        ~LifecycleStateListenerJob() override
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->Release();
                            }
                        }

                        static Core::ProxyType<Core::IDispatch> Create(Exchange::ILifecycleManagerState::INotification* notification,
                            const string& appId,
                            const string& appInstanceId,
                            const Exchange::ILifecycleManager::LifecycleState oldLifecycleState,
                            const Exchange::ILifecycleManager::LifecycleState newLifecycleState,
                            const string& navigationIntent)
                        {
#ifndef USE_THUNDER_R4
                            return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<LifecycleStateListenerJob>::Create(notification, appId, appInstanceId, oldLifecycleState, newLifecycleState, navigationIntent)));
#else
                            return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<LifecycleStateListenerJob>::Create(notification, appId, appInstanceId, oldLifecycleState, newLifecycleState, navigationIntent)));
#endif
                        }

                        void Dispatch() override
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->OnAppLifecycleStateChanged(mAppId, mAppInstanceId, mOldLifecycleState, mNewLifecycleState, mNavigationIntent);
                            }
                        }

                    private:
                        Exchange::ILifecycleManagerState::INotification* mNotification;
                        const string mAppId;
                        const string mAppInstanceId;
                        const Exchange::ILifecycleManager::LifecycleState mOldLifecycleState;
                        const Exchange::ILifecycleManager::LifecycleState mNewLifecycleState;
                        const string mNavigationIntent;
                };
	    private: /* members */
                mutable Core::CriticalSection mAdminLock;
	        std::mutex mEventQueueLock;
                std::deque<std::pair<EventNames, JsonValue>> mEventQueue;
                bool mEventDispatchScheduled { false };
	        std::list<Exchange::ILifecycleManager::INotification*> mLifecycleManagerNotification;
	        std::list<Exchange::ILifecycleManagerState::INotification*> mLifecycleManagerStateNotification;
                std::list<std::shared_ptr<ApplicationContext>> mLoadedApplications;
                std::map<string, PendingRespawnRequest> mPendingRespawns;
                PluginHost::IShell* mService;
	    private: /* internal methods */
                bool initialize(PluginHost::IShell* service);
                void terminate();
                void dispatchEvent(EventNames, const JsonValue &params);
                void drainEvents();
                void Dispatch(EventNames event, const JsonValue params);
                void submitStateChangeNotifications(const JsonObject& data);
                void handleRuntimeManagerEvent(const JsonObject &data);
                void notifyOnFailure(const string& appInstanceId, const string& errorCode);
                void handleStateChangeEvent(const JsonObject &data);
                bool tryGetPendingRespawn(const string& appInstanceId, PendingRespawnRequest& pendingRespawn);
                void handlePendingRespawn(const PendingRespawnRequest& pendingRespawn);
                void handleWindowManagerEvent(const JsonObject &data);
                std::shared_ptr<ApplicationContext> getContext(const string& appInstanceId, const string& appId) const;
                void addStateTransitionRequest(ApplicationContext* context, std::string event);

                friend class Job;

#ifdef UNIT_TEST                
                friend class LifecycleManagerImplementationTest;
#endif
        };
    } /* namespace Plugin */
} /* namespace WPEFramework */

