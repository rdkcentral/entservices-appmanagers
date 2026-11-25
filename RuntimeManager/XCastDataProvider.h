#pragma once

#include <string>

namespace WPEFramework {
namespace PluginHost {
class IShell;
}
namespace Plugin {

struct XCastAdditionalData {
    std::string usn;
    std::string dataUrl;
};

class XCastDataProvider {
public:
    // Uses Thunder COM-RPC to obtain the additional-data URL from org.rdk.Xcast.
    static bool GetAdditionalDataURL(PluginHost::IShell* service,
                                     const std::string& applicationName,
                                     const std::string& usn,
                                     XCastAdditionalData& data);
};

} // namespace Plugin
} // namespace WPEFramework
