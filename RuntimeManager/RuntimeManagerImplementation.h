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
#include <interfaces/IRuntimeManager.h>
#include "UtilsLogging.h"
#include "tracing/Logging.h"
#include <mutex>

namespace WPEFramework
{
    namespace Plugin
    {
        class RuntimeManagerImplementation : public Exchange::IRuntimeManager
	{
            public:
                enum EventNames
                {
                    RUNTIME_MANAGER_EVENT_STATECHANGED
                };
                class EXTERNAL Job : public Core::IDispatch
	        {
                    protected:
                         Job(RuntimeManagerImplementation *runtimeManagerImplementation, EventNames event, JsonValue &params)
                            : mRuntimeManagerImplementation(runtimeManagerImplementation)
                            , _event(event)
                            , _params(params)
	                {
                            if (mRuntimeManagerImplementation != nullptr)
	            	    {
                                mRuntimeManagerImplementation->AddRef();
                            }
                        }

                    public:
                         Job() = delete;
                         Job(const Job&) = delete;
                         Job& operator=(const Job&) = delete;
                         ~Job()
	                 {
                             if (mRuntimeManagerImplementation != nullptr)
	            	     {
                                 mRuntimeManagerImplementation->Release();
                             }
                         }

                    public:
                         static Core::ProxyType<Core::IDispatch> Create(RuntimeManagerImplementation *runtimeManagerImplementation, EventNames event, JsonValue params)
	                 {
#ifndef  USE_THUNDER_R4
                             return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(runtimeManagerImplementation, event, params)));
#else
                             return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(runtimeManagerImplementation, event, params)));
#endif
                         }

                         virtual void Dispatch()
	                 {
                             mRuntimeManagerImplementation->Dispatch(_event, _params);
                         }
                    private:
                        RuntimeManagerImplementation *mRuntimeManagerImplementation;
                        const EventNames _event;
                        const JsonValue _params;
                };

                RuntimeManagerImplementation ();
                ~RuntimeManagerImplementation () override;
                RuntimeManagerImplementation (const RuntimeManagerImplementation &) = delete;
                RuntimeManagerImplementation & operator=(const RuntimeManagerImplementation &) = delete;

		BEGIN_INTERFACE_MAP(RuntimeManagerImplementation)
		INTERFACE_ENTRY(Exchange::IRuntimeManager)
	        END_INTERFACE_MAP

                /* IRuntimeManager methods  */
                virtual Core::hresult Register(Exchange::IRuntimeManager::INotification *notification) override;
                virtual Core::hresult Unregister(Exchange::IRuntimeManager::INotification *notification) override;

                virtual Core::hresult Run(const string& appInstanceId, const string& appPath, const string& runtimePath, IStringIterator* const& envVars, const uint32_t userId, const uint32_t groupId, IValueIterator* const& ports, IStringIterator* const& paths, IStringIterator* const& debugSettings, bool& success);
                virtual Core::hresult Hibernate(const string& appInstanceId, bool& success) const override;
                virtual Core::hresult Wake(const string& appInstanceId, const string& state, bool& success) const override;
                virtual Core::hresult Suspend(const string& appInstanceId, bool& success) const override;
                virtual Core::hresult Resume(const string& appInstanceId, bool& success) const override;
                virtual Core::hresult Terminate(const string& appInstanceId, bool& success) const override;
                virtual Core::hresult Kill(const string& appInstanceId, bool& success) const override;
                virtual Core::hresult GetInfo(const string& appInstanceId, const string& info , bool& success) const override;
                virtual Core::hresult Annotate(const string& appInstanceId, const string& key , const string& value , bool& success) const override;
                virtual Core::hresult Mount() const override;
                virtual Core::hresult Unmount() const override;

	    private: /* members */
                mutable Core::CriticalSection mAdminLock;
	        std::list<Exchange::IRuntimeManager::INotification*> mRuntimeManagerNotification;;	

	    private: /* internal methods */
                void dispatchEvent(EventNames, const JsonValue &params);
                void Dispatch(EventNames event, const JsonValue params);

                friend class Job;
        };
    } /* namespace Plugin */
} /* namespace WPEFramework */
