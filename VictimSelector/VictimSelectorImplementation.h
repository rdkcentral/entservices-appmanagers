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

#pragma once

#include "Module.h"
#include <interfaces/IVictimSelector.h>
#include <interfaces/IAppManager.h>
#include <interfaces/IRuntimeManager.h>
#include <interfaces/IConfiguration.h>
#include "UtilsLogging.h"
#include <mutex>
#include <string>

namespace WPEFramework {
namespace Plugin {

class VictimSelectorImplementation : public Exchange::IVictimSelector,
                                     public Exchange::IConfiguration {
private:
    class AppManagerNotification : public Exchange::IAppManager::INotification {
    public:
        explicit AppManagerNotification(VictimSelectorImplementation& parent)
            : mParent(parent) {}

        BEGIN_INTERFACE_MAP(AppManagerNotification)
        INTERFACE_ENTRY(Exchange::IAppManager::INotification)
        END_INTERFACE_MAP

        void OnAppInstalled(const string&, const string&) override {}
        void OnAppUninstalled(const string&) override {}
        void OnAppLaunchRequest(const string&, const string&, const string&) override {}
        void OnAppUnloaded(const string&, const string&) override {}
        void OnAppLifecycleStateChanged(const string& appId,
                                        const string& appInstanceId,
                                        const Exchange::IAppManager::AppLifecycleState newState,
                                        const Exchange::IAppManager::AppLifecycleState oldState,
                                        const Exchange::IAppManager::AppErrorReason errorReason) override;

    private:
        VictimSelectorImplementation& mParent;
    };

public:
    VictimSelectorImplementation();
    ~VictimSelectorImplementation() override;
    VictimSelectorImplementation(const VictimSelectorImplementation&) = delete;
    VictimSelectorImplementation& operator=(const VictimSelectorImplementation&) = delete;

    BEGIN_INTERFACE_MAP(VictimSelectorImplementation)
    INTERFACE_ENTRY(Exchange::IVictimSelector)
    INTERFACE_ENTRY(Exchange::IConfiguration)
    END_INTERFACE_MAP

    Core::hresult Register(Exchange::IVictimSelector::INotification* notification) override;
    Core::hresult Unregister(Exchange::IVictimSelector::INotification* notification) override;
    Core::hresult Evict(const EvictionReason reason, const EvictionType type) override;
    Core::hresult Configure(PluginHost::IShell* service) override;

private:
    Core::hresult selectVictim(std::string& appId, bool& isHibernated);
    uint32_t getAppPriority(const std::string& appId) const;
    void onAppLifecycleStateChanged(const string& appId,
                                    const string& appInstanceId,
                                    Exchange::IAppManager::AppLifecycleState newState,
                                    Exchange::IAppManager::AppLifecycleState oldState,
                                    Exchange::IAppManager::AppErrorReason errorReason);
    void complete(bool evicted, EvictErrorReason errorCode);
    void releaseAppManager();

    PluginHost::IShell* mService;
    Exchange::IAppManager* mAppManager;
    Exchange::IRuntimeManager* mRuntimeManager;
    Core::Sink<AppManagerNotification> mAppManagerNotification;
    bool mAppManagerRegistered;
    Exchange::IVictimSelector::INotification* mNotification;
    std::string mPendingAppId;
    EvictionType mPendingEvictionType;
    bool mEvictionInProgress;
    std::mutex mLock;
    std::mutex mEvictLock;
};

} // namespace Plugin
} // namespace WPEFramework
