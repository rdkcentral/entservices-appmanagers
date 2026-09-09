#include "RuntimeConfigPayload.h"

#include <core/JSON.h>

#include <cerrno>
#include <cstdlib>

namespace WPEFramework {
namespace Plugin {
namespace Utils {

struct RuntimeConfigPayload::Impl {
    JsonObject object;
};

RuntimeConfigPayload::RuntimeConfigPayload()
    : mImpl(new Impl())
{
}

RuntimeConfigPayload::~RuntimeConfigPayload() = default;
RuntimeConfigPayload::RuntimeConfigPayload(RuntimeConfigPayload&&) noexcept = default;
RuntimeConfigPayload& RuntimeConfigPayload::operator=(RuntimeConfigPayload&&) noexcept = default;

bool RuntimeConfigPayload::Parse(const std::string& payload, std::string& error)
{
    JsonObject parsed;
    if (payload.empty() || !parsed.FromString(payload)) {
        error = "runtime configuration payload is not valid JSON";
        return false;
    }
    mImpl->object = std::move(parsed);
    error.clear();
    return true;
}

bool RuntimeConfigPayload::Serialize(std::string& payload, std::string& error) const
{
    std::string serialized;
    if (!mImpl->object.ToString(serialized)) {
        error = "runtime configuration payload could not be serialized";
        return false;
    }
    payload = std::move(serialized);
    error.clear();
    return true;
}

void RuntimeConfigPayload::SetString(const std::string& key, const std::string& value)
{
    mImpl->object[key.c_str()] = value;
}

void RuntimeConfigPayload::SetBoolean(const std::string& key, bool value)
{
    mImpl->object[key.c_str()] = value;
}

void RuntimeConfigPayload::SetInteger(const std::string& key, int64_t value)
{
    mImpl->object[key.c_str()] = value;
}

void RuntimeConfigPayload::SetUnsigned(const std::string& key, uint64_t value)
{
    mImpl->object[key.c_str()] = value;
}

void RuntimeConfigPayload::SetStringArray(const std::string& key, const std::vector<std::string>& values)
{
    JsonArray array;
    for (const auto& value : values) {
        array.Add(value);
    }
    mImpl->object[key.c_str()] = array;
}

bool RuntimeConfigPayload::AppendString(const std::string& key, const std::string& value, std::string& error)
{
    JsonArray array;
    if (mImpl->object.HasLabel(key.c_str())) {
        if (mImpl->object[key.c_str()].Content() != JsonValue::type::ARRAY) {
            error = key + " must be an array of strings";
            return false;
        }
        array = mImpl->object[key.c_str()].Array();
        for (uint16_t index = 0; index < array.Length(); ++index) {
            if (array[index].Content() != JsonValue::type::STRING) {
                error = key + " must be an array of strings";
                return false;
            }
        }
    }
    array.Add(value);
    mImpl->object[key.c_str()] = array;
    error.clear();
    return true;
}

bool RuntimeConfigPayload::UpsertEnvironment(const std::string& value, std::string& error)
{
    const size_t separator = value.find('=');
    const std::string name = separator == std::string::npos ? value : value.substr(0, separator);
    JsonArray source;
    if (mImpl->object.HasLabel("envVariables")) {
        if (mImpl->object["envVariables"].Content() != JsonValue::type::ARRAY) {
            error = "envVariables must be an array of strings";
            return false;
        }
        source = mImpl->object["envVariables"].Array();
    }

    JsonArray updated;
    bool replaced = false;
    for (uint16_t index = 0; index < source.Length(); ++index) {
        if (source[index].Content() != JsonValue::type::STRING) {
            error = "envVariables must be an array of strings";
            return false;
        }
        const std::string current = source[index].String();
        const size_t currentSeparator = current.find('=');
        const std::string currentName = currentSeparator == std::string::npos ? current : current.substr(0, currentSeparator);
        if (currentName == name) {
            if (!replaced) {
                updated.Add(value);
                replaced = true;
            }
        } else {
            updated.Add(current);
        }
    }
    if (!replaced) {
        updated.Add(value);
    }
    mImpl->object["envVariables"] = updated;
    error.clear();
    return true;
}

bool RuntimeConfigPayload::GetString(const std::string& key, std::string& value, bool& present, std::string& error) const
{
    present = mImpl->object.HasLabel(key.c_str());
    if (!present) {
        return true;
    }
    if (mImpl->object[key.c_str()].Content() != JsonValue::type::STRING) {
        error = key + " must be a string";
        return false;
    }
    value = mImpl->object[key.c_str()].String();
    return true;
}

bool RuntimeConfigPayload::GetBoolean(const std::string& key, bool& value, bool& present, std::string& error) const
{
    present = mImpl->object.HasLabel(key.c_str());
    if (!present) {
        return true;
    }
    if (mImpl->object[key.c_str()].Content() != JsonValue::type::BOOLEAN) {
        error = key + " must be a boolean";
        return false;
    }
    value = mImpl->object[key.c_str()].Boolean();
    return true;
}

bool RuntimeConfigPayload::GetInteger(const std::string& key, int64_t& value, bool& present, std::string& error) const
{
    present = mImpl->object.HasLabel(key.c_str());
    if (!present) {
        return true;
    }
    const JsonValue& jsonValue = mImpl->object[key.c_str()];
    if (jsonValue.Content() != JsonValue::type::NUMBER) {
        error = key + " must be an integer";
        return false;
    }
    const std::string encoded = jsonValue.Value();
    char* end = nullptr;
    errno = 0;
    const long long parsed = std::strtoll(encoded.c_str(), &end, 10);
    if ((errno == ERANGE) || (end == encoded.c_str()) || (*end != '\0')) {
        error = key + " must be an integer";
        return false;
    }
    value = static_cast<int64_t>(parsed);
    return true;
}

bool RuntimeConfigPayload::GetUnsigned(const std::string& key, uint64_t& value, bool& present, std::string& error) const
{
    int64_t signedValue = 0;
    if (!GetInteger(key, signedValue, present, error)) {
        return false;
    }
    if (present && signedValue < 0) {
        error = key + " must be an unsigned integer";
        return false;
    }
    value = static_cast<uint64_t>(signedValue);
    return true;
}

bool RuntimeConfigPayload::GetStringArray(const std::string& key, std::vector<std::string>& values, bool& present, std::string& error) const
{
    present = mImpl->object.HasLabel(key.c_str());
    if (!present) {
        return true;
    }
    if (mImpl->object[key.c_str()].Content() != JsonValue::type::ARRAY) {
        error = key + " must be an array of strings";
        return false;
    }
    JsonArray array = mImpl->object[key.c_str()].Array();
    std::vector<std::string> parsed;
    parsed.reserve(array.Length());
    for (uint16_t index = 0; index < array.Length(); ++index) {
        if (array[index].Content() != JsonValue::type::STRING) {
            error = key + " must be an array of strings";
            return false;
        }
        parsed.emplace_back(array[index].String());
    }
    values = std::move(parsed);
    return true;
}

}
}
}
