#pragma once

#include <functional>
#include <json/json.h>
#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <semaphore.h>

namespace WPEFramework {
namespace Exchange {
	struct IAppManager;  // Forward declaration
}
namespace Plugin {

// Callback for WebSocket notifications
using WebSocketNotifyCallback = std::function<void(const std::string& url, const std::string& message)>;

// Callback for uninstall event notifications
using UninstallEventCallback = std::function<void()>;

/**
 * @brief AppsStatus - Tracks app lifecycle status from org.rdk.AppManager events
 * 
 * Subscribes to Thunder AppManager events and maintains a status map of all apps.
 * Broadcasts status updates via WebSocket to /as/apps/status endpoint.
 * 
 * Events subscribed:
 * - onAppInstalled: New app installed
 * - onAppUninstalled: App removed
 * - onAppLifecycleStateChanged: App state change (ACTIVE, INACTIVE, etc.)
 * - onAppLaunchRequest: App launch requested (with source info)
 * - onAppUnloaded: App unloaded from memory
 */
class AppsStatus {
public:
	explicit AppsStatus(WebSocketNotifyCallback notifyCb);
	~AppsStatus();

	// Disable copy
	AppsStatus(const AppsStatus&) = delete;
	AppsStatus& operator=(const AppsStatus&) = delete;

	/**
	 * @brief Set IAppManager interface for querying app status
	 * @param appManager Pointer to IAppManager (can be nullptr)
	 */
	void SetAppManager(WPEFramework::Exchange::IAppManager* appManager);

	/**
	 * @brief Process Thunder event notification
	 * @param eventJson JSON-RPC notification from AppManager
	 */
	void HandleThunderEvent(const std::string& eventJson);

	/**
	 * @brief Get current apps status as JSON
	 * @return JSON string with apps array
	 */
	std::string GetCurrentStatus() const;

	/**
	 * @brief Set callback for uninstall event (for synchronous uninstall operations)
	 * @param callback Function to call when app is uninstalled
	 */
	void SetUninstallCallback(UninstallEventCallback callback);

	/**
	 * @brief Wait for uninstall event with timeout (for synchronous uninstall)
	 * @param packageId Package being uninstalled
	 * @param timeoutSec Timeout in seconds
	 * @return true if uninstall event received, false if timed out
	 */
	bool WaitForUninstall(const std::string& packageId, int timeoutSec);

private:
	void registerForEvents();
	void initializeAppsStatus();
	void processNotification(const Json::Value& notification, bool ignoreStatusCheck = false);
	void updateAppsStatusWebSocket();
	void notifyUninstallEvent();

private:
	WebSocketNotifyCallback m_notifyCb;
	UninstallEventCallback m_uninstallCb;
	WPEFramework::Exchange::IAppManager* m_appManager;  // COM-RPC interface for querying loaded apps
	std::map<std::string, Json::Value> m_status;
	std::unique_ptr<Json::CharReader> m_jsonReader;
	std::unique_ptr<Json::StreamWriter> m_jsonWriter;
	mutable std::mutex m_statusMutex;
    
	// For synchronous uninstall operations
	std::string m_pendingUninstallPackage;
	sem_t m_uninstallSem;
};

} // namespace Plugin
} // namespace WPEFramework

