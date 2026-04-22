#pragma once

#include <string>

namespace RdkWindowManager {

class RdkWindowManagerEventListener {
public:
    virtual ~RdkWindowManagerEventListener() = default;
    virtual void onApplicationConnected(const std::string& client) { (void)client; }
    virtual void onApplicationDisconnected(const std::string& client) { (void)client; }
    virtual void onReady(const std::string& client) { (void)client; }
    virtual void onUserInactive(const double minutes) { (void)minutes; }
    virtual void onApplicationVisible(const std::string& client) { (void)client; }
    virtual void onApplicationHidden(const std::string& client) { (void)client; }
    virtual void onApplicationFocus(const std::string& client) { (void)client; }
    virtual void onApplicationBlur(const std::string& client) { (void)client; }
};

} // namespace RdkWindowManager
