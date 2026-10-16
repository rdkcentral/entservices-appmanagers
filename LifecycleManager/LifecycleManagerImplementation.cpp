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

#include "LifecycleManagerImplementation.h"
#include "RequestHandler.h"
#include "UtilsJsonRpc.h"
#include <interfaces/json/JsonData_LifecycleManagerState.h>
#include <interfaces/json/JLifecycleManagerState.h>
#include <semaphore.h>
#include "LifecycleManagerTelemetryReporting.h"
#include "UtilsAppManagerTelemetry.h"

namespace WPEFramework
{
    namespace Plugin
    {
        RDKAM_DEFINE_TELEMETRY_CLIENT(WPEFramework::Plugin::LifecycleManagerTelemetryReporting, "lifecycleManagerBootstrapTime")

        SERVICE_REGISTRATION(LifecycleManagerImplementation, 1, 0);

        LifecycleManagerImplementation::LifecycleManagerImplementation(): mLifecycleManagerNotification(), mLifecycleManagerStateNotification(), mLoadedApplications(), mPendingRespawns(), mService(nullptr)
        {
            LOGINFO("Create LifecycleManagerImplementation Instance");
        }

        LifecycleManagerImplementation::~LifecycleManagerImplementation()
        {
            LOGINFO("Delete LifecycleManagerImplementation Instance");
            terminate();
        }

        bool LifecycleManagerImplementation::initialize(PluginHost::IShell* service)
        {
            bool ret = RequestHandler::getInstance()->initialize(service, this);
            RDKAM_TELEMETRY_INIT(service);
	    return ret;
        }

        void LifecycleManagerImplementation::terminate()
        {
            try
            {
                RequestHandler::getInstance()->terminate();
            }
            catch(const std::exception& e)
            {
                LOGERR("Exception in LifecycleManagerImplementation::terminate: %s", e.what());
            }
            if (mService != nullptr)
            {
               mService->Release();
               mService = nullptr;
            }
        }

        Core::hresult LifecycleManagerImplementation::Register(Exchange::ILifecycleManager::INotification *notification)
        {
            ASSERT (nullptr != notification);
        
            mAdminLock.Lock();
        
            /* Make sure we can't register the same notification callback multiple times */
            if (std::find(mLifecycleManagerNotification.begin(), mLifecycleManagerNotification.end(), notification) == mLifecycleManagerNotification.end())
            {
                LOGINFO("Register notification");
                mLifecycleManagerNotification.push_back(notification);
                notification->AddRef();
            }
        
            mAdminLock.Unlock();
        
            return Core::ERROR_NONE;
        }
        
        Core::hresult LifecycleManagerImplementation::Unregister(Exchange::ILifecycleManager::INotification *notification )
        {
            Core::hresult status = Core::ERROR_GENERAL;
        
            ASSERT (nullptr != notification);
        
            mAdminLock.Lock();
        
            /* Make sure we can't unregister the same notification callback multiple times */
            auto itr = std::find(mLifecycleManagerNotification.begin(), mLifecycleManagerNotification.end(), notification);
            if (itr != mLifecycleManagerNotification.end())
            {
                (*itr)->Release();
                LOGINFO("Unregister notification");
                mLifecycleManagerNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("notification not found");
            }
        
            mAdminLock.Unlock();
        
            return status;
        }
        
        void LifecycleManagerImplementation::dispatchEvent(EventNames event, const JsonValue &params)
        {
            bool schedule = false;
            {
                std::lock_guard<std::mutex> lock(mEventQueueLock);
                mEventQueue.emplace_back(event, params);
                if (false == mEventDispatchScheduled)
                {
                    mEventDispatchScheduled = true;
                    schedule = true;
                }
            }
            if (schedule)
            {
                Core::IWorkerPool::Instance().Submit(Job::Create(this));
            }
        }

        void LifecycleManagerImplementation::drainEvents()
        {
            while (true)
            {
                std::pair<EventNames, JsonValue> next;
                {
                    std::lock_guard<std::mutex> lock(mEventQueueLock);
                    if (mEventQueue.empty())
                    {
                        mEventDispatchScheduled = false;
                        return;
                    }
                    next = std::move(mEventQueue.front());
                    mEventQueue.pop_front();
                }
                Dispatch(next.first, next.second);
            }
        }
        
        void LifecycleManagerImplementation::Dispatch(EventNames event, const JsonValue params)
        {
             JsonObject obj = params.Object();
               PendingRespawnRequest pendingRespawn;
               bool shouldRespawn = false;

             //below is for ILifecycleManager notification
             string appId(obj["appId"].String());
             string errorReason(obj["errorReason"].String());
             uint32_t newLifecycleState(obj["newLifecycleState"].Number());

             //below is additional for ILifecycleManagerState notification
             string appInstanceId(obj["appInstanceId"].String());
             uint32_t oldLifecycleState(obj["oldLifecycleState"].Number());
             string navigationIntent(obj["navigationIntent"].String());
             auto contextHolder = getContext("", appId);
             ApplicationContext* context = contextHolder.get();

             mAdminLock.Lock();
        
             std::list<Exchange::ILifecycleManager::INotification*>::const_iterator index(mLifecycleManagerNotification.begin());
             std::list<Exchange::ILifecycleManagerState::INotification*>::const_iterator stateNotificationIndex(mLifecycleManagerStateNotification.begin());
        
             switch(event)
             {
                 case LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED:
                 {
                     LifecycleManagerTelemetryReporting::getInstance().reportTelemetryDataOnStateChange(context, obj);
                     handleStateChangeEvent(obj);
                     if (Exchange::ILifecycleManager::LifecycleState::UNLOADED == static_cast<Exchange::ILifecycleManager::LifecycleState>(newLifecycleState))
                     {
                         shouldRespawn = tryGetPendingRespawn(appInstanceId, pendingRespawn);
                     }
                     while (index != mLifecycleManagerNotification.end())
                     {
                         (*index)->OnAppStateChanged(appId, (LifecycleState)newLifecycleState, errorReason);
                         ++index;
                     }
                    const bool isUnloadedState = (Exchange::ILifecycleManager::LifecycleState::UNLOADED == static_cast<Exchange::ILifecycleManager::LifecycleState>(newLifecycleState));
                    const bool isUnexpectedTermination = (nullptr != context) && context->getUnexpectedTermination();
                    const string effectiveNavigationIntent = (isUnloadedState && isUnexpectedTermination) ? "unexpectedTermination" : navigationIntent;
                    while (stateNotificationIndex != mLifecycleManagerStateNotification.end())
                    {
                        (*stateNotificationIndex)->OnAppLifecycleStateChanged(appId, appInstanceId, (LifecycleState)oldLifecycleState, (LifecycleState)newLifecycleState, effectiveNavigationIntent);
                        ++stateNotificationIndex;
                    }
                    if ((nullptr != context) && isUnloadedState && isUnexpectedTermination) {
                        context->setUnexpectedTermination(false);
                    }
                    break;
                 }
                 case LIFECYCLE_MANAGER_EVENT_RUNTIME:
                     handleRuntimeManagerEvent(obj);
                     break;
                 case LIFECYCLE_MANAGER_EVENT_WINDOW:
                      handleWindowManagerEvent(obj);
                      break;
                 case LIFECYCLE_MANAGER_EVENT_ONFAILURE:
                      while (index != mLifecycleManagerNotification.end())
                      {
                          (*index)->OnAppStateChanged(appId, (LifecycleState)newLifecycleState, errorReason);
                          ++index;
                      }
                      break;
                 default:
                     LOGWARN("Event[%u] not handled appId=%s appInstanceId=%s", event, appId.c_str(), appInstanceId.c_str());
                     break;
             }
        
             mAdminLock.Unlock();

             if (shouldRespawn)
             {
                 handlePendingRespawn(pendingRespawn);
             }
        }
        
        Core::hresult LifecycleManagerImplementation::GetLoadedApps(const bool verbose, string& apps)
        {
            Core::hresult status = Core::ERROR_NONE;
            JsonArray appsInformation;
            Core::SafeSyncType<Core::CriticalSection> adminLock(mAdminLock);
            auto iter = mLoadedApplications.end();
            for (iter = mLoadedApplications.begin(); iter != mLoadedApplications.end(); iter++)
            {
                if (nullptr != *iter)
                {
                    JsonObject appData;
                    ApplicationContext* context = (*iter).get();
                    appData["appInstanceID"] = context->getAppInstanceId();
                    appData["appId"] = context->getAppId();
                    struct timespec lastStateChangeTime = context->getLastLifecycleStateChangeTime();
                    char timeData[200];
                    char timeDataExpanded[1000];
                    memset(timeData, 0, sizeof(timeData));
                    memset(timeDataExpanded, 0, sizeof(timeDataExpanded));
                    strftime(timeData, sizeof timeData, "%D %T", gmtime(&lastStateChangeTime.tv_sec));
                    sprintf(timeDataExpanded, "%s.%09ld", timeData, lastStateChangeTime.tv_nsec);
                    appData["lifecycleState"] = (uint32_t) context->getCurrentLifecycleState();
                    appData["timeOfLastLifecycleStateChange"] = string(timeDataExpanded);
                    appData["activeSessionId"] = context->getActiveSessionId();
                    appData["targetLifecycleState"] = (uint32_t) context->getTargetLifecycleState();
                    appData["mostRecentIntent"] = context->getMostRecentIntent();
                    if (verbose)
                    {
                        RuntimeManagerHandler* runtimeManagerHandler = RequestHandler::getInstance()->getRuntimeManagerHandler();
                        if (nullptr != runtimeManagerHandler)
                        {
                            string runtimeStats("");
                            bool runtimeStatsResult = runtimeManagerHandler->getRuntimeStats(context->getAppInstanceId(), runtimeStats);
                            if (true == runtimeStatsResult)
                            {
                                appData["runtimeStats"] = runtimeStats;
                            }
                            else
                            {
                                LOGWARN("Unable to get runtime status for appInstanceId=%s", context->getAppInstanceId().c_str());
                            }
                        }
                    }
                    appsInformation.Add(appData);
                }
            }
            appsInformation.ToString(apps);
            return status;
        }
        
        Core::hresult LifecycleManagerImplementation::IsAppLoaded(const string& appId, bool& loaded) const
        {
            Core::hresult status = Core::ERROR_NONE;
            loaded = false;
            auto context = getContext("", appId);
            if (nullptr != context)
            {
                loaded = true;
            }
            return status;
        }
        
       /* Core::hresult LifecycleManagerImplementation::SpawnApp(const string& appId, const string& launchIntent, const LifecycleState targetLifecycleState, const WPEFramework::Exchange::RuntimeConfig& runtimeConfigObject, const string& launchArgs, string& appInstanceId, string& errorReason, bool& success)
        {
	    // Launches an app.  This will be an asynchronous call.
            // Notifies appropriate API Gateway when an app is about to be loaded
            // Lifecycle manager will create the appInstanceId once the app is loaded.  Ripple is responsible for creating a token. 
            Core::hresult status = Core::ERROR_NONE;
            bool firstLaunch = false;
            time_t requestTime = 0;
            requestTime = LifecycleManagerTelemetryReporting::getInstance().getCurrentTimestampMs();
            auto context = getContext("", appId);
            mAdminLock.Lock();
            if (nullptr == context)
	    {
                context = std::make_shared<ApplicationContext>(appId);
                context->setApplicationLaunchParams(appId, launchIntent, launchArgs, targetLifecycleState, runtimeConfigObject);
		mLoadedApplications.push_back(context);
                firstLaunch = true;
	    }
            else if (context->mPendingStateTransition)
            {
                mAdminLock.Unlock();
                return Core::ERROR_GENERAL;
            }
            else if (context->getCurrentLifecycleState() == targetLifecycleState &&
                     !launchIntent.empty() &&
                     context->getMostRecentIntent() != launchIntent)
            {
                // App is already in the target state with a different intent.
                // Skip the state transition and dispatch only the intent event so
                // subscribers (e.g. AppManager) are notified of the new intent.
                LOGINFO("SpawnApp: appId=%s already in target state=%d with different intent, dispatching intent-only event",
                        appId.c_str(), static_cast<int>(targetLifecycleState));
                context->setMostRecentIntent(launchIntent);
                appInstanceId = context->getAppInstanceId();
                success = true;
                mAdminLock.Unlock();

                JsonObject eventData;
                eventData["appId"] = context->getAppId();
                eventData["appInstanceId"] = context->getAppInstanceId();
                eventData["oldLifecycleState"] = static_cast<uint32_t>(context->getCurrentLifecycleState());
                eventData["newLifecycleState"] = static_cast<uint32_t>(context->getCurrentLifecycleState());
                eventData["navigationIntent"] = launchIntent;
                eventData["errorReason"] = "";
                dispatchEvent(LifecycleManagerImplementation::EventNames::LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED, eventData);
                return status;
            }			
            context->setRequestTime(requestTime);
            context->setRequestType(REQUEST_TYPE_LAUNCH);
            context->setTargetLifecycleState(targetLifecycleState);
            context->setMostRecentIntent(launchIntent);
            context->resetPendingStates();
            success = RequestHandler::getInstance()->launch(context.get(), launchIntent, targetLifecycleState, errorReason);
            if (!success)
	    {
                status = Core::ERROR_GENERAL;
	    }
            else
	    {
                if (firstLaunch)
		{
                    sem_wait(&context->mReachedLoadingStateSemaphore);
		}
                appInstanceId = context->getAppInstanceId();
            }
            mAdminLock.Unlock();
            return status;
        }*/

	Core::hresult LifecycleManagerImplementation::SpawnApp(
    const string& appId,
    const string& launchIntent,
    const LifecycleState targetLifecycleState,
    const WPEFramework::Exchange::RuntimeConfig& runtimeConfigObject,
    const string& launchArgs,
    string& appInstanceId,
    string& errorReason,
    bool& success)
{
    /*
     * Launches an application asynchronously.
     *
     * For suspended package replacement, the first SpawnApp() call only
     * updates the existing ApplicationContext with the new package
     * configuration. CloseApp(KILL_AND_ACTIVATE) then performs the actual
     * kill and deferred respawn.
     */
    Core::hresult status = Core::ERROR_NONE;
    bool firstLaunch = false;

    const time_t requestTime =
        LifecycleManagerTelemetryReporting::getInstance()
            .getCurrentTimestampMs();

    auto context = getContext("", appId);

    mAdminLock.Lock();

    if (nullptr == context)
    {
        /*
         * Normal first-launch or deferred-respawn path.
         *
         * After the old instance reaches UNLOADED, its context is removed.
         * Therefore, handlePendingRespawn() reaches this branch and creates
         * a new context with the prepared replacement configuration.
         */
        context =
            std::make_shared<ApplicationContext>(appId);

        if (nullptr == context)
        {
            LOGERR(
                "SpawnApp failed: unable to create context "
                "for appId=%s",
                appId.c_str());

            mAdminLock.Unlock();

            return Core::ERROR_GENERAL;
        }

        context->setApplicationLaunchParams(
            appId,
            launchIntent,
            launchArgs,
            targetLifecycleState,
            runtimeConfigObject);

        mLoadedApplications.push_back(context);

        firstLaunch = true;

        LOGINFO(
            "[REPLACEMENT_DEBUG] Created new application context: "
            "appId=%s targetState=%d path=%s",
            appId.c_str(),
            static_cast<int>(targetLifecycleState),
            runtimeConfigObject.unpackedPath.c_str());
    }
    else if (context->mPendingStateTransition)
    {
        /*
         * Do not modify the context or start another transition while
         * an earlier lifecycle operation is still pending.
         */
        LOGERR(
            "SpawnApp failed: lifecycle transition is pending "
            "appId=%s instanceId=%s",
            appId.c_str(),
            context->getAppInstanceId().c_str());

        mAdminLock.Unlock();

        return Core::ERROR_GENERAL;
    }
    else
    {
        ApplicationLaunchParams& currentLaunchParams =
            context->getApplicationLaunchParams();

        const bool isSuspended =
            (Exchange::ILifecycleManager::
                 LifecycleState::SUSPENDED ==
             context->getCurrentLifecycleState());

        const bool activeRequested =
            (Exchange::ILifecycleManager::
                 LifecycleState::ACTIVE ==
             targetLifecycleState);

        const bool validNewPath =
            !runtimeConfigObject.unpackedPath.empty();

        const bool packagePathChanged =
            validNewPath &&
            (currentLaunchParams
                 .mRuntimeConfigObject
                 .unpackedPath !=
             runtimeConfigObject.unpackedPath);

        /*
         * Special package-version replacement preparation.
         *
         * Execute only when:
         * 1. The app already has a context.
         * 2. The current app is SUSPENDED.
         * 3. ACTIVE is requested.
         * 4. The incoming package path is valid.
         * 5. The incoming package path differs from the stored path.
         *
         * This branch does not launch a container and does not generate
         * a new app instance ID. It only refreshes the existing context.
         *
         * The caller must immediately invoke:
         *
         * CloseApp(appId, KILL_AND_ACTIVATE)
         *
         * CloseApp() then captures these updated parameters, kills the
         * old instance, and performs the real SpawnApp() only after the
         * old instance reaches UNLOADED.
         */
        const bool replacementPreparation =
            isSuspended &&
            activeRequested &&
            packagePathChanged;

        if (replacementPreparation)
        {
            const string oldPath =
                currentLaunchParams
                    .mRuntimeConfigObject
                    .unpackedPath;

            const string oldInstanceId =
                context->getAppInstanceId();

            LOGINFO(
                "[REPLACEMENT_DEBUG] Preparing suspended replacement: "
                "appId=%s instanceId=%s oldPath=%s newPath=%s "
                "launchArgsSize=%zu intentSize=%zu",
                appId.c_str(),
                oldInstanceId.c_str(),
                oldPath.c_str(),
                runtimeConfigObject.unpackedPath.c_str(),
                launchArgs.size(),
                launchIntent.size());

            /*
             * Update all cached launch parameters consistently:
             * app ID, intent, launch arguments, target state and the
             * replacement RuntimeConfig.
             */
            context->setApplicationLaunchParams(
                appId,
                launchIntent,
                launchArgs,
                targetLifecycleState,
                runtimeConfigObject);

            /*
             * Return the existing suspended instance ID.
             * This proves that no replacement instance was created yet.
             */
            appInstanceId =
                oldInstanceId;

            success = true;
            errorReason.clear();

            mAdminLock.Unlock();

            LOGINFO(
                "[REPLACEMENT_DEBUG] Suspended context updated: "
                "appId=%s instanceId=%s oldPath=%s newPath=%s; "
                "no container started, waiting for "
                "CloseApp(KILL_AND_ACTIVATE)",
                appId.c_str(),
                appInstanceId.c_str(),
                oldPath.c_str(),
                runtimeConfigObject.unpackedPath.c_str());

            return Core::ERROR_NONE;
        }

        /*
         * Preserve the existing intent-only behavior.
         *
         * If the app is already in the requested state and only the
         * navigation intent changed, avoid an unnecessary state
         * transition and notify subscribers about the new intent.
         */
        const bool intentOnlyUpdate =
            (context->getCurrentLifecycleState() ==
             targetLifecycleState) &&
            !launchIntent.empty() &&
            (context->getMostRecentIntent() !=
             launchIntent);

        if (intentOnlyUpdate)
        {
            LOGINFO(
                "SpawnApp: appId=%s already in target state=%d "
                "with different intent, dispatching intent-only event",
                appId.c_str(),
                static_cast<int>(targetLifecycleState));

            context->setMostRecentIntent(
                launchIntent);

            appInstanceId =
                context->getAppInstanceId();

            success = true;
            errorReason.clear();

            /*
             * Capture event fields before releasing the lock.
             */
            const string contextAppId =
                context->getAppId();

            const string contextInstanceId =
                context->getAppInstanceId();

            const uint32_t currentState =
                static_cast<uint32_t>(
                    context->getCurrentLifecycleState());

            mAdminLock.Unlock();

            JsonObject eventData;

            eventData["appId"] =
                contextAppId;

            eventData["appInstanceId"] =
                contextInstanceId;

            eventData["oldLifecycleState"] =
                currentState;

            eventData["newLifecycleState"] =
                currentState;

            eventData["navigationIntent"] =
                launchIntent;

            eventData["errorReason"] = "";

            dispatchEvent(
                LifecycleManagerImplementation::EventNames::
                    LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED,
                eventData);

            return Core::ERROR_NONE;
        }
    }

    /*
     * Existing normal launch/resume flow.
     *
     * This executes for:
     * - First launch
     * - Actual deferred respawn after UNLOADED
     * - Same-version resume
     * - Other valid lifecycle transitions
     */
    context->setRequestTime(
        requestTime);

    context->setRequestType(
        REQUEST_TYPE_LAUNCH);

    context->setTargetLifecycleState(
        targetLifecycleState);

    context->setMostRecentIntent(
        launchIntent);

    context->resetPendingStates();

    LOGINFO(
        "[REPLACEMENT_DEBUG] Calling RequestHandler launch: "
        "appId=%s firstLaunch=%d currentState=%d "
        "targetState=%d path=%s",
        appId.c_str(),
        firstLaunch,
        static_cast<int>(
            context->getCurrentLifecycleState()),
        static_cast<int>(
            targetLifecycleState),
        context->getApplicationLaunchParams()
            .mRuntimeConfigObject
            .unpackedPath.c_str());

    success =
        RequestHandler::getInstance()->launch(
            context.get(),
            launchIntent,
            targetLifecycleState,
            errorReason);

    if (!success)
    {
        status = Core::ERROR_GENERAL;

        LOGERR(
            "SpawnApp failed: RequestHandler launch failed "
            "appId=%s targetState=%d path=%s error=%s",
            appId.c_str(),
            static_cast<int>(targetLifecycleState),
            context->getApplicationLaunchParams()
                .mRuntimeConfigObject
                .unpackedPath.c_str(),
            errorReason.c_str());
    }
    else
    {
        if (firstLaunch)
        {
            /*
             * Preserve the existing behavior. Wait until the newly
             * created instance reaches LOADING and receives its ID.
             */
            sem_wait(
                &context->mReachedLoadingStateSemaphore);
        }

        appInstanceId =
            context->getAppInstanceId();

        LOGINFO(
            "[REPLACEMENT_DEBUG] RequestHandler launch accepted: "
            "appId=%s instanceId=%s firstLaunch=%d "
            "targetState=%d path=%s",
            appId.c_str(),
            appInstanceId.c_str(),
            firstLaunch,
            static_cast<int>(targetLifecycleState),
            context->getApplicationLaunchParams()
                .mRuntimeConfigObject
                .unpackedPath.c_str());
    }

    mAdminLock.Unlock();

    return status;
}
        
        Core::hresult LifecycleManagerImplementation::SetTargetAppState(const string& appInstanceId, const LifecycleState targetLifecycleState, const string& launchIntent)
        {
            // Moves a currently loaded app between states
            Core::hresult status = Core::ERROR_NONE;
            auto context = getContext(appInstanceId, "");
            time_t requestTime = 0;
            requestTime = LifecycleManagerTelemetryReporting::getInstance().getCurrentTimestampMs();
            if (nullptr == context)
	    {
                LOGERR("SetTargetAppState failed: context not found for appInstanceId=%s", appInstanceId.c_str());
                status = Core::ERROR_GENERAL;
                return status;
	    }
            mAdminLock.Lock();
            if (targetLifecycleState == context->getCurrentLifecycleState())
            {
                mAdminLock.Unlock();
                return status;
            }
            if (context->mPendingStateTransition)
            {
                mAdminLock.Unlock();
                status = Core::ERROR_GENERAL;
                return status;
            }
            switch(targetLifecycleState)
            {
                case Exchange::ILifecycleManager::LifecycleState::PAUSED:        //before SUSPEND or HIBERNATE app will be PAUSED
                    context->setRequestTime(requestTime);
                    context->setRequestType(REQUEST_TYPE_PAUSE);
                break;
                case Exchange::ILifecycleManager::LifecycleState::SUSPENDED:
                    if (Exchange::ILifecycleManager::LifecycleState::HIBERNATED == context->getCurrentLifecycleState())
                    {
                        context->setRequestTime(requestTime);
                        context->setRequestType(REQUEST_TYPE_WAKE);
                    }
                    else
                    {
                        context->setRequestType(REQUEST_TYPE_SUSPEND);
                    }
                break;
                case Exchange::ILifecycleManager::LifecycleState::HIBERNATED:
                    context->setRequestType(REQUEST_TYPE_HIBERNATE);
                break;
                case Exchange::ILifecycleManager::LifecycleState::ACTIVE:
                    context->setRequestTime(requestTime);
                    context->setRequestType(REQUEST_TYPE_RESUME);
                break;
                default:
                    LOGERR("Invalid targetLifecycleState=%d for appId=%s currentState=%d", static_cast<int>(targetLifecycleState), context->getAppId().c_str(), static_cast<int>(context->getCurrentLifecycleState()));
                break;
            }
            string errorReason("");
            context->setTargetLifecycleState(targetLifecycleState);
			if (!launchIntent.empty())
			{
                context->setMostRecentIntent(launchIntent);
			}
            context->resetPendingStates();
            bool success = RequestHandler::getInstance()->updateState(context.get(), targetLifecycleState, errorReason);
            mAdminLock.Unlock();
            if (false == success)
            {
                LOGERR("SetTargetAppState failed: updateState returned false appInstanceId=%s appId=%s targetState=%d errorReason=%s", appInstanceId.c_str(), context->getAppId().c_str(), static_cast<int>(targetLifecycleState), errorReason.c_str());
                status = Core::ERROR_GENERAL;
                return status;
            }
            return status;
        }
        
        Core::hresult LifecycleManagerImplementation::UnloadApp(const string& appInstanceId, string& errorReason, bool& success)
        {
            // Begins a graceful shutdown of the app.  Moves the app through the lifecycle states till it ultimately ends in app container being terminated.
            // This is an asynchronous call, clients should use the onAppStateChange event to determine when the app is actually terminated.
            // This moves an app to the unloaded state
            Core::hresult status = Core::ERROR_NONE;
            auto context = getContext(appInstanceId, "");
            time_t requestTime = 0;
            requestTime = LifecycleManagerTelemetryReporting::getInstance().getCurrentTimestampMs();
            if (nullptr == context)
	    {
                status = Core::ERROR_GENERAL;
                success = false;
                return status;
	    }
            mAdminLock.Lock();
            if (context->mPendingStateTransition)
            {
                mAdminLock.Unlock();
                success = false;
                return Core::ERROR_GENERAL;
            }
            if(REQUEST_TYPE_PAUSE != context->getRequestType())   //If request through AppManager closeApp, requestTime is already set
            {
                context->setRequestTime(requestTime);
            }
            context->setRequestType(REQUEST_TYPE_TERMINATE);
            context->setTargetLifecycleState(Exchange::ILifecycleManager::LifecycleState::TERMINATING);
            context->setApplicationKillParams(false);

            context->resetPendingStates();
            success = RequestHandler::getInstance()->terminate(context.get(), false, errorReason);
            if (!success)
	    {
                status = Core::ERROR_GENERAL;
	    }
            mAdminLock.Unlock();
            return status;
        }
        
        Core::hresult LifecycleManagerImplementation::KillApp(const string& appInstanceId, string& errorReason, bool& success)
        {
            Core::hresult status = Core::ERROR_NONE;
            auto context = getContext(appInstanceId, "");
            time_t requestTime =0;
            requestTime = LifecycleManagerTelemetryReporting::getInstance().getCurrentTimestampMs();

            if (nullptr == context)
	    {
                status = Core::ERROR_GENERAL;
                success = false;
                return status;
	    }
            mAdminLock.Lock();
            if (context->mPendingStateTransition)
            {
                mAdminLock.Unlock();
                success = false;
                return Core::ERROR_GENERAL;
            }
            context->setRequestTime(requestTime);
            context->setRequestType(REQUEST_TYPE_TERMINATE);
            context->setTargetLifecycleState(Exchange::ILifecycleManager::LifecycleState::TERMINATING);
            context->setApplicationKillParams(true);
            context->resetPendingStates();
            success = RequestHandler::getInstance()->terminate(context.get(), true, errorReason);
            mAdminLock.Unlock();
            return status;
        }
        
        Core::hresult LifecycleManagerImplementation::SendIntentToActiveApp(const string& appInstanceId, const string& intent, string& errorReason, bool& success)
        {
            // Sends arguments to a launched app.  This API is used for sending deeplinks to an application.  This can be sent to apps in any lifecycle state (ACTIVE or non-ACTIVE).
            Core::hresult status = Core::ERROR_NONE;
            auto context = getContext(appInstanceId, "");
            if (nullptr == context)
	    {
                status = Core::ERROR_GENERAL;
                success = false;
                return status;
	    }

            // Get the current lifecycle state for event reporting
            Exchange::ILifecycleManager::LifecycleState currentState = context->getCurrentLifecycleState();

            JsonObject eventData;
            eventData["appId"] = context->getAppId();
            eventData["appInstanceId"] = context->getAppInstanceId();
            eventData["oldLifecycleState"] = (uint32_t)currentState;
            eventData["newLifecycleState"] = (uint32_t)currentState;
            eventData["navigationIntent"] = intent;
            eventData["errorReason"] = "";
            dispatchEvent(LifecycleManagerImplementation::EventNames::LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED, eventData);
            success = true;
            status = Core::ERROR_NONE;
            return status;
        }

        Core::hresult LifecycleManagerImplementation::Register(Exchange::ILifecycleManagerState::INotification *notification)
        {
            ASSERT (nullptr != notification);
        
            mAdminLock.Lock();
        
            /* Make sure we can't register the same notification callback multiple times */
            if (std::find(mLifecycleManagerStateNotification.begin(), mLifecycleManagerStateNotification.end(), notification) == mLifecycleManagerStateNotification.end())
            {
                LOGINFO("Register notification");
                mLifecycleManagerStateNotification.push_back(notification);
                notification->AddRef();
            }
        
            mAdminLock.Unlock();
        
            return Core::ERROR_NONE;
        }
        
        Core::hresult LifecycleManagerImplementation::Unregister(Exchange::ILifecycleManagerState::INotification *notification )
        {
            Core::hresult status = Core::ERROR_GENERAL;
        
            ASSERT (nullptr != notification);
        
            mAdminLock.Lock();
        
            /* Make sure we can't unregister the same notification callback multiple times */
            auto itr = std::find(mLifecycleManagerStateNotification.begin(), mLifecycleManagerStateNotification.end(), notification);
            if (itr != mLifecycleManagerStateNotification.end())
            {
                (*itr)->Release();
                LOGINFO("Unregister notification");
                mLifecycleManagerStateNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("notification not found");
            }
        
            mAdminLock.Unlock();
        
            return status;
        }
        
	Core::hresult LifecycleManagerImplementation::AppReady(const string& appId)
        {
            Core::hresult status = Core::ERROR_NONE;
	    LOGINFO("[LifecycleManager] Received appReady event for [%s]", appId.c_str());
            auto context = getContext("", appId);
            if (nullptr == context)
	    {
                status = Core::ERROR_GENERAL;
                return status;
	    }
            printf("[LifecycleManager] Received AppReady event appId[%s] appInstanceId[%s] pending[%d] pendingEvent[%s]\n",
                   context->getAppId().c_str(),
                   context->getAppInstanceId().c_str(),
                   context->mPendingStateTransition,
                   context->mPendingEventName.c_str());
            fflush(stdout);
            if ((true == context->mPendingStateTransition) && (0 == context->mPendingEventName.compare("onAppReady")))
            {
                addStateTransitionRequest(context.get(), "onAppReady");
            }
	    return status;
	}

        Core::hresult LifecycleManagerImplementation::StateChangeComplete(const string& appId, const uint32_t stateChangedId, const bool success)
        {
            Core::hresult status = Core::ERROR_NONE;
	    return status;
	}

/*	Core::hresult LifecycleManagerImplementation::CloseApp(const string& appId, const Exchange::ILifecycleManagerState::AppCloseReason closeReason)
        {
            Core::hresult status = Core::ERROR_NONE;
            auto context = getContext("", appId);
            if (nullptr == context)
	    {
                status = Core::ERROR_GENERAL;
                return status;
	    }
	    const string appInstanceId = context->getAppInstanceId();
        const bool shouldRespawn = ((closeReason == KILL_AND_RUN) || (closeReason == KILL_AND_ACTIVATE));
        ApplicationLaunchParams launchParams;
        if (true == shouldRespawn)
        {
            launchParams = context->getApplicationLaunchParams();
        }
	    bool success = false;
            bool activate = false;
            string errorReason("");
	    status = KillApp(appInstanceId, errorReason, success); 
	    if ((Core::ERROR_NONE != status) || (false == success))
	    {
                if (Core::ERROR_NONE == status)
                {
                    status = Core::ERROR_GENERAL;
                }
                mAdminLock.Lock();
                mPendingRespawns.erase(appInstanceId);
                mAdminLock.Unlock();
                LOGERR("Failed to close app appId=%s: status=%d success=%d error=%s", appId.c_str(), status, success, errorReason.c_str());
                return status;
	    }
	    if (false == shouldRespawn)
	    {
                mAdminLock.Lock();
                mPendingRespawns.erase(appInstanceId);
                mAdminLock.Unlock();
                return status;
	    }
            activate = (closeReason == KILL_AND_ACTIVATE);		    

	    PendingRespawnRequest pendingRespawn;
            pendingRespawn.mLaunchParams = launchParams;
            pendingRespawn.mLaunchParams.mTargetState = activate ? Exchange::ILifecycleManager::LifecycleState::ACTIVE : Exchange::ILifecycleManager::LifecycleState::PAUSED;
            pendingRespawn.mLaunchParams.mLaunchIntent = "";  // Clear intent for respawn to prevent runtime manager from auto-resuming

            mAdminLock.Lock();
            mPendingRespawns[appInstanceId] = pendingRespawn;
            mAdminLock.Unlock();

	    return status;
	}*/

	Core::hresult LifecycleManagerImplementation::CloseApp(
    const string& appId,
    const Exchange::ILifecycleManagerState::AppCloseReason closeReason)
{
    Core::hresult status = Core::ERROR_NONE;

    auto context = getContext("", appId);

    if (nullptr == context)
    {
        LOGERR(
            "CloseApp failed: context not found for appId=%s",
            appId.c_str());

        return Core::ERROR_GENERAL;
    }

    const string appInstanceId =
        context->getAppInstanceId();

    if (appInstanceId.empty())
    {
        LOGERR(
            "CloseApp failed: instanceId is empty for appId=%s",
            appId.c_str());

        return Core::ERROR_GENERAL;
    }

    const bool shouldRespawn =
        ((closeReason == KILL_AND_RUN) ||
         (closeReason == KILL_AND_ACTIVATE));

    LOGINFO(
        "[REPLACEMENT_DEBUG] CloseApp entered: "
        "appId=%s instanceId=%s closeReason=%d "
        "shouldRespawn=%d",
        appId.c_str(),
        appInstanceId.c_str(),
        static_cast<int>(closeReason),
        shouldRespawn);

    /*
     * Register the respawn request before starting the asynchronous kill.
     * The UNLOADED event can otherwise arrive before this map is updated.
     */
    if (shouldRespawn)
    {
        PendingRespawnRequest pendingRespawn;

        pendingRespawn.mLaunchParams =
            context->getApplicationLaunchParams();

        pendingRespawn.mLaunchParams.mTargetState =
            (closeReason == KILL_AND_ACTIVATE)
                ? Exchange::ILifecycleManager::
                    LifecycleState::ACTIVE
                : Exchange::ILifecycleManager::
                    LifecycleState::PAUSED;

        /*
         * Keep this only if the existing runtime contract requires
         * the intent to be cleared during respawn.
         */
        pendingRespawn.mLaunchParams.mLaunchIntent = "";

        LOGINFO(
            "[REPLACEMENT_DEBUG] Registering pending respawn: "
            "appId=%s oldInstanceId=%s targetState=%d "
            "path=%s launchArgsSize=%zu envCount=%zu",
            appId.c_str(),
            appInstanceId.c_str(),
            static_cast<int>(
                pendingRespawn.mLaunchParams.mTargetState),
            pendingRespawn.mLaunchParams
                .mRuntimeConfigObject.unpackedPath.c_str(),
            pendingRespawn.mLaunchParams.mLaunchArgs.size(),
            pendingRespawn.mLaunchParams
                .mRuntimeConfigObject.envVariables.size());

        mAdminLock.Lock();
        mPendingRespawns[appInstanceId] =
            pendingRespawn;
        const size_t pendingCount =
            mPendingRespawns.size();
        mAdminLock.Unlock();

        LOGINFO(
            "[REPLACEMENT_DEBUG] Pending respawn registered "
            "before KillApp: appId=%s oldInstanceId=%s "
            "pendingCount=%zu",
            appId.c_str(),
            appInstanceId.c_str(),
            pendingCount);
    }
    else
    {
        /*
         * USER_EXIT and ERROR must not retain an earlier pending
         * respawn request.
         */
        mAdminLock.Lock();
        const size_t erased =
            mPendingRespawns.erase(appInstanceId);
        mAdminLock.Unlock();

        LOGINFO(
            "[REPLACEMENT_DEBUG] Non-respawn close: "
            "appId=%s instanceId=%s closeReason=%d "
            "stalePendingErased=%zu",
            appId.c_str(),
            appInstanceId.c_str(),
            static_cast<int>(closeReason),
            erased);
    }

    bool success = false;
    string errorReason("");

    LOGINFO(
        "[REPLACEMENT_DEBUG] Calling KillApp: "
        "appId=%s instanceId=%s",
        appId.c_str(),
        appInstanceId.c_str());

    status = KillApp(
        appInstanceId,
        errorReason,
        success);

    LOGINFO(
        "[REPLACEMENT_DEBUG] KillApp returned: "
        "appId=%s instanceId=%s status=%d "
        "success=%d error=%s",
        appId.c_str(),
        appInstanceId.c_str(),
        status,
        success,
        errorReason.empty()
            ? "<empty>"
            : errorReason.c_str());

    if ((Core::ERROR_NONE != status) ||
        (false == success))
    {
        /*
         * Remove the pending request because no successful termination
         * and corresponding UNLOADED event can be expected.
         */
        mAdminLock.Lock();
        const size_t erased =
            mPendingRespawns.erase(appInstanceId);
        const size_t pendingCount =
            mPendingRespawns.size();
        mAdminLock.Unlock();

        if (Core::ERROR_NONE == status)
        {
            status = Core::ERROR_GENERAL;
        }

        LOGERR(
            "[REPLACEMENT_DEBUG] CloseApp failed: "
            "appId=%s instanceId=%s status=%d "
            "success=%d error=%s pendingErased=%zu "
            "pendingCount=%zu",
            appId.c_str(),
            appInstanceId.c_str(),
            status,
            success,
            errorReason.empty()
                ? "<empty>"
                : errorReason.c_str(),
            erased,
            pendingCount);

        return status;
    }

    LOGINFO(
        "[REPLACEMENT_DEBUG] Kill initiated successfully: "
        "appId=%s oldInstanceId=%s shouldRespawn=%d",
        appId.c_str(),
        appInstanceId.c_str(),
        shouldRespawn);

    return Core::ERROR_NONE;
}

        uint32_t LifecycleManagerImplementation::Configure(PluginHost::IShell* service)
        {
            uint32_t result = Core::ERROR_GENERAL;
            if (nullptr == service)
            {
                terminate();
                return Core::ERROR_NONE;
            }

            if (nullptr != mService)
            {
                LOGWARN("LifecycleManagerImplementation already configured");
                return Core::ERROR_NONE;
            }

            mService = service;
            mService->AddRef();

            bool ret = initialize(service);
	    if (ret)
	    {
                result = Core::ERROR_NONE;
	    }
            else
	    {
                LOGERR("Unable to configure LifecycleManager");
	    }
            return result;
        }

        std::shared_ptr<ApplicationContext> LifecycleManagerImplementation::getContext(const string& appInstanceId, const string& appId) const
	{
            std::shared_ptr<ApplicationContext> context = nullptr;
            mAdminLock.Lock();
            auto iter = mLoadedApplications.end();
	    for (iter = mLoadedApplications.begin(); iter != mLoadedApplications.end(); iter++)
	    {
                if (nullptr != *iter)
		{
                    if ((!appInstanceId.empty()) && ((*iter)->getAppInstanceId() == appInstanceId))
                    {
		        context = *iter;
		        break;
                    }
		    else if ((!appId.empty()) && ((*iter)->getAppId() == appId))
                    {
		        context = *iter;
		        break;
                    }
		}
	    }
            mAdminLock.Unlock();
	    return context;
	}

	void LifecycleManagerImplementation::onRuntimeManagerEvent(JsonObject& data)
	{
            dispatchEvent(LifecycleManagerImplementation::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME, data);
	}

        void LifecycleManagerImplementation::onWindowManagerEvent(JsonObject& data)
	{
            dispatchEvent(LifecycleManagerImplementation::EventNames::LIFECYCLE_MANAGER_EVENT_WINDOW, data);
	}

        void LifecycleManagerImplementation::onRippleEvent(string name, JsonObject& data)
	{
            //mLifecycleManager->dispatchEvent(LifecycleManagerImplementation::EventNames::LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED, JsonValue(minutes));
	}

        void LifecycleManagerImplementation::onStateChangeEvent(JsonObject& data)
	{
            dispatchEvent(LifecycleManagerImplementation::EventNames::LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED, data);
	}

        void LifecycleManagerImplementation::handleRuntimeManagerEvent(const JsonObject &data)
        {
            string eventName = data["name"];
            if (eventName.compare("onTerminated") == 0)
            {
                LOGINFO("Received onterminated event from runtime manager");
                string appInstanceId = data["appInstanceId"];
                auto context = getContext(appInstanceId, "");
                if (nullptr == context)
                {
                    LOGERR("Received termination event for app which is not available");
                }
                else
                {
                    LOGINFO("Received state change event for app Onterminated event ");
                    //check if unexpected termination
                    if (context->getCurrentLifecycleState() != Exchange::ILifecycleManager::LifecycleState::TERMINATING)
                    {
                        LOGWARN("Container has terminated unexpectedly for app[%s] ", appInstanceId.c_str());
                        LOGWARN("Unloading app [%s] - [%s]", context->getAppId().c_str(), appInstanceId.c_str());
                        std::string terminateError="";
                        std::string updateError="";
                        bool terminated = false;
                        context->setUnexpectedTermination(true);
                        context->setRequestType(REQUEST_TYPE_TERMINATE);
                        context->setTargetLifecycleState(Exchange::ILifecycleManager::LifecycleState::TERMINATING);
                        context->setApplicationKillParams(false);
                        context->resetPendingStates();
                        context->setTerminated(true);

                        terminated = RequestHandler::getInstance()->terminate(context.get(), false, terminateError);
                        if(terminated)
                        {
                            LOGINFO("Successfully handled unexpected container termination for app[%s] ", appInstanceId.c_str());
                            LOGINFO("Successfully triggered unload for app[%s]", appInstanceId.c_str());
                        }
                        else
                        {
                            LOGERR("Failed to handle unexpected termination for app[%s] terminateSuccess[%d] terminateError[%s] updateError[%s]", appInstanceId.c_str(), terminated, terminateError.c_str(), updateError.c_str());
                        }
                    }
                    else
                    {
                       addStateTransitionRequest(context.get(), "onAppTerminating");
                    }
                }
            }
            else if (eventName.compare("onStateChanged") == 0)
            {
                string appInstanceId = data["appInstanceId"];
                uint32_t runtimeState = data["state"].Number();
                if (Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_RUNNING == runtimeState)
                {
                    auto context = getContext(appInstanceId, "");
                    if (nullptr == context)
                    {
                        LOGERR("Received state change event for app which is not available");
                    }
                    else
                    {
                        LOGINFO("Received state change event for app which is available ie running");
                        addStateTransitionRequest(context.get(), "onAppRunning");
                    }
                }
                // state change to TERMINATING is ignored as it is handled in onTerminated
            }
            else if (eventName.compare("onFailure") == 0)
            {
                string appInstanceId = data["appInstanceId"];
                string errorCode = data["errorCode"];
                LOGERR("Received container failure event from runtime manager for app[%s] error[%s]", appInstanceId.c_str(), errorCode.c_str());
                notifyOnFailure(appInstanceId, errorCode);
            }
            else if (eventName.compare("onStarted") == 0)
            {
                string appInstanceId = data["appInstanceId"];
                LOGINFO("Received container started event from runtime manager for app[%s]", appInstanceId.c_str());
            }
        }

        void LifecycleManagerImplementation::notifyOnFailure(const string& appInstanceId, const string& errorCode)
        {
            string appId = "";
            JsonObject eventData;
            auto context = getContext(appInstanceId, "");
            if (nullptr != context)
            {
                appId = context->getAppId();
            }
            eventData["appId"] = appId;
            eventData["appInstanceId"] = appInstanceId;
            eventData["newLifecycleState"] = static_cast<uint32_t>(Exchange::ILifecycleManager::LifecycleState::UNLOADED);
            eventData["errorReason"] = errorCode;
            dispatchEvent(LifecycleManagerImplementation::EventNames::LIFECYCLE_MANAGER_EVENT_ONFAILURE, eventData);
            LOGINFO("Notify error event for appId[%s] appInstanceId[%s] error[%s]", appId.c_str(), appInstanceId.c_str(), errorCode.c_str());
        }

    void LifecycleManagerImplementation::handleStateChangeEvent(const JsonObject &data)
    {
            string appInstanceId = data["appInstanceId"];
	    uint32_t stateInput = data["newLifecycleState"].Number();
            Exchange::ILifecycleManager::LifecycleState state = (Exchange::ILifecycleManager::LifecycleState) stateInput;
            if (Exchange::ILifecycleManager::LifecycleState::UNLOADED != state)
	    {
	        return;
	    }
            Core::SafeSyncType<Core::CriticalSection> adminLock(mAdminLock);
            auto iter = mLoadedApplications.end();
	    for (iter = mLoadedApplications.begin(); iter != mLoadedApplications.end(); iter++)
	    {
                if (nullptr == *iter)
		{
                    continue;
		}
                if ((*iter)->getAppInstanceId() == appInstanceId)
	        {
	            break;	    
                }
	    }
	    if (iter != mLoadedApplications.end())
	    {
                mLoadedApplications.erase(iter);
	    }
        }

        bool LifecycleManagerImplementation::tryGetPendingRespawn(const string& appInstanceId, PendingRespawnRequest& pendingRespawn)
        {
            Core::SafeSyncType<Core::CriticalSection> adminLock(mAdminLock);
            auto pendingRespawnIter = mPendingRespawns.find(appInstanceId);
            if (pendingRespawnIter == mPendingRespawns.end())
            {
                return false;
            }

            pendingRespawn = pendingRespawnIter->second;
            mPendingRespawns.erase(pendingRespawnIter);
            return true;
        }

        void LifecycleManagerImplementation::handlePendingRespawn(const PendingRespawnRequest& pendingRespawn)
        {
            string appInstanceId(""), errorReason("");
            bool success = false;
            const ApplicationLaunchParams& launchParams = pendingRespawn.mLaunchParams;

            LOGINFO("Respawning app [%s] after unload confirmation", launchParams.mAppId.c_str());
            Core::hresult status = SpawnApp(launchParams.mAppId,
                                            launchParams.mLaunchIntent,
                                            launchParams.mTargetState,
                                            launchParams.mRuntimeConfigObject,
                                            launchParams.mLaunchArgs,
                                            appInstanceId,
                                            errorReason,
                                            success);
            if ((Core::ERROR_NONE != status) || (false == success))
            {
                LOGERR("Failed to respawn app [%s] after unload confirmation. status[%d] success[%d] error[%s]", launchParams.mAppId.c_str(), status, success, errorReason.c_str());
            }
        }


    void LifecycleManagerImplementation::handleWindowManagerEvent(const JsonObject &data)
    {
        string eventName = data["name"];
        if (eventName.compare("onUserInactivity") == 0)
        {
            LOGINFO("Received onUserInactivity event from window manager");
        }
        else if (eventName.compare("onDisconnect") == 0)
        {
            LOGINFO("Received onDisconnect event from window manager");
	}
        else if (eventName.compare("onReady") == 0)
        {
            LOGINFO("Received onReady event from window manager");
            std::string appInstanceId = data["appInstanceId"];
            auto context = getContext(appInstanceId, "");
            if (nullptr != context)
	    {
                addStateTransitionRequest(context.get(), "onFirstFrame");
	    }
	}
    }

    void LifecycleManagerImplementation::addStateTransitionRequest(ApplicationContext* context, std::string event)
    {
        if (context->mPendingStateTransition && (0 == context->mPendingEventName.compare(event)))
        {
            std::string errorReason("");
            bool success = RequestHandler::getInstance()->updateState(context, context->getTargetLifecycleState(), errorReason);
            LOGINFO("Added state transition request success[%d] event[%s]", success, event.c_str());
        }
        else
        {
            LOGWARN("Received wrong state transition request: appInstanceId=%s expected=%s got=%s pendingTransition=%d", context->getAppInstanceId().c_str(), context->mPendingEventName.c_str(), event.c_str(), context->mPendingStateTransition);
        }
    }

    } /* namespace Plugin */
} /* namespace WPEFramework */


