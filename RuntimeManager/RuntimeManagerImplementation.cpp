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

#include "RuntimeManagerImplementation.h"

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(RuntimeManagerImplementation, 1, 0);

        RuntimeManagerImplementation::RuntimeManagerImplementation()
        {
            LOGINFO("Create RuntimeManagerImplementation Instance");
        }

        RuntimeManagerImplementation::~RuntimeManagerImplementation()
        {
            LOGINFO("Delete RuntimeManagerImplementation Instance");
        }

        Core::hresult RuntimeManagerImplementation::Register(Exchange::IRuntimeManager::INotification *notification)
        {
            ASSERT (nullptr != notification);
        
            mAdminLock.Lock();
        
            /* Make sure we can't register the same notification callback multiple times */
            if (std::find(mRuntimeManagerNotification.begin(), mRuntimeManagerNotification.end(), notification) == mRuntimeManagerNotification.end())
            {
                LOGINFO("Register notification");
                mRuntimeManagerNotification.push_back(notification);
                notification->AddRef();
            }
        
            mAdminLock.Unlock();
        
            return Core::ERROR_NONE;
        }
        
        Core::hresult RuntimeManagerImplementation::Unregister(Exchange::IRuntimeManager::INotification *notification )
        {
            Core::hresult status = Core::ERROR_GENERAL;
        
            ASSERT (nullptr != notification);
        
            mAdminLock.Lock();
        
            /* Make sure we can't unregister the same notification callback multiple times */
            auto itr = std::find(mRuntimeManagerNotification.begin(), mRuntimeManagerNotification.end(), notification);
            if (itr != mRuntimeManagerNotification.end())
            {
                (*itr)->Release();
                LOGINFO("Unregister notification");
                mRuntimeManagerNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("notification not found");
            }
        
            mAdminLock.Unlock();
        
            return status;
        }
        
        //mRuntimeManager->dispatchEvent(RuntimeManagerImplementation::EventNames::LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED, JsonValue(minutes));
        
        void RuntimeManagerImplementation::dispatchEvent(EventNames event, const JsonValue &params)
        {
            Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
        }
        
        void RuntimeManagerImplementation::Dispatch(EventNames event, const JsonValue params)
        {
             mAdminLock.Lock();
        
             std::list<Exchange::IRuntimeManager::INotification*>::const_iterator index(mRuntimeManagerNotification.begin());
        
             switch(event)
             {
                 case RUNTIME_MANAGER_EVENT_STATECHANGED:
                     while (index != mRuntimeManagerNotification.end())
                     {
                         // const string& appId, const string& state
        		 string appId("");
        		 string state("Starting");
                         (*index)->OnStateChanged(appId, state);
                         ++index;
                     }
                 break;
        
                 default:
                     LOGWARN("Event[%u] not handled", event);
                     break;
             }
        
             mAdminLock.Unlock();
        }
        

        Core::hresult RuntimeManagerImplementation::Run(const string& appInstanceId, const string& appPath, const string& runtimePath, IStringIterator* const& envVars, const uint32_t userId, const uint32_t groupId, IValueIterator* const& ports, IStringIterator* const& paths, IStringIterator* const& debugSettings, bool& success)
        {
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }
        
        Core::hresult RuntimeManagerImplementation::Hibernate(const string& appInstanceId, bool& success) const
        {
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }
        
        Core::hresult RuntimeManagerImplementation::Wake(const string& appInstanceId, const string& state, bool& success) const
        {
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }
        
        Core::hresult RuntimeManagerImplementation::Suspend(const string& appInstanceId , bool& success) const
        {
            printf("Madana Gopal suspend \n");
	    fflush(stdout);
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }
        
        Core::hresult RuntimeManagerImplementation::Resume(const string& appInstanceId, bool& success) const
        {
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }
        
        Core::hresult RuntimeManagerImplementation::Terminate(const string& appInstanceId, bool& success) const
        {
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }
        
        Core::hresult RuntimeManagerImplementation::Kill(const string& appInstanceId, bool& success) const
        {
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }

        Core::hresult RuntimeManagerImplementation::GetInfo(const string& appInstanceId, const string& info, bool& success) const
        {
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }

        Core::hresult RuntimeManagerImplementation::Annotate(const string& appInstanceId, const string& key, const string& value , bool& success) const
        {
            Core::hresult status = Core::ERROR_NONE;
            success = true;
            return status;
        }

        Core::hresult RuntimeManagerImplementation::Mount() const
        {
            Core::hresult status = Core::ERROR_NONE;
            return status;
        }

        Core::hresult RuntimeManagerImplementation::Unmount() const
        {
            Core::hresult status = Core::ERROR_NONE;
            return status;
        }
    } /* namespace Plugin */
} /* namespace WPEFramework */
