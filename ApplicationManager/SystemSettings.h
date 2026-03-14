#pragma once

#include <map>
#include <mutex>
#include <string>

namespace WPEFramework {
namespace Plugin {

class SystemSettings {
public:
    static SystemSettings& Instance();

    bool Get(const std::string& key, std::string& value) const;
    bool Set(const std::string& key, const std::string& value);
    std::string ToJson() const;

private:
    SystemSettings();

    mutable std::mutex m_lock;
    std::map<std::string, std::string> m_values;
};

} // namespace Plugin
} // namespace WPEFramework
