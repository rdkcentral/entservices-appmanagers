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
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <thread>

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

                struct AppStateNotification
                {
                    string appId;
                    Exchange::ILifecycleManager::LifecycleState newLifecycleState;
                    string errorReason;
                };

                class EXTERNAL AppStateNotificationQueueJob : public Core::IDispatch
                {
                    protected:
                        AppStateNotificationQueueJob(LifecycleManagerImplementation* manager,
                            Exchange::ILifecycleManager::INotification* notification)
                            : mLifecycleManager(manager)
                            , mNotification(notification)
                            , mGeneration((manager != nullptr) && (notification != nullptr) ? manager->mAppStateNotificationGeneration[notification] : 0U)
                        {
                            if (mLifecycleManager != nullptr)
                            {
                                mLifecycleManager->AddRef();
                            }
                            if (mNotification != nullptr)
                            {
                                mNotification->AddRef();
                            }
                        }

                    public:
                        AppStateNotificationQueueJob() = delete;
                        AppStateNotificationQueueJob(const AppStateNotificationQueueJob&) = delete;
                        AppStateNotificationQueueJob& operator=(const AppStateNotificationQueueJob&) = delete;
                        ~AppStateNotificationQueueJob() override
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->Release();
                            }
                            if (mLifecycleManager != nullptr)
                            {
                                mLifecycleManager->Release();
                            }
                        }

                        static Core::ProxyType<Core::IDispatch> Create(LifecycleManagerImplementation* manager,
                            Exchange::ILifecycleManager::INotification* notification)
                        {
#ifndef USE_THUNDER_R4
                            return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<AppStateNotificationQueueJob>::Create(manager, notification)));
#else
                            return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<AppStateNotificationQueueJob>::Create(manager, notification)));
#endif
                        }

                        void Dispatch() override
                        {
                            if ((mLifecycleManager == nullptr) || (mNotification == nullptr))
                            {
                                return;
                            }

                            {
                                Core::SafeSyncType<Core::CriticalSection> lock(mLifecycleManager->mAdminLock);
                                if (mLifecycleManager->mAppStateNotificationGeneration[mNotification] != mGeneration)
                                {
                                    return;
                                }
                            }

                            {
                                std::lock_guard<std::mutex> lock(mLifecycleManager->mAppStateNotificationLock);
                                ++mLifecycleManager->mAppStateNotificationInFlight[mNotification];
                                mLifecycleManager->mAppStateNotificationDispatchingThread[mNotification] = std::this_thread::get_id();
                            }

                            std::deque<AppStateNotification> notifications;
                            {
                                Core::SafeSyncType<Core::CriticalSection> lock(mLifecycleManager->mAdminLock);
                                auto generationIt = mLifecycleManager->mAppStateNotificationGeneration.find(mNotification);
                                if ((generationIt == mLifecycleManager->mAppStateNotificationGeneration.end()) ||
                                    (generationIt->second != mGeneration))
                                {
                                    // This job is stale; it will fall through to the normal in-flight decrement.
                                }
                                else
                                {
                                    auto it = mLifecycleManager->mAppStateNotifications.find(mNotification);
                                    if (it == mLifecycleManager->mAppStateNotifications.end())
                                    {
                                        mLifecycleManager->mAppStateNotificationDrainScheduled.erase(mNotification);
                                    {
                                        std::lock_guard<std::mutex> lock(mLifecycleManager->mAppStateNotificationLock);
                                        auto inFlight = mLifecycleManager->mAppStateNotificationInFlight.find(mNotification);
                                        if (inFlight != mLifecycleManager->mAppStateNotificationInFlight.end())
                                        {
                                            if (0U < inFlight->second)
                                            {
                                                --inFlight->second;
                                            }
                                            if (0U == inFlight->second)
                                            {
                                                mLifecycleManager->mAppStateNotificationInFlight.erase(mNotification);
                                                mLifecycleManager->mAppStateNotificationDispatchingThread.erase(mNotification);
                                                mLifecycleManager->mAppStateNotificationCV.notify_all();
                                            }
                                        }
                                    }
                                    return;
                                }

                                        notifications = std::move(it->second);
                                        it->second.clear();
                                        mLifecycleManager->mAppStateNotifications.erase(it);
                                    }
                                }
                            }

                            while (!notifications.empty())
                            {
                                {
                                    Core::SafeSyncType<Core::CriticalSection> generationLock(mLifecycleManager->mAdminLock);
                                    if (mLifecycleManager->mAppStateNotificationGeneration[mNotification] != mGeneration)
                                    {
                                        break;
                                    }
                                }

                                const auto notification = notifications.front();
                                notifications.pop_front();
                                mNotification->OnAppStateChanged(notification.appId, notification.newLifecycleState, notification.errorReason);
                            }

                            {
                                bool reschedule = false;
                                Core::SafeSyncType<Core::CriticalSection> lock(mLifecycleManager->mAdminLock);
                                auto generationIt = mLifecycleManager->mAppStateNotificationGeneration.find(mNotification);
                                if ((generationIt != mLifecycleManager->mAppStateNotificationGeneration.end()) &&
                                    (generationIt->second == mGeneration))
                                {
                                    auto queueIt = mLifecycleManager->mAppStateNotifications.find(mNotification);
                                    if ((queueIt != mLifecycleManager->mAppStateNotifications.end()) && (!queueIt->second.empty()))
                                    {
                                        reschedule = true;
                                        mLifecycleManager->mAppStateNotificationDrainScheduled[mNotification] = true;
                                    }
                                    else
                                    {
                                        mLifecycleManager->mAppStateNotificationDrainScheduled[mNotification] = false;
                                    }
                                    if (reschedule)
                                    {
                                        Core::IWorkerPool::Instance().Submit(AppStateNotificationQueueJob::Create(mLifecycleManager, mNotification));
                                    }
                                }
                            }

                            {
                                std::lock_guard<std::mutex> lock(mLifecycleManager->mAppStateNotificationLock);
                                auto inFlight = mLifecycleManager->mAppStateNotificationInFlight.find(mNotification);
                                if (inFlight != mLifecycleManager->mAppStateNotificationInFlight.end())
                                {
                                    if (0U < inFlight->second)
                                    {
                                        --inFlight->second;
                                    }
                                    if (0U == inFlight->second)
                                    {
                                        mLifecycleManager->mAppStateNotificationInFlight.erase(mNotification);
                                        mLifecycleManager->mAppStateNotificationDispatchingThread.erase(mNotification);
                                        mLifecycleManager->mAppStateNotificationCV.notify_all();
                                    }
                                }
                            }
                        }

                    private:
                        LifecycleManagerImplementation* mLifecycleManager;
                        Exchange::ILifecycleManager::INotification* mNotification;
                        const uint32_t mGeneration;
                };

                struct LifecycleStateNotification
                {
                    string appId;
                    string appInstanceId;
                    Exchange::ILifecycleManager::LifecycleState oldLifecycleState;
                    Exchange::ILifecycleManager::LifecycleState newLifecycleState;
                    string navigationIntent;
                };

                class EXTERNAL LifecycleStateNotificationQueueJob : public Core::IDispatch
                {
                    protected:
                        LifecycleStateNotificationQueueJob(LifecycleManagerImplementation* manager,
                            Exchange::ILifecycleManagerState::INotification* notification)
                            : mLifecycleManager(manager)
                            , mNotification(notification)
                            , mGeneration((manager != nullptr) && (notification != nullptr) ? manager->mLifecycleStateNotificationGeneration[notification] : 0U)
                        {
                            if (mLifecycleManager != nullptr)
                            {
                                mLifecycleManager->AddRef();
                            }
                            if (mNotification != nullptr)
                            {
                                mNotification->AddRef();
                            }
                        }

                    public:
                        LifecycleStateNotificationQueueJob() = delete;
                        LifecycleStateNotificationQueueJob(const LifecycleStateNotificationQueueJob&) = delete;
                        LifecycleStateNotificationQueueJob& operator=(const LifecycleStateNotificationQueueJob&) = delete;
                        ~LifecycleStateNotificationQueueJob() override
                        {
                            if (mNotification != nullptr)
                            {
                                mNotification->Release();
                            }
                            if (mLifecycleManager != nullptr)
                            {
                                mLifecycleManager->Release();
                            }
                        }

                        static Core::ProxyType<Core::IDispatch> Create(LifecycleManagerImplementation* manager,
                            Exchange::ILifecycleManagerState::INotification* notification)
                        {
#ifndef USE_THUNDER_R4
                            return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<LifecycleStateNotificationQueueJob>::Create(manager, notification)));
#else
                            return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<LifecycleStateNotificationQueueJob>::Create(manager, notification)));
#endif
                        }

                        void Dispatch() override
                        {
                            if ((mLifecycleManager == nullptr) || (mNotification == nullptr))
                            {
                                return;
                            }

                            {
                                Core::SafeSyncType<Core::CriticalSection> lock(mLifecycleManager->mAdminLock);
                                if (mLifecycleManager->mLifecycleStateNotificationGeneration[mNotification] != mGeneration)
                                {
                                    return;
                                }
                            }

                            {
                                std::lock_guard<std::mutex> lock(mLifecycleManager->mLifecycleStateNotificationLock);
                                ++mLifecycleManager->mLifecycleStateNotificationInFlight[mNotification];
                                mLifecycleManager->mLifecycleStateNotificationDispatchingThread[mNotification] = std::this_thread::get_id();
                            }

                            std::deque<LifecycleStateNotification> notifications;
                            {
                                Core::SafeSyncType<Core::CriticalSection> lock(mLifecycleManager->mAdminLock);
                                auto generationIt = mLifecycleManager->mLifecycleStateNotificationGeneration.find(mNotification);
                                if ((generationIt == mLifecycleManager->mLifecycleStateNotificationGeneration.end()) ||
                                    (generationIt->second != mGeneration))
                                {
                                    // This job is stale; it will fall through to the normal in-flight decrement.
                                }
                                else
                                {
                                    auto it = mLifecycleManager->mLifecycleStateNotifications.find(mNotification);
                                    if (it == mLifecycleManager->mLifecycleStateNotifications.end())
                                    {
                                        mLifecycleManager->mLifecycleStateNotificationDrainScheduled.erase(mNotification);
                                        {
                                            std::lock_guard<std::mutex> lock(mLifecycleManager->mLifecycleStateNotificationLock);
                                            auto inFlight = mLifecycleManager->mLifecycleStateNotificationInFlight.find(mNotification);
                                            if (inFlight != mLifecycleManager->mLifecycleStateNotificationInFlight.end())
                                            {
                                                if (0U < inFlight->second)
                                                {
                                                    --inFlight->second;
                                                }
                                                if (0U == inFlight->second)
                                                {
                                                    mLifecycleManager->mLifecycleStateNotificationInFlight.erase(mNotification);
                                                    mLifecycleManager->mLifecycleStateNotificationDispatchingThread.erase(mNotification);
                                                    mLifecycleManager->mLifecycleStateNotificationCV.notify_all();
                                                }
                                            }
                                        }
                                        return;
                                    }

                                    notifications = std::move(it->second);
                                    it->second.clear();
                                    mLifecycleManager->mLifecycleStateNotifications.erase(it);
                                }
                            }

                            while (!notifications.empty())
                            {
                                {
                                    Core::SafeSyncType<Core::CriticalSection> generationLock(mLifecycleManager->mAdminLock);
                                    if (mLifecycleManager->mLifecycleStateNotificationGeneration[mNotification] != mGeneration)
                                    {
                                        break;
                                    }
                                }

                                const auto notification = notifications.front();
                                notifications.pop_front();
                                mNotification->OnAppLifecycleStateChanged(notification.appId,
                                    notification.appInstanceId,
                                    notification.oldLifecycleState,
                                    notification.newLifecycleState,
                                    notification.navigationIntent);
                            }

                            {
                                bool reschedule = false;
                                Core::SafeSyncType<Core::CriticalSection> lock(mLifecycleManager->mAdminLock);
                                auto generationIt = mLifecycleManager->mLifecycleStateNotificationGeneration.find(mNotification);
                                if ((generationIt != mLifecycleManager->mLifecycleStateNotificationGeneration.end()) &&
                                    (generationIt->second == mGeneration))
                                {
                                    auto queueIt = mLifecycleManager->mLifecycleStateNotifications.find(mNotification);
                                    if ((queueIt != mLifecycleManager->mLifecycleStateNotifications.end()) && (!queueIt->second.empty()))
                                    {
                                        reschedule = true;
                                        mLifecycleManager->mLifecycleStateNotificationDrainScheduled[mNotification] = true;
                                    }
                                    else
                                    {
                                        mLifecycleManager->mLifecycleStateNotificationDrainScheduled[mNotification] = false;
                                    }
                                    if (reschedule)
                                    {
                                        Core::IWorkerPool::Instance().Submit(LifecycleStateNotificationQueueJob::Create(mLifecycleManager, mNotification));
                                    }
                                }
                            }

                            {
                                std::lock_guard<std::mutex> lock(mLifecycleManager->mLifecycleStateNotificationLock);
                                auto inFlight = mLifecycleManager->mLifecycleStateNotificationInFlight.find(mNotification);
                                if (inFlight != mLifecycleManager->mLifecycleStateNotificationInFlight.end())
                                {
                                    if (0U < inFlight->second)
                                    {
                                        --inFlight->second;
                                    }
                                    if (0U == inFlight->second)
                                    {
                                        mLifecycleManager->mLifecycleStateNotificationInFlight.erase(mNotification);
                                        mLifecycleManager->mLifecycleStateNotificationDispatchingThread.erase(mNotification);
                                        mLifecycleManager->mLifecycleStateNotificationCV.notify_all();
                                    }
                                }
                            }
                        }

                    private:
                        LifecycleManagerImplementation* mLifecycleManager;
                        Exchange::ILifecycleManagerState::INotification* mNotification;
                        const uint32_t mGeneration;
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

                LifecycleManagerImplementation ();
                ~LifecycleManagerImplementation () override;
                LifecycleManagerImplementation (const LifecycleManagerImplementation &) = delete;
                LifecycleManagerImplementation & operator=(const LifecycleManagerImplementation &) = delete;

		BEGIN_INTERFACE_MAP(LifecycleManagerImplementation)
		INTERFACE_ENTRY(Exchange::ILifecycleManager)
                INTERFACE_ENTRY(Exchange::ILifecycleManagerState)
                INTERFACE_ENTRY(Exchange::IConfiguration)
	        END_INTERFACE_MAP

                /* ILifecycleManager methods  */
                virtual Core::hresult Register(Exchange::ILifecycleManager::INotification *notification) override;
                virtual Core::hresult Unregister(Exchange::ILifecycleManager::INotification *notification) override;
                virtual Core::hresult GetLoadedApps(const bool verbose, string& apps) override;
                virtual Core::hresult IsAppLoaded(const string& appId, bool& loaded) const override;
                virtual Core::hresult SpawnApp(const string& appId, const string& launchIntent, const Exchange::ILifecycleManager::LifecycleState targetLifecycleState, const string& runtimeConfigPayload, const string& launchArgs, string& appInstanceId, string& errorReason, bool& success) override;
                virtual Core::hresult SetTargetAppState(const string& appInstanceId, const Exchange::ILifecycleManager::LifecycleState targetLifecycleState, const string& launchIntent) override;
                virtual Core::hresult UnloadApp(const string& appInstanceId, string& errorReason, bool& success) override;
                virtual Core::hresult KillApp(const string& appInstanceId, string& errorReason, bool& success) override;
                virtual Core::hresult SendIntentToActiveApp(const string& appInstanceId, const string& intent, string& errorReason, bool& success) override;

                /* ILifecycleManagerState methods  */
                /** Register notification interface */
                virtual Core::hresult Register(Exchange::ILifecycleManagerState::INotification *notification) override;
                /** Unregister notification interface */
                virtual Core::hresult Unregister(Exchange::ILifecycleManagerState::INotification *notification) override;
		virtual Core::hresult AppReady(const string& appId) override;
		virtual Core::hresult StateChangeComplete(const string& appId, const uint32_t stateChangedId, const bool success) override;
		virtual Core::hresult CloseApp(const string& appId, const AppCloseReason closeReason) override;

                /* IConfiguration methods */
                uint32_t Configure(PluginHost::IShell* service) override;

                /* IEventHandler methods  */
                virtual void onRuntimeManagerEvent(JsonObject& data) override;
                virtual void onWindowManagerEvent(JsonObject& data) override;
                virtual void onRippleEvent(string name, JsonObject& data) override;
                virtual void onStateChangeEvent(JsonObject& data) override;

	    private: /* members */
                mutable Core::CriticalSection mAdminLock;
	        std::mutex mEventQueueLock;
                std::deque<std::pair<EventNames, JsonValue>> mEventQueue;
                bool mEventDispatchScheduled { false };
	        std::list<Exchange::ILifecycleManager::INotification*> mLifecycleManagerNotification;
	        std::list<Exchange::ILifecycleManagerState::INotification*> mLifecycleManagerStateNotification;
                /* Persistent per-listener FIFO: each subscriber keeps a queue in arrival order and
                 * a single scheduled drain job prevents overlapping callbacks and stale ordering. */
                std::map<Exchange::ILifecycleManager::INotification*, std::deque<AppStateNotification>> mAppStateNotifications;
                std::map<Exchange::ILifecycleManagerState::INotification*, std::deque<LifecycleStateNotification>> mLifecycleStateNotifications;
                std::map<Exchange::ILifecycleManager::INotification*, bool> mAppStateNotificationDrainScheduled;
                std::map<Exchange::ILifecycleManagerState::INotification*, bool> mLifecycleStateNotificationDrainScheduled;
                std::map<Exchange::ILifecycleManager::INotification*, uint32_t> mAppStateNotificationGeneration;
                std::map<Exchange::ILifecycleManagerState::INotification*, uint32_t> mLifecycleStateNotificationGeneration;
                std::map<Exchange::ILifecycleManager::INotification*, uint32_t> mAppStateNotificationInFlight;
                std::map<Exchange::ILifecycleManagerState::INotification*, uint32_t> mLifecycleStateNotificationInFlight;
                std::map<Exchange::ILifecycleManager::INotification*, std::thread::id> mAppStateNotificationDispatchingThread;
                std::map<Exchange::ILifecycleManagerState::INotification*, std::thread::id> mLifecycleStateNotificationDispatchingThread;
                std::mutex mAppStateNotificationLock;
                std::condition_variable mAppStateNotificationCV;
                std::mutex mLifecycleStateNotificationLock;
                std::condition_variable mLifecycleStateNotificationCV;
                std::list<std::shared_ptr<ApplicationContext>> mLoadedApplications;
                std::map<string, PendingRespawnRequest> mPendingRespawns;
                PluginHost::IShell* mService;
            private: /* internal methods */
                bool initialize(PluginHost::IShell* service);
                void terminate();
                void dispatchEvent(EventNames, const JsonValue &params);
                void drainEvents();
                void Dispatch(EventNames event, const JsonValue params);
                void handleRuntimeManagerEvent(const JsonObject &data);
                void notifyOnFailure(const string& appInstanceId, const string& errorCode);
                void handleStateChangeEvent(const JsonObject &data);
                bool tryGetPendingRespawn(const string& appInstanceId, PendingRespawnRequest& pendingRespawn);
                void submitStateChangeNotifications(const JsonObject& data, bool includeAppStateNotifications = true, bool includeLifecycleStateNotifications = true);
                void waitForAppStateNotificationDrain(Exchange::ILifecycleManager::INotification* notification);
                void waitForLifecycleStateNotificationDrain(Exchange::ILifecycleManagerState::INotification* notification);
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

