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

#include "NativeJSImplementation.h"

using namespace std;

namespace WPEFramework
{
    namespace Plugin
    {

        std::string gPendingUrlRequest("");
        std::string gPendingUrlOptionsRequest("");
        SERVICE_REGISTRATION(NativeJSImplementation, 1, 0);

        NativeJSImplementation::NativeJSImplementation()
            : mRunning(true)
        {
            TRACE(Trace::Information, (_T("Constructing NativeJSImplementation Service: %p"), this));
        }

        NativeJSImplementation::~NativeJSImplementation()
        {
            TRACE(Trace::Information, (_T("Destructing NativeJSImplementation Service: %p"), this));
            mNativeJSRenderer = nullptr;
        }

        Core::hresult NativeJSImplementation::Initialize(string waylandDisplay)
        {   
            std::cout << "initialize called on nativejs implementation " << std::endl;
            mRenderThread = std::thread([=](std::string waylandDisplay) {
                mNativeJSRenderer = std::make_shared<NativeJSRenderer>(waylandDisplay);
                if (!gPendingUrlRequest.empty())
                {
		    ModuleSettings moduleSettings;
                    moduleSettings.fromString(gPendingUrlOptionsRequest);
                    mNativeJSRenderer->launchApplication(gPendingUrlRequest, moduleSettings);
		    gPendingUrlRequest = "";
                    gPendingUrlOptionsRequest = "";
                }
		
		mNativeJSRenderer->run();
		
		printf("After launch application execution ... \n"); fflush(stdout);
		mNativeJSRenderer.reset();

            }, waylandDisplay);
            return (Core::ERROR_NONE);
        }

        Core::hresult NativeJSImplementation::Deinitialize()
        {
           LOGINFO("deinitializing NativeJS process");
           if (mNativeJSRenderer)
           {
               mNativeJSRenderer->terminate();
               if (mRenderThread.joinable())
               {
                   mRenderThread.join();
               }
           }
	   return (Core::ERROR_NONE);
        }

        Core::hresult NativeJSImplementation::LaunchApplication(const std::string url, const std::string options)
        {
            LOGINFO("LaunchApplication invoked");
            if (mNativeJSRenderer)
            {		    
		std::string optionsVal(options);
		ModuleSettings moduleSettings;
                moduleSettings.fromString(optionsVal);
                mNativeJSRenderer->launchApplication(url, moduleSettings);
            }
	    else
            {
                gPendingUrlRequest = url;
                gPendingUrlOptionsRequest = options;
            }

	    return (Core::ERROR_NONE);	    
        }
        
	Core::hresult NativeJSImplementation::DestroyApplication(const std::string url)
        {
            LOGINFO("DestroyApplication invoked");
            if (mNativeJSRenderer)
            {		    
                mNativeJSRenderer->terminateApplication(url);
            }
            return (Core::ERROR_NONE);
        }
} // namespace Plugin
} // namespace WPEFramework
