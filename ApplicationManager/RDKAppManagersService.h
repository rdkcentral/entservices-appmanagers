#pragma once

#include "Module.h"
#include "AppsStatus.h"

#include <interfaces/IAppManager.h>
#include <interfaces/IStorageManager.h>
#include <interfaces/IRDKWindowManager.h>
#include <plugins/plugins.h>

#include <functional>
#include <json/json.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace WPEFramework {
namespace Plugin {

/**
 * @brief IEventHandler - Interface for receiving WebSocket notifications
 */
class IEventHandler {
public:
	virtual ~IEventHandler() = default;

	/**
	 * @brief Called when WebSocket notification should be sent
	 * @param url WebSocket endpoint URL (e.g., "/as/apps/status")
	 * @param message JSON message to send
	 */
	virtual void NotifyWebSocketUpdate(const std::string& url, const std::string& message) = 0;
};

class RDKAppManagersService {
public:
	explicit RDKAppManagersService(PluginHost::IShell* shell = nullptr);
	~RDKAppManagersService();

	void SetShell(PluginHost::IShell* shell);

	// Event handling and status tracking
	void SetEventHandler(IEventHandler* handler);
	void StartStatusListener();
	void StopStatusListener();
	bool StartAppManagerListener(PluginHost::IShell* shell);
	void StopAppManagerListener();
	void ProcessThunderEvent(const std::string& eventName, const std::string& eventData);
	std::string GetAppsStatus() const;
	bool IsListenerActive() const;

	// Request handlers
	Core::hresult GetInstalledAppsRequest(uint32_t& code, std::string& responseBody);
	Core::hresult LaunchAppRequest(const std::string& appId, const std::string& token, const std::string& mode,
								   const std::string& intent, const std::string& launchArgs,
								   uint32_t& code, std::string& responseBody);
	Core::hresult CloseAppRequest(const std::string& appId, uint32_t& code, std::string& responseBody);
	Core::hresult KillAppRequest(const std::string& appId, uint32_t& code, std::string& responseBody);
	Core::hresult FocusAppRequest(const std::string& client, uint32_t& code, std::string& responseBody);
	Core::hresult GetSystemStatsRequest(uint32_t& code, std::string& responseBody);
	Core::hresult ResetAppDataRequest(const std::string& appId, uint32_t& code, std::string& responseBody);
	Core::hresult DispatchMappedRequest(const std::vector<std::string>& methods,
		const std::map<std::string, std::string>& paramTemplate,
		const std::map<std::string, std::string>& runtimeParams,
		uint32_t& code, std::string& responseBody);

private:
	class AppManagerNotification : public Exchange::IAppManager::INotification {
	public:
		explicit AppManagerNotification(RDKAppManagersService& parent)
			: _parent(parent)
		{
		}

		AppManagerNotification(const AppManagerNotification&) = delete;
		AppManagerNotification& operator=(const AppManagerNotification&) = delete;

		void OnAppInstalled(const string& appId, const string& version) override;
		void OnAppUninstalled(const string& appId) override;
		void OnAppLifecycleStateChanged(const string& appId, const string& appInstanceId,
										const Exchange::IAppManager::AppLifecycleState newState,
										const Exchange::IAppManager::AppLifecycleState oldState,
										const Exchange::IAppManager::AppErrorReason errorReason) override;
		void OnAppLaunchRequest(const string& appId, const string& intent, const string& source) override;
		void OnAppUnloaded(const string& appId, const string& appInstanceId) override;

		BEGIN_INTERFACE_MAP(AppManagerNotification)
			INTERFACE_ENTRY(Exchange::IAppManager::INotification)
		END_INTERFACE_MAP

	private:
		RDKAppManagersService& _parent;
	};

	// Internal notification helper
	void NotifyWebSocket(const std::string& url, const std::string& message);

	// WebSocket event listener setup
	void StartEventListener();
	void StopEventListener();
	void OnEventMessageReceived(const std::string& message);
	void EventListenerThread();

	// AppManager notification helpers
	void SendAppManagerEvent(const std::string& eventName, const Json::Value& params);
	static std::string LifecycleStateToString(Exchange::IAppManager::AppLifecycleState state);
	static std::string ErrorReasonToString(Exchange::IAppManager::AppErrorReason reason);

private:
	PluginHost::IShell* m_shell;

	IEventHandler* m_eventHandler;
	std::unique_ptr<AppsStatus> m_appsStatus;
	mutable std::mutex m_statusMutex;

	// WebSocket event listener
	void* m_wsConnection;  // ThunderWsConnection pointer (opaque)
	std::thread m_listenerThread;
	bool m_listenerRunning;
	std::mutex m_listenerMutex;

	// AppManager RPC listener
	Exchange::IAppManager* m_appManager;
	Exchange::IRDKWindowManager* m_windowManager;
	Core::Sink<AppManagerNotification> m_appManagerNotification;
};

} // namespace Plugin
} // namespace WPEFramework


