#include "SystemSettings.h"

#include <array>
#include <sstream>

namespace WPEFramework {
namespace Plugin {

namespace {
constexpr std::array<const char*, 6> kOrderedKeys = {
    "appcatalogueuri",
    "apps.datamigration.enable",
    "apps.enforcepin",
    "apps.softcat.enable",
    "system.devicelocation",
    "system.acceptcasting"
};

const std::map<std::string, std::string> kJsonFieldNames = {
    { "appcatalogueuri", "appCatalogueURI" },
    { "apps.datamigration.enable", "apps.datamigration.enable" },
    { "apps.enforcepin", "apps.enforcepin" },
    { "apps.softcat.enable", "apps.softcat.enable" },
    { "system.devicelocation", "system.devicelocation" },
    { "system.acceptcasting", "system.acceptcasting" }
};
}

SystemSettings& SystemSettings::Instance()
{
    static SystemSettings instance;
    return instance;
}

SystemSettings::SystemSettings()
    : m_values({
        { "appcatalogueuri", "" },
        { "apps.datamigration.enable", "false" },
        { "apps.enforcepin", "NEVER" },
        { "apps.softcat.enable", "false" },
        { "system.devicelocation", "" },
        { "system.acceptcasting", "false" }
    })
{
}

bool SystemSettings::Get(const std::string& key, std::string& value) const
{
    std::lock_guard<std::mutex> guard(m_lock);
    const auto it = m_values.find(key);
    if (it == m_values.end()) {
        return false;
    }

    value = it->second;
    return true;
}

bool SystemSettings::Set(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> guard(m_lock);
    const auto it = m_values.find(key);
    if (it == m_values.end()) {
        return false;
    }

    it->second = value;
    return true;
}

std::string SystemSettings::ToJson() const
{
    std::lock_guard<std::mutex> guard(m_lock);

    std::ostringstream json;
    json << "{\n";

    for (auto it = kOrderedKeys.begin(); it != kOrderedKeys.end(); ++it) {
        const std::string key(*it);
        const auto jsonNameIt = kJsonFieldNames.find(key);
        const auto valueIt = m_values.find(key);
        if ((jsonNameIt == kJsonFieldNames.end()) || (valueIt == m_values.end())) {
            continue;
        }

        json << "    \"" << jsonNameIt->second << "\" : \"" << valueIt->second << "\"";
        if (std::next(it) != kOrderedKeys.end()) {
            json << ",";
        }
        json << "\n";
    }

    json << "}";
    return json.str();
}

} // namespace Plugin
} // namespace WPEFramework
