#include "XCastDataProvider.h"

#include "UtilsLogging.h"
#include <interfaces/IXCast.h>
#include <plugins/plugins.h>

namespace WPEFramework {
namespace Plugin {

bool XCastDataProvider::GetAdditionalDataURL(PluginHost::IShell* service,
                                              const std::string& applicationName,
                                              const std::string& usn,
                                              XCastAdditionalData& data)
{
    data.usn = usn;
    data.dataUrl.clear();

    if (service == nullptr || applicationName.empty()) {
        LOGWARN("XCast additional data unavailable: service or applicationName is empty");
        return false;
    }

    Exchange::IXCast* xcast =
        service->QueryInterfaceByCallsign<Exchange::IXCast>("org.rdk.Xcast");
    if (xcast == nullptr) {
        LOGWARN("Unable to resolve org.rdk.Xcast through COM-RPC");
        return false;
    }

    bool success = false;
    const Core::hresult result = xcast->GetAdditionalDataURL(applicationName, data.dataUrl, success);
    xcast->Release();
    if (result != Core::ERROR_NONE || !success) {
        data.dataUrl.clear();
        LOGWARN("XCast GetAdditionalDataURL failed applicationName=%s rc=%u success=%d",
            applicationName.c_str(), static_cast<uint32_t>(result), success ? 1 : 0);
        return false;
    }

    return true;
}

} // namespace Plugin
} // namespace WPEFramework
