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

#include "StateTransitionHandler.h"
#include "StateHandler.h"
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <exception>
#include "UtilsLogging.h"

namespace WPEFramework
{
    namespace Plugin
    {
        static std::thread requestHandlerThread;
        std::mutex gRequestMutex;
        sem_t gRequestSemaphore;
        std::vector<std::shared_ptr<StateTransitionRequest>> gRequests;
        static std::atomic<bool> sRunning{true};
        static std::atomic<bool> sInitialized{false};

        StateTransitionHandler* StateTransitionHandler::mInstance = nullptr;

        StateTransitionHandler* StateTransitionHandler::getInstance()
	{
            if (nullptr == mInstance)
            {
                mInstance = new StateTransitionHandler();
            }
            return mInstance;
	}

        StateTransitionHandler::StateTransitionHandler()
	{
	}

        StateTransitionHandler::~StateTransitionHandler()
	{
	}

        bool StateTransitionHandler::initialize()
	{
            {
                std::lock_guard<std::mutex> lock(gRequestMutex);
                if (true == sInitialized.load())
                {
                    return true;
                }

                sRunning.store(true);

                if (0 != sem_init(&gRequestSemaphore, 0, 0))
                {
                    LOGERR("sem_init failed in %s", __FUNCTION__);
                    return false;
                }

                sInitialized.store(true);
            }

            StateHandler::initialize();

            try
            {
            requestHandlerThread = std::thread([=]() {
                bool isRunning = true;
                {
                    std::lock_guard<std::mutex> lock(gRequestMutex);
                    isRunning = sRunning.load();
                }
                while(isRunning)
		{
                    while (true)
                    {
                        std::shared_ptr<StateTransitionRequest> request;
                        {
                            std::lock_guard<std::mutex> lock(gRequestMutex);
                            if (true == gRequests.empty())
                            {
                                break;
                            }
                            request = gRequests.front();
                            gRequests.erase(gRequests.begin());
                        }

                        if (nullptr == request)
                        {
                            continue;
                        }

                        std::string errorReason;
                        bool success = false;
                        try
                        {
                            success = StateHandler::changeState(*request, errorReason);
                        }
                        catch (const std::exception& ex)
                        {
                            errorReason = ex.what();
                        }
                        catch (...)
                        {
                            errorReason = "unknown exception";
                        }

                        if (false == success)
                        {
                            LOGERR("ERROR IN STATE TRANSITION ... %s \n",errorReason.c_str());
                            //TODO: Decide on what to do on state transition error
                            break;
                        }
                    }
                    sem_wait(&gRequestSemaphore);
                    {
                        std::lock_guard<std::mutex> lock(gRequestMutex);
                        isRunning = sRunning.load();
                    }
                }
            });
            }
            catch (const std::system_error& ex)
            {
                LOGERR("Failed to create requestHandlerThread in %s: %s", __FUNCTION__, ex.what());
                {
                    std::lock_guard<std::mutex> lock(gRequestMutex);
                    sInitialized.store(false);
                    gRequests.clear();
                }
                sem_destroy(&gRequestSemaphore);
                return false;
            }

	    return true;	
	}

	void StateTransitionHandler::terminate()
	{
            {
                std::lock_guard<std::mutex> lock(gRequestMutex);
                if (false == sInitialized.load())
                {
                    return;
                }

                sRunning.store(false);
                sInitialized.store(false);
            }

            sem_post(&gRequestSemaphore);

            if (true == requestHandlerThread.joinable())
            {
                requestHandlerThread.join();
            }
            else
            {
                LOGWARN("requestHandlerThread is not joinable in %s", __FUNCTION__);
            }

            if (0 != sem_destroy(&gRequestSemaphore))
            {
                LOGERR("sem_destroy failed in %s: %s", __FUNCTION__, strerror(errno));
            }

            {
                std::lock_guard<std::mutex> lock(gRequestMutex);
                gRequests.clear();
            }
	}

	void StateTransitionHandler::addRequest(StateTransitionRequest& request)
	{
           // Prevent sem_post on destroyed semaphore
           if (false == sInitialized.load())
           {
               LOGWARN("addRequest called while handler is not initialized in %s, dropping request", __FUNCTION__);
               return;
           }

           //TODO: Pass contect and state as argument to function
	   std::shared_ptr<StateTransitionRequest> stateTransitionRequest = std::make_shared<StateTransitionRequest>(request.mContext, request.mTargetState);
	   {
               std::lock_guard<std::mutex> lock(gRequestMutex);
               if (false == sInitialized.load())
               {
                   LOGWARN("addRequest called after handler shutdown started in %s, dropping request", __FUNCTION__);
                   return;
               }

               gRequests.push_back(std::move(stateTransitionRequest));
               sem_post(&gRequestSemaphore);
	   }
	}

    } /* namespace Plugin */
} /* namespace WPEFramework */
