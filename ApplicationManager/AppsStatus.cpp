#include "AppsStatus.h"
// ThunderConnection removed - using IAppManager COM-RPC interface
#include <interfaces/IAppManager.h>
#include "Module.h"

#include <sstream>
#include <time.h>

// App state constants
namespace {
const char* APP_STATE_UNKNOWN = "APP_STATE_UNKNOWN";
const char* APP_STATE_ACTIVE = "APP_STATE_ACTIVE";
} // anonymous namespace

namespace WPEFramework {
namespace Plugin {

AppsStatus::AppsStatus(WebSocketNotifyCallback notifyCb)
	: m_notifyCb(std::move(notifyCb))
	, m_uninstallCb(nullptr)
	, m_appManager(nullptr)
{
	SYSLOG(Logging::Startup, (string(_T("AppsStatus Constructor"))));

	// Initialize semaphore for synchronous uninstall
	sem_init(&m_uninstallSem, 0, 0);

	// Initialize JSON reader/writer
	Json::StreamWriterBuilder writeBuilder;
	writeBuilder["commentStyle"] = "None";
	writeBuilder["indentation"] = "  ";
	m_jsonWriter = std::unique_ptr<Json::StreamWriter>(writeBuilder.newStreamWriter());

	Json::CharReaderBuilder readBuilder;
	readBuilder["collectComments"] = false;
	m_jsonReader = std::unique_ptr<Json::CharReader>(readBuilder.newCharReader());

	// Register for Thunder AppManager events
	registerForEvents();

	// Initialize with current app status
	initializeAppsStatus();
}

AppsStatus::~AppsStatus()
{
	SYSLOG(Logging::Shutdown, (string(_T("AppsStatus Destructor"))));
	sem_destroy(&m_uninstallSem);
}

void AppsStatus::SetAppManager(Exchange::IAppManager* appManager)
{
	SYSLOG(Logging::Startup, (_T("AppsStatus::SetAppManager - %p"), appManager));
	m_appManager = appManager;
    
	// Initialize apps status when AppManager is set
	if (m_appManager) {
		initializeAppsStatus();
	}
}

void AppsStatus::SetUninstallCallback(UninstallEventCallback callback)
{
	m_uninstallCb = std::move(callback);
}

bool AppsStatus::WaitForUninstall(const std::string& packageId, int timeoutSec)
{
	m_pendingUninstallPackage = packageId;
    
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeoutSec;
    
	int result = sem_timedwait(&m_uninstallSem, &ts);
	m_pendingUninstallPackage.clear();
    
	return (result == 0);
}

void AppsStatus::notifyUninstallEvent()
{
	sem_post(&m_uninstallSem);
	if (m_uninstallCb) {
		m_uninstallCb();
	}
}

void AppsStatus::registerForEvents()
{
	SYSLOG(Logging::Startup, (string(_T("AppsStatus::registerForEvents - Events now registered via COM-RPC in RDKAppManagersService"))));
    
	// Event registration is now handled by RDKAppManagersService::StartAppManagerListener()
	// which registers AppManagerNotification callbacks directly via IAppManager COM-RPC interface.
	// This method is kept for compatibility but does nothing.
}

void AppsStatus::initializeAppsStatus()
{
	SYSLOG(Logging::Startup, (string(_T("AppsStatus::initializeAppsStatus - Getting loaded apps"))));

	if (!m_appManager) {
		SYSLOG(Logging::Startup, (string(_T("AppsStatus::initializeAppsStatus - No IAppManager interface, starting empty"))));
		return;
	}

	// Call IAppManager::GetLoadedApps via COM-RPC
	Exchange::IAppManager::ILoadedAppInfoIterator* appsIter = nullptr;
	Core::hresult result = m_appManager->GetLoadedApps(appsIter);
    
	if (result != Core::ERROR_NONE || !appsIter) {
		SYSLOG(Logging::Startup, (_T("AppsStatus::initializeAppsStatus - getLoadedApps failed, starting empty")));
		return;
	}

	std::lock_guard<std::mutex> lock(m_statusMutex);
    
	// Iterate through loaded apps
	Exchange::IAppManager::LoadedAppInfo appInfo;
	while (appsIter->Next(appInfo)) {
		std::string appId = appInfo.appId;
		std::string lifecycleState = "";
        
		// Convert lifecycle state enum to string
		switch (appInfo.lifecycleState) {
			case Exchange::IAppManager::APP_STATE_ACTIVE:
				lifecycleState = "APP_STATE_ACTIVE";
				break;
			case Exchange::IAppManager::APP_STATE_RUNNING:
			case Exchange::IAppManager::APP_STATE_PAUSED:
				lifecycleState = "APP_STATE_RUNNING";
				break;
			default:
				lifecycleState = "APP_STATE_UNLOADED";
				break;
		}
        
		Json::Value newEntry(Json::objectValue);
		newEntry["appId"] = appId;
		newEntry["launchMethod"] = "unknown";
		newEntry["fireboltStatus"] = "FOREGROUND";
		newEntry["status"] = (appInfo.lifecycleState == Exchange::IAppManager::APP_STATE_ACTIVE) ? "VISIBLE" : "RUNNING";
        
		m_status[appId] = newEntry;
		SYSLOG(Logging::Startup, (_T("AppsStatus::initializeAppsStatus - Initialized app: %s state=%s"), 
			   appId.c_str(), lifecycleState.c_str()));
	}
    
	appsIter->Release();
    
	// Update WebSocket with initial status
	updateAppsStatusWebSocket();
}

void AppsStatus::HandleThunderEvent(const std::string& eventJson)
{
	SYSLOG(Logging::Startup, (_T("AppsStatus::HandleThunderEvent - %s"), eventJson.c_str()));

	Json::Value notification;
	std::string errors;

	if (m_jsonReader->parse(eventJson.c_str(), eventJson.c_str() + eventJson.length(), &notification, &errors)) {
		if (notification.isMember("method")) {
			processNotification(notification);
		}
	} else {
		SYSLOG(Logging::Startup, (_T("AppsStatus::HandleThunderEvent - Parse error: %s"), errors.c_str()));
	}
}

void AppsStatus::processNotification(const Json::Value& notification, bool ignoreStatusCheck)
{
	std::string method = notification["method"].asString();
	Json::Value params = notification["params"];
	std::string appId = params["appId"].asString();

	SYSLOG(Logging::Startup, (_T("AppsStatus::processNotification - method=%s appId=%s ignoreStatusCheck=%d"), 
		   method.c_str(), appId.c_str(), ignoreStatusCheck));

	std::string source;
	std::string state;
	std::string previousState;
	std::string appInstanceId = params.isMember("appInstanceId") ? params["appInstanceId"].asString() : "";

	// Handle specific events
	if (method.find("onAppLaunchRequest") != std::string::npos) {
		// Extract source from intent
		std::string intentStr = params["intent"].asString();
		Json::Value intent;
		std::string errors;
		if (m_jsonReader->parse(intentStr.c_str(), intentStr.c_str() + intentStr.length(), &intent, &errors)) {
			source = intent["context"]["source"].asString();
		}
		SYSLOG(Logging::Startup, (_T("AppsStatus - onAppLaunchRequest: appId=%s source=%s"), 
			   appId.c_str(), source.c_str()));
	} 
	else if (method.find("onAppLifecycleStateChanged") != std::string::npos) {
		state = params["newState"].asString();
		previousState = params["previousState"].asString();
		SYSLOG(Logging::Startup, (_T("AppsStatus - onAppLifecycleStateChanged: appId=%s state=%s prev=%s"), 
			   appId.c_str(), state.c_str(), previousState.c_str()));
        
		// Check ignoreStatusCheck conditions (as per reference microservice)
		// Ignore if state is APP_STATE_UNKNOWN
		if (state == APP_STATE_UNKNOWN) {
			ignoreStatusCheck = true;
			SYSLOG(Logging::Startup, (_T("AppsStatus - Ignoring APP_STATE_UNKNOWN")));
		}
	}
	else if (method.find("onAppInstalled") != std::string::npos) {
		// New app installed - ignore status check, just add to list
		ignoreStatusCheck = true;
		SYSLOG(Logging::Startup, (_T("AppsStatus - onAppInstalled: appId=%s"), appId.c_str()));
	}
	else if (method.find("onAppUninstalled") != std::string::npos) {
		// App uninstalled - ignore status check, remove from list
		ignoreStatusCheck = true;
		SYSLOG(Logging::Startup, (_T("AppsStatus - onAppUninstalled: appId=%s"), appId.c_str()));
        
		// Notify uninstall waiters
		if (!m_pendingUninstallPackage.empty()) {
			notifyUninstallEvent();
		}
	}
	else if (method.find("onAppUnloaded") != std::string::npos) {
		ignoreStatusCheck = true;
		SYSLOG(Logging::Startup, (_T("AppsStatus - onAppUnloaded: appId=%s"), appId.c_str()));
	}

	// Lock for status map access
	std::lock_guard<std::mutex> lock(m_statusMutex);

	// Handle app removal events
	if (method.find("onAppUnloaded") != std::string::npos ||
		method.find("onAppUninstalled") != std::string::npos) {
		if (m_status.erase(appId)) {
			SYSLOG(Logging::Startup, (_T("AppsStatus - Removed app: %s"), appId.c_str()));
		}
	}
	else if (!ignoreStatusCheck) {
		// Add or update app in status map
		auto it = m_status.find(appId);
		if (it == m_status.end()) {
			Json::Value newEntry(Json::objectValue);
			newEntry["appId"] = appId;
			newEntry["launchMethod"] = source.empty() ? "unknown" : source;
			newEntry["fireboltStatus"] = "FOREGROUND";
			m_status[appId] = newEntry;
			it = m_status.find(appId);
		}

		Json::Value& appEntry = it->second;
		appEntry["appId"] = appId;
		if (!appInstanceId.empty()) {
			appEntry["activeSessionId"] = appInstanceId;
			appEntry["runningSessionId"] = appInstanceId;
		}

		if (!source.empty()) {
			appEntry["launchMethod"] = source;
		}

		if (!state.empty()) {
			// Map Thunder state to status
			if (state == APP_STATE_ACTIVE) {
				appEntry["status"] = "VISIBLE";
				appEntry["fireboltStatus"] = "FOREGROUND";
			} else {
				appEntry["status"] = "RUNNING";
				appEntry["fireboltStatus"] = "FOREGROUND";
			}
		}

		SYSLOG(Logging::Startup, (_T("AppsStatus - Updated app: %s"), appId.c_str()));
	}

	// Broadcast updated status (even for install/uninstall/unload events)
	updateAppsStatusWebSocket();
}

void AppsStatus::updateAppsStatusWebSocket()
{
	static const std::string wsUrl("/as/apps/status");

	// Note: Assumes caller holds m_statusMutex if needed
	std::ostringstream ss;
	ss << "{ \"apps\": [\n";

	bool first = true;
	for (const auto& appStatus : m_status) {
		if (first)
			first = false;
		else
			ss << ",\n";

		m_jsonWriter->write(appStatus.second, &ss);
	}
	ss << "\n] }";

	std::string payload = ss.str();
	SYSLOG(Logging::Startup, (_T("AppsStatus::updateAppsStatusWebSocket - Broadcasting to %d clients"), 
		   static_cast<int>(m_status.size())));

	if (m_notifyCb) {
		m_notifyCb(wsUrl, payload);
	}
}

std::string AppsStatus::GetCurrentStatus() const
{
	std::lock_guard<std::mutex> lock(m_statusMutex);

	std::ostringstream ss;
	ss << "{ \"apps\": [\n";

	bool first = true;
	for (const auto& appStatus : m_status) {
		if (first)
			first = false;
		else
			ss << ",\n";

		m_jsonWriter->write(appStatus.second, &ss);
	}
	ss << "\n] }";

	return ss.str();
}

} // namespace Plugin
} // namespace WPEFramework

