#include "RDKAppManagersService.h"
#include "RDKAppManagersServiceUtils.h"

#include <ctime>
#include <json/json.h>
#include <memory>

namespace WPEFramework {
namespace Plugin {

RDKAppManagersService::RDKAppManagersService(PluginHost::IShell* shell)
	: m_shell(nullptr)
	, m_appManager(nullptr)
	, m_windowManager(nullptr)
	, m_storageManager(nullptr)
{
	SetShell(shell);
}

RDKAppManagersService::~RDKAppManagersService()
{
	ReleaseInterfaces();
}

void RDKAppManagersService::SetShell(PluginHost::IShell* shell)
{
	if (m_shell != shell) {
		ReleaseInterfaces();
	}
	m_shell = shell;
}

void RDKAppManagersService::ReleaseInterfaces()
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

Exchange::IAppManager* RDKAppManagersService::GetAppManager()
{
	if (m_appManager == nullptr && m_shell != nullptr) {
		m_appManager = m_shell->QueryInterfaceByCallsign<Exchange::IAppManager>("org.rdk.AppManager");
	}

	return m_appManager;
}

Exchange::IRDKWindowManager* RDKAppManagersService::GetWindowManager()
{
	if (m_windowManager == nullptr && m_shell != nullptr) {
		m_windowManager = m_shell->QueryInterfaceByCallsign<Exchange::IRDKWindowManager>("org.rdk.RDKWindowManager");
	}

	return m_windowManager;
}

Exchange::IStorageManager* RDKAppManagersService::GetStorageManager()
{
	if (m_storageManager == nullptr && m_shell != nullptr) {
		m_storageManager = m_shell->QueryInterfaceByCallsign<Exchange::IStorageManager>("org.rdk.StorageManager");
	}

	return m_storageManager;
}

Core::hresult RDKAppManagersService::DispatchMappedRequest(const std::vector<std::string>& methods,
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
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("No mapped method executed");
		return Core::ERROR_NOT_SUPPORTED;
	}

	return status;
}

Core::hresult RDKAppManagersService::GetInstalledAppsRequest(uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IAppManager* appManager = GetAppManager();
	if (appManager == nullptr) {
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("org.rdk.AppManager interface unavailable");
		return Core::ERROR_GENERAL;
	}

	std::string apps;
	Core::hresult status = appManager->GetInstalledApps(apps);

	if (status == Core::ERROR_NONE) {
		code = 200;
		responseBody = apps.empty() ? "{}" : apps;
	} else {
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Failed to get installed apps");
	}

	return status;
}

Core::hresult RDKAppManagersService::LaunchAppRequest(const std::string& appId, const std::string& token, const std::string& mode,
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
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Missing appId");
		return Core::ERROR_INVALID_INPUT_LENGTH;
	}

	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IAppManager* appManager = GetAppManager();
	if (appManager == nullptr) {
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("org.rdk.AppManager interface unavailable");
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
			responseBody = RDKAppManagersServiceUtils::BuildErrorResponse(preloadError.empty() ? "Preload failed" : preloadError);
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
			responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Launch failed");
		}
	}

	return status;
}

Core::hresult RDKAppManagersService::CloseAppRequest(const std::string& appId, uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (appId.empty()) {
		code = 400;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Missing appId");
		return Core::ERROR_INVALID_INPUT_LENGTH;
	}

	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IAppManager* appManager = GetAppManager();
	if (appManager == nullptr) {
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("org.rdk.AppManager interface unavailable");
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
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Close app failed");
	}

	return status;
}

Core::hresult RDKAppManagersService::KillAppRequest(const std::string& appId, uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (appId.empty()) {
		code = 400;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Missing appId");
		return Core::ERROR_INVALID_INPUT_LENGTH;
	}

	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IAppManager* appManager = GetAppManager();
	if (appManager == nullptr) {
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("org.rdk.AppManager interface unavailable");
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
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Kill app failed");
	}

	return status;
}

Core::hresult RDKAppManagersService::FocusAppRequest(const std::string& client, uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (client.empty()) {
		code = 400;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Missing client/appId");
		return Core::ERROR_INVALID_INPUT_LENGTH;
	}

	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IRDKWindowManager* windowManager = GetWindowManager();
	if (windowManager == nullptr) {
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("org.rdk.RDKWindowManager interface unavailable");
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
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Set focus failed");
	}

	return status;
}

Core::hresult RDKAppManagersService::GetSystemStatsRequest(uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IRDKWindowManager* windowManager = GetWindowManager();
	if (windowManager == nullptr) {
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("org.rdk.RDKWindowManager interface unavailable");
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

Core::hresult RDKAppManagersService::ResetAppDataRequest(const std::string& appId, uint32_t& code, std::string& responseBody)
{
	code = 500;
	if (m_shell == nullptr) {
		code = 503;
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Service shell is not configured");
		return Core::ERROR_GENERAL;
	}

	Exchange::IStorageManager* storageManager = GetStorageManager();
	if (storageManager == nullptr) {
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("org.rdk.StorageManager interface unavailable");
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
		responseBody = RDKAppManagersServiceUtils::BuildErrorResponse(errorReason.empty() ? "Reset app data failed" : errorReason);
	}

	return status;
}

} // namespace Plugin
} // namespace WPEFramework

