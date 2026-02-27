#pragma once

#include "Module.h"
#include <interfaces/IAppManager.h>
#include <interfaces/IStorageManager.h>
#include <interfaces/IRDKWindowManager.h>
#include <map>
#include <string>
#include <vector>

namespace WPEFramework {
namespace Plugin {

class RDKAppManagersService {
public:
	explicit RDKAppManagersService(PluginHost::IShell* shell = nullptr);
	~RDKAppManagersService();

	void SetShell(PluginHost::IShell* shell);

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
	void ReleaseInterfaces();
	Exchange::IAppManager* GetAppManager();
	Exchange::IRDKWindowManager* GetWindowManager();
	Exchange::IStorageManager* GetStorageManager();

	PluginHost::IShell* m_shell;
	Exchange::IAppManager* m_appManager;
	Exchange::IRDKWindowManager* m_windowManager;
	Exchange::IStorageManager* m_storageManager;
};

} // namespace Plugin
} // namespace WPEFramework

