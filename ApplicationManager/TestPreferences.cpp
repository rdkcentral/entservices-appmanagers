#include "TestPreferences.h"

#include <sstream>

namespace WPEFramework {
namespace Plugin {

TestPreferences& TestPreferences::Instance()
{
    static TestPreferences instance;
    return instance;
}

TestPreferences::TestPreferences()
    : m_values({
        { "appwatchdogmode", "warn" },
        { "compositorbkgndcolor", "#000000" },
        { "compositordebuglayers", "false" },
        { "compositordebugmode", "false" },
        { "compositordebugoverlay", "false" },
        { "enableavblocking", "false" },
        { "enableirinput", "true" },
        { "enablettsreserving", "false" },
        { "epgonlylauncher", "true" },
        { "extraappenvvars", "" },
        { "forceallappslaunchable", "false" },
        { "forceallappsvisible", "false" },
        { "forcesubtitlesvisible", "false" },
        { "forcewatermarkvisible", "false" },
        { "magnifyscreenarea", "0,0,1,1" },
        { "rialtooverride", "default" },
        { "softcatdisablefetch", "true" },
        { "softcatdisableremoveapps", "false" },
        { "systemappblocklist", "" }
    })
{
}

bool TestPreferences::Get(const std::string& key, std::string& value) const
{
    std::lock_guard<std::mutex> guard(m_lock);
    const auto it = m_values.find(key);
    if (it == m_values.end()) {
        return false;
    }

    value = it->second;
    return true;
}

bool TestPreferences::Set(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> guard(m_lock);
    const auto it = m_values.find(key);
    if (it == m_values.end()) {
        return false;
    }

    it->second = value;
    return true;
}

std::string TestPreferences::ToJson() const
{
    std::lock_guard<std::mutex> guard(m_lock);

    std::ostringstream json;
    json << "{\n";

    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        json << "    \"" << it->first << "\" : \"" << it->second << "\"";
        if (std::next(it) != m_values.end()) {
            json << ",";
        }
        json << "\n";
    }

    json << "}";
    return json.str();
}

} // namespace Plugin
} // namespace WPEFramework
