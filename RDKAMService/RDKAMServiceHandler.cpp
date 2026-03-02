#include "RDKAMServiceHandler.h"
#include "RDKAMServiceUtils.h"
#include "Module.h"

#include <chrono>
#include <ctime>
#include <json/json.h>
#include <memory>
#include <thread>

namespace WPEFramework {
namespace Plugin {

RDKAMServiceHandler::RDKAMServiceHandler(PluginHost::IShell* shell)
	: m_shell(shell)
	, m_eventHandler(nullptr)
	, m_wsConnection(nullptr)
	, m_listenerRunning(false)
	, m_appManager(nullptr)
	, m_windowManager(nullptr)
	, m_appManagerNotification(*this)
  , m_storageManager(nullptr)
{
	SetShell(shell);
}

RDKAMServiceHandler::~RDKAMServiceHandler()
{
	ReleaseInterfaces();
}

RDKAMServiceHandler::~RDKAMServiceHandler()
{
	StopStatusListener();
	StopEventListener();
	StopAppManagerListener();
}

void RDKAMServiceHandler::SetShell(PluginHost::IShell* shell)
{
	if (m_shell != shell) {
		ReleaseInterfaces();
	}
	m_shell = shell;
}

void RDKAMServiceHandler::SetEventHandler(IEventHandler* handler)
{
	m_eventHandler = handler;
}

void RDKAMServiceHandler::NotifyWebSocket(const std::string& url, const std::string& message)
{
	if (m_eventHandler) {
		m_eventHandler->NotifyWebSocketUpdate(url, message);
	}
}

void RDKAMServiceHandler::StartStatusListener()
{
	std::lock_guard<std::mutex> lock(m_statusMutex);

	if (!m_appsStatus) {
		// Create AppsStatus with a lambda that calls our NotifyWebSocket
		m_appsStatus = std::make_unique<AppsStatus>(
			[this](const std::string& url, const std::string& message) {
				this->NotifyWebSocket(url, message);
			}
		);

		// Set IAppManager interface if available
		if (m_appManager) {
			m_appsStatus->SetAppManager(m_appManager);
		}
	}
}

void RDKAMServiceHandler::StopStatusListener()
{
	std::lock_guard<std::mutex> lock(m_statusMutex);

	if (m_appsStatus) {
		m_appsStatus.reset();
	}
}

void RDKAMServiceHandler::ProcessThunderEvent(const std::string& eventName, const std::string& eventData)
{
	SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::ProcessThunderEvent - eventName=%s eventData=%s", eventName.c_str(), eventData.c_str())));
	
	std::lock_guard<std::mutex> lock(m_statusMutex);

	if (!m_appsStatus) {
		SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::ProcessThunderEvent - m_appsStatus is null, returning")));
		return;
	}

	// Build JSON-RPC notification format expected by AppsStatus
	Json::Value notification;
	notification["method"] = eventName;

	// Parse event data as params
	Json::CharReaderBuilder readBuilder;
	readBuilder["collectComments"] = false;
	std::unique_ptr<Json::CharReader> reader(readBuilder.newCharReader());
	std::string errors;

	if (reader->parse(eventData.c_str(), eventData.c_str() + eventData.length(), &notification["params"], &errors)) {
		Json::StreamWriterBuilder writer;
		writer["indentation"] = "";
		std::string notificationJson = Json::writeString(writer, notification);

		SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::ProcessThunderEvent - Calling HandleThunderEvent with: %s", notificationJson.c_str())));
		m_appsStatus->HandleThunderEvent(notificationJson);
	} else {
		SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::ProcessThunderEvent - Failed to parse eventData: %s", errors.c_str())));
	}
}

std::string RDKAMServiceHandler::GetAppsStatus() const
{
	std::lock_guard<std::mutex> lock(m_statusMutex);

	if (m_appsStatus) {
		return m_appsStatus->GetCurrentStatus();
	}

	return R"({"apps":[]})";
}

bool RDKAMServiceHandler::IsListenerActive() const
{
	std::lock_guard<std::mutex> lock(m_statusMutex);
	return m_appsStatus != nullptr;
}

// WebSocket event listener implementation
void RDKAMServiceHandler::StartEventListener()
{
	std::lock_guard<std::mutex> lock(m_listenerMutex);

	if (m_listenerRunning) {
		return;
	}

	m_listenerRunning = true;
	m_listenerThread = std::thread(&RDKAMServiceHandler::EventListenerThread, this);
}

void RDKAMServiceHandler::StopEventListener()
{
	{
		std::lock_guard<std::mutex> lock(m_listenerMutex);
		if (!m_listenerRunning) {
			return;
		}
		m_listenerRunning = false;
	}

	if (m_listenerThread.joinable()) {
		m_listenerThread.join();
	}
}

void RDKAMServiceHandler::EventListenerThread()
{
	// Events are received via COM-RPC callbacks through AppManagerNotification
	try {
		while (m_listenerRunning) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	} catch (const std::exception&) {
	}
}

void RDKAMServiceHandler::OnEventMessageReceived(const std::string& message)
{
	// Parse the JSON-RPC notification
	Json::CharReaderBuilder readBuilder;
	readBuilder["collectComments"] = false;
	std::unique_ptr<Json::CharReader> reader(readBuilder.newCharReader());
	Json::Value notification;
	std::string errors;

	if (!reader->parse(message.c_str(), message.c_str() + message.length(), &notification, &errors)) {
		return;
	}

	// Extract method name and params
	if (notification.isMember("method")) {
		std::string method = notification["method"].asString();

		// Extract event name from method (e.g., "RDKAMService.onAppLaunchRequest" -> "onAppLaunchRequest")
		size_t dotPos = method.find('.');
		std::string eventName = (dotPos != std::string::npos) ? method.substr(dotPos + 1) : method;

		// Get params as JSON string
		std::string eventData = "{}";
		if (notification.isMember("params")) {
			Json::StreamWriterBuilder writer;
			writer["indentation"] = "";
			eventData = Json::writeString(writer, notification["params"]);
		}

		// Route to ProcessThunderEvent
		ProcessThunderEvent(eventName, eventData);
	}
}

// AppManager RPC Listener Implementation
bool RDKAMServiceHandler::StartAppManagerListener(PluginHost::IShell* shell)
{
	SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::StartAppManagerListener - START")));
	
	if (!shell) {
		SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::StartAppManagerListener - shell is null")));
		return false;
	}

	// Query the AppManager plugin interface
	m_appManager = shell->QueryInterfaceByCallsign<Exchange::IAppManager>("org.rdk.AppManager");
	if (!m_appManager) {
		SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::StartAppManagerListener - Failed to get IAppManager interface")));
		return false;
	}

	SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::StartAppManagerListener - Got IAppManager interface")));

	// Register for notifications
	Core::hresult result = m_appManager->Register(&m_appManagerNotification);
	if (result != Core::ERROR_NONE) {
		SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::StartAppManagerListener - Register failed: %d", result)));
		m_appManager->Release();
		m_appManager = nullptr;
		return false;
	}

	SYSLOG(Logging::Startup, (_T("RDKAMServiceHandler::StartAppManagerListener - Successfully registered for AppManager notifications")));

	// Query the RDKWindowManager interface for system stats
	m_windowManager = shell->QueryInterfaceByCallsign<Exchange::IRDKWindowManager>("org.rdk.RDKWindowManager");

	// Pass IAppManager to AppsStatus if it exists
	std::lock_guard<std::mutex> lock(m_statusMutex);
	if (m_appsStatus) {
		m_appsStatus->SetAppManager(m_appManager);
	}

	return true;
}

void RDKAMServiceHandler::StopAppManagerListener()
{
	if (m_appManager) {
		m_appManager->Unregister(&m_appManagerNotification);
		m_appManager->Release();
		m_appManager = nullptr;
	}

	if (m_windowManager) {
		m_windowManager->Release();
		m_windowManager = nullptr;
	}
}

void RDKAMServiceHandler::SendAppManagerEvent(const std::string& eventName, const Json::Value& params)
{
	ProcessThunderEvent(eventName, Json::writeString(Json::StreamWriterBuilder(), params));
}

std::string RDKAMServiceHandler::LifecycleStateToString(Exchange::IAppManager::AppLifecycleState state)
{
	switch (state) {
		case Exchange::IAppManager::APP_STATE_UNKNOWN:       return "APP_STATE_UNKNOWN";
		case Exchange::IAppManager::APP_STATE_UNLOADED:      return "APP_STATE_UNLOADED";
		case Exchange::IAppManager::APP_STATE_LOADING:       return "APP_STATE_LOADING";
		case Exchange::IAppManager::APP_STATE_INITIALIZING:  return "APP_STATE_INITIALIZING";
		case Exchange::IAppManager::APP_STATE_PAUSED:        return "APP_STATE_PAUSED";
		case Exchange::IAppManager::APP_STATE_RUNNING:       return "APP_STATE_RUNNING";
		case Exchange::IAppManager::APP_STATE_ACTIVE:        return "APP_STATE_ACTIVE";
		case Exchange::IAppManager::APP_STATE_SUSPENDED:     return "APP_STATE_SUSPENDED";
		case Exchange::IAppManager::APP_STATE_HIBERNATED:    return "APP_STATE_HIBERNATED";
		case Exchange::IAppManager::APP_STATE_TERMINATING:   return "APP_STATE_TERMINATING";
		default:                                              return "UNKNOWN";
	}
}

std::string RDKAMServiceHandler::ErrorReasonToString(Exchange::IAppManager::AppErrorReason reason)
{
	switch (reason) {
		case Exchange::IAppManager::APP_ERROR_NONE:           return "APP_ERROR_NONE";
		case Exchange::IAppManager::APP_ERROR_UNKNOWN:        return "APP_ERROR_UNKNOWN";
		case Exchange::IAppManager::APP_ERROR_STATE_TIMEOUT:  return "APP_ERROR_STATE_TIMEOUT";
		case Exchange::IAppManager::APP_ERROR_ABORT:          return "APP_ERROR_ABORT";
		case Exchange::IAppManager::APP_ERROR_INVALID_PARAM:  return "APP_ERROR_INVALID_PARAM";
		case Exchange::IAppManager::APP_ERROR_CREATE_DISPLAY: return "APP_ERROR_CREATE_DISPLAY";
		case Exchange::IAppManager::APP_ERROR_DOBBY_SPEC:     return "APP_ERROR_DOBBY_SPEC";
		case Exchange::IAppManager::APP_ERROR_NOT_INSTALLED:  return "APP_ERROR_NOT_INSTALLED";
		case Exchange::IAppManager::APP_ERROR_PACKAGE_LOCK:   return "APP_ERROR_PACKAGE_LOCK";
		default:                                               return "UNKNOWN";
	}
void RDKAMServiceHandler::ReleaseInterfaces()
{
	if (m_appManager != nullptr) {
		m_appManager->Release();
		m_appManager = nullptr;
	}

	if (m_windowManager != nullptr) {
		m_windowManager->Release();
		m_windowManager = nullptr;
	}

	if (m_storageManager != nullptr) {
		m_storageManager->Release();
		m_storageManager = nullptr;
	}
}

Exchange::IAppManager* RDKAMServiceHandler::GetAppManager()
{
	if (m_appManager == nullptr && m_shell != nullptr) {
		m_appManager = m_shell->QueryInterfaceByCallsign<Exchange::IAppManager>("org.rdk.AppManager");
	}

	return m_appManager;
}

Exchange::IRDKWindowManager* RDKAMServiceHandler::GetWindowManager()
{
	if (m_windowManager == nullptr && m_shell != nullptr) {
		m_windowManager = m_shell->QueryInterfaceByCallsign<Exchange::IRDKWindowManager>("org.rdk.RDKWindowManager");
	}

	return m_windowManager;
}

Exchange::IStorageManager* RDKAMServiceHandler::GetStorageManager()
{
	if (m_storageManager == nullptr && m_shell != nullptr) {
		m_storageManager = m_shell->QueryInterfaceByCallsign<Exchange::IStorageManager>("org.rdk.StorageManager");
	}

	return m_storageManager;
}

Core::hresult RDKAMServiceHandler::DispatchMappedRequest(const std::vector<std::string>& methods,
	const std::map<std::string, std::string>& paramTemplate,
	const std::map<std::string, std::string>& runtimeParams,
	uint32_t& code, std::string& responseBody)
{
	using MethodHandler = std::function<bool(Core::hresult&)>;

	auto resolveParam = [&](const std::string& key) -> std::string {
		const auto templateIt = paramTemplate.find(key);
		if (templateIt == paramTemplate.end()) {
			const auto runtimeIt = runtimeParams.find(key);
			if (runtimeIt != runtimeParams.end()) {
				return runtimeIt->second;
			}
			return "";
		}

		const std::string& token = templateIt->second;
		if (token.size() > 2 && token.front() == '$' && token.back() == '$') {
			const std::string lookupKey = token.substr(1, token.size() - 2);
			const auto runtimeIt = runtimeParams.find(lookupKey);
			if (runtimeIt != runtimeParams.end()) {
				return runtimeIt->second;
			}
			return "";
		}

		return token;
	};

	Core::hresult status = Core::ERROR_NOT_SUPPORTED;
	bool executed = false;

	const std::map<std::string, MethodHandler> handlers = {
		{ "org.rdk.AppManager.getInstalledApps", [&](Core::hresult& outStatus) {
			outStatus = GetInstalledAppsRequest(code, responseBody);
			return true;
		} },
		{ "org.rdk.AppManager.launchApp", [&](Core::hresult& outStatus) {
			outStatus = LaunchAppRequest(
				resolveParam("appId"),
				resolveParam("token"),
				resolveParam("mode"),
				resolveParam("intent"),
				resolveParam("launchArgs"),
				code,
				responseBody);
			return true;
		} },
		{ "org.rdk.AppManager.closeApp", [&](Core::hresult& outStatus) {
			outStatus = CloseAppRequest(resolveParam("appId"), code, responseBody);
			return true;
		} },
		{ "org.rdk.AppManager.killApp", [&](Core::hresult& outStatus) {
			outStatus = KillAppRequest(resolveParam("appId"), code, responseBody);
			return true;
		} },
		{ "org.rdk.RDKWindowManager.setFocus", [&](Core::hresult& outStatus) {
			outStatus = FocusAppRequest(resolveParam("client"), code, responseBody);
			return true;
		} },
		{ "org.rdk.RDKWindowManager.getLastKeyPress", [&](Core::hresult& outStatus) {
			outStatus = GetSystemStatsRequest(code, responseBody);
			return true;
		} },
		{ "org.rdk.StorageManager.clear", [&](Core::hresult& outStatus) {
			const std::string appId = resolveParam("appId");
			if (appId.empty()) {
				return false;
			}
			outStatus = ResetAppDataRequest(appId, code, responseBody);
			return true;
		} },
		{ "org.rdk.StorageManager.clearAll", [&](Core::hresult& outStatus) {
			const std::string appId = resolveParam("appId");
			if (!appId.empty()) {
				return false;
			}
			outStatus = ResetAppDataRequest("", code, responseBody);
			return true;
		} }
	};

	for (const std::string& method : methods) {
		const auto handlerIt = handlers.find(method);
		if (handlerIt == handlers.end()) {
			continue;
		}

		Core::hresult methodStatus = Core::ERROR_NOT_SUPPORTED;
		if (!handlerIt->second(methodStatus)) {
			continue;
		}

		executed = true;
		status = methodStatus;

		if (status != Core::ERROR_NONE) {
			break;
		}
	}

	if (!executed) {
		code = 404;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("No mapped method executed");
		return Core::ERROR_NOT_SUPPORTED;
	}

	return status;
}

Core::hresult RDKAMServiceHandler::GetInstalledAppsRequest(uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IAppManager* appManager = GetAppManager();
	if (appManager == nullptr) {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("org.rdk.AppManager interface unavailable");
		return Core::ERROR_GENERAL;
	}

	std::string apps;
	Core::hresult status = appManager->GetInstalledApps(apps);

	if (status == Core::ERROR_NONE) {
		code = 200;
		responseBody = apps.empty() ? "{}" : apps;
	} else {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Failed to get installed apps");
	}

	return status;
}

Core::hresult RDKAMServiceHandler::LaunchAppRequest(const std::string& appId, const std::string& token, const std::string& mode,
													  const std::string& intent, const std::string& launchArgs,
													  uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (token == "invalid_token") {
		responseBody.clear();
		return Core::ERROR_GENERAL;
	}

	if (appId.empty()) {
		code = 400;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Missing appId");
		return Core::ERROR_INVALID_INPUT_LENGTH;
	}

	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IAppManager* appManager = GetAppManager();
	if (appManager == nullptr) {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("org.rdk.AppManager interface unavailable");
		return Core::ERROR_GENERAL;
	}

	Core::hresult status = Core::ERROR_GENERAL;
	if (mode == "background") {
		std::string preloadError;
		status = appManager->PreloadApp(appId, launchArgs, preloadError);
		if (status == Core::ERROR_NONE) {
			code = 200;
			Json::Value successResponse;
			successResponse["success"] = true;
			successResponse["appId"] = appId;
			successResponse["mode"] = mode;

			Json::StreamWriterBuilder writerBuilder;
			writerBuilder["indentation"] = "";
			responseBody = Json::writeString(writerBuilder, successResponse);
		} else {
			responseBody = RDKAMServiceUtils::BuildErrorResponse(preloadError.empty() ? "Preload failed" : preloadError);
		}
	} else {
		std::string resolvedIntent = intent;
		if (resolvedIntent.empty()) {
			resolvedIntent = "{\"action\":\"launch\",\"context\":{\"source\":\"voice\"}}";
		}
		status = appManager->LaunchApp(appId, resolvedIntent, launchArgs);
		if (status == Core::ERROR_NONE) {
			code = 200;
			Json::Value successResponse;
			successResponse["success"] = true;
			successResponse["appId"] = appId;

			Json::StreamWriterBuilder writerBuilder;
			writerBuilder["indentation"] = "";
			responseBody = Json::writeString(writerBuilder, successResponse);
		} else {
			responseBody = RDKAMServiceUtils::BuildErrorResponse("Launch failed");
		}
	}

	return status;
}

Core::hresult RDKAMServiceHandler::CloseAppRequest(const std::string& appId, uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (appId.empty()) {
		code = 400;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Missing appId");
		return Core::ERROR_INVALID_INPUT_LENGTH;
	}

	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IAppManager* appManager = GetAppManager();
	if (appManager == nullptr) {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("org.rdk.AppManager interface unavailable");
		return Core::ERROR_GENERAL;
	}

	Core::hresult status = appManager->CloseApp(appId);
	if (status == Core::ERROR_NONE) {
		code = 200;
		Json::Value successResponse;
		successResponse["success"] = true;
		successResponse["appId"] = appId;

		Json::StreamWriterBuilder writerBuilder;
		writerBuilder["indentation"] = "";
		responseBody = Json::writeString(writerBuilder, successResponse);
	} else {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Close app failed");
	}

	return status;
}

Core::hresult RDKAMServiceHandler::KillAppRequest(const std::string& appId, uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (appId.empty()) {
		code = 400;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Missing appId");
		return Core::ERROR_INVALID_INPUT_LENGTH;
	}

	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IAppManager* appManager = GetAppManager();
	if (appManager == nullptr) {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("org.rdk.AppManager interface unavailable");
		return Core::ERROR_GENERAL;
	}

	Core::hresult status = appManager->KillApp(appId);
	if (status == Core::ERROR_NONE) {
		code = 200;
		Json::Value successResponse;
		successResponse["success"] = true;
		successResponse["appId"] = appId;

		Json::StreamWriterBuilder writerBuilder;
		writerBuilder["indentation"] = "";
		responseBody = Json::writeString(writerBuilder, successResponse);
	} else {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Kill app failed");
	}

	return status;
}

Core::hresult RDKAMServiceHandler::FocusAppRequest(const std::string& client, uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (client.empty()) {
		code = 400;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Missing client/appId");
		return Core::ERROR_INVALID_INPUT_LENGTH;
	}

	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IRDKWindowManager* windowManager = GetWindowManager();
	if (windowManager == nullptr) {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("org.rdk.RDKWindowManager interface unavailable");
		return Core::ERROR_GENERAL;
	}

	Core::hresult status = windowManager->SetFocus(client);
	if (status == Core::ERROR_NONE) {
		code = 200;
		Json::Value successResponse;
		successResponse["success"] = true;
		successResponse["client"] = client;

		Json::StreamWriterBuilder writerBuilder;
		writerBuilder["indentation"] = "";
		responseBody = Json::writeString(writerBuilder, successResponse);
	} else {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Set focus failed");
	}

	return status;
}

Core::hresult RDKAMServiceHandler::GetSystemStatsRequest(uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IRDKWindowManager* windowManager = GetWindowManager();
	if (windowManager == nullptr) {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("org.rdk.RDKWindowManager interface unavailable");
		return Core::ERROR_GENERAL;
	}

	try {
		uint32_t keyCode = 0;
		uint32_t modifiers = 0;
		uint64_t timestampInSeconds = 0;
		Core::hresult status = windowManager->GetLastKeyInfo(keyCode, modifiers, timestampInSeconds);

		if (status != Core::ERROR_NONE) {
			Json::Value statsResponse;
			statsResponse["lastkeypress"] = 0;
			statsResponse["relativelastkeypress"] = 0;
			statsResponse["lastmotion"] = 0;
			statsResponse["relativelastmotion"] = 0;

			Json::StreamWriterBuilder writerBuilder;
			writerBuilder["indentation"] = "";
			responseBody = Json::writeString(writerBuilder, statsResponse);
			code = 200;
			return Core::ERROR_NONE;
		}

		const uint64_t lastKeyPressTime = timestampInSeconds * 1000;

		struct timespec ts;
		uint64_t currentSystemMs = 0;
		if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
			currentSystemMs = (static_cast<uint64_t>(ts.tv_sec) * 1000) + (ts.tv_nsec / 1000000);
		}

		uint64_t relativeLastkeypress = 0;
		if (lastKeyPressTime > 0 && currentSystemMs > lastKeyPressTime) {
			relativeLastkeypress = currentSystemMs - lastKeyPressTime;
		}

		Json::Value statsResponse;
		statsResponse["lastkeypress"] = static_cast<Json::Value::UInt64>(lastKeyPressTime);
		statsResponse["relativelastkeypress"] = static_cast<Json::Value::UInt64>(relativeLastkeypress);
		statsResponse["lastmotion"] = 0;
		statsResponse["relativelastmotion"] = 0;

		Json::StreamWriterBuilder writerBuilder;
		writerBuilder["indentation"] = "";
		responseBody = Json::writeString(writerBuilder, statsResponse);
		code = 200;
		return Core::ERROR_NONE;
	} catch (const std::exception& err) {
		Json::Value errorResponse;
		errorResponse["success"] = false;
		errorResponse["error"] = err.what();

		Json::StreamWriterBuilder writerBuilder;
		responseBody = Json::writeString(writerBuilder, errorResponse);
		code = 500;
		return Core::ERROR_GENERAL;
	}
}

Core::hresult RDKAMServiceHandler::ResetAppDataRequest(const std::string& appId, uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAMServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IStorageManager* storageManager = GetStorageManager();
	if (storageManager == nullptr) {
		responseBody = RDKAMServiceUtils::BuildErrorResponse("org.rdk.StorageManager interface unavailable");
		return Core::ERROR_GENERAL;
	}

	std::string errorReason;
	Core::hresult status = Core::ERROR_GENERAL;
	if (!appId.empty()) {
		status = storageManager->Clear(appId, errorReason);
		if (status == Core::ERROR_NONE) {
			code = 200;
			Json::Value successResponse;
			successResponse["success"] = true;
			successResponse["appId"] = appId;

			Json::StreamWriterBuilder writerBuilder;
			writerBuilder["indentation"] = "";
			responseBody = Json::writeString(writerBuilder, successResponse);
		}
	} else {
		const std::string exemptApps = "[\"epg_test_id\"]";
		status = storageManager->ClearAll(exemptApps, errorReason);
		if (status == Core::ERROR_NONE) {
			code = 200;
			Json::Value successResponse;
			successResponse["success"] = true;
			successResponse["scope"] = "all";

			Json::StreamWriterBuilder writerBuilder;
			writerBuilder["indentation"] = "";
			responseBody = Json::writeString(writerBuilder, successResponse);
		}
	}

	if (status != Core::ERROR_NONE) {
		responseBody = RDKAMServiceUtils::BuildErrorResponse(errorReason.empty() ? "Reset app data failed" : errorReason);
	}

	return status;
}

// AppManagerNotification implementation
void RDKAMServiceHandler::AppManagerNotification::OnAppInstalled(const string& appId, const string& version)
{
	SYSLOG(Logging::Startup, (_T("AppManagerNotification::OnAppInstalled called - appId=%s version=%s", appId.c_str(), version.c_str())));
	Json::Value params;
	params["appId"] = appId;
	params["version"] = version;
	_parent.SendAppManagerEvent("onAppInstalled", params);
}

void RDKAMServiceHandler::AppManagerNotification::OnAppUninstalled(const string& appId)
{
	SYSLOG(Logging::Startup, (_T("AppManagerNotification::OnAppUninstalled called - appId=%s", appId.c_str())));
	Json::Value params;
	params["appId"] = appId;
	_parent.SendAppManagerEvent("onAppUninstalled", params);
}

void RDKAMServiceHandler::AppManagerNotification::OnAppLifecycleStateChanged(
	const string& appId, const string& appInstanceId,
	const Exchange::IAppManager::AppLifecycleState newState,
	const Exchange::IAppManager::AppLifecycleState oldState,
	const Exchange::IAppManager::AppErrorReason errorReason)
{
	SYSLOG(Logging::Startup, (_T("AppManagerNotification::OnAppLifecycleStateChanged called - appId=%s state=%s", appId.c_str(), RDKAMServiceHandler::LifecycleStateToString(newState).c_str())));
	Json::Value params;
	params["appId"] = appId;
	params["appInstanceId"] = appInstanceId;
	params["newState"] = RDKAMServiceHandler::LifecycleStateToString(newState);
	params["previousState"] = RDKAMServiceHandler::LifecycleStateToString(oldState);
	params["errorReason"] = RDKAMServiceHandler::ErrorReasonToString(errorReason);
	_parent.SendAppManagerEvent("onAppLifecycleStateChanged", params);
}

void RDKAMServiceHandler::AppManagerNotification::OnAppLaunchRequest(
	const string& appId, const string& intent, const string& source)
{SYSLOG(Logging::Startup, (_T("AppManagerNotification::OnAppLaunchRequest called - appId=%s source=%s", appId.c_str(), source.c_str())));
	
	Json::Value params;
	params["appId"] = appId;
	params["intent"] = intent;
	params["source"] = source;
	_parent.SendAppManagerEvent("onAppLaunchRequest", params);
}

void RDKAMServiceHandler::AppManagerNotification::OnAppUnloaded(const string& appId, const string& appInstanceId)
{SYSLOG(Logging::Startup, (_T("AppManagerNotification::OnAppUnloaded called - appId=%s", appId.c_str())));
	
	Json::Value params;
	params["appId"] = appId;
	params["appInstanceId"] = appInstanceId;
	_parent.SendAppManagerEvent("onAppUnloaded", params);
}

} // namespace Plugin
} // namespace WPEFramework



