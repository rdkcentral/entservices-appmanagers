#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace WPEFramework {
namespace Plugin {
namespace Utils {

class RuntimeConfigPayload {
public:
    RuntimeConfigPayload();
    ~RuntimeConfigPayload();
    RuntimeConfigPayload(RuntimeConfigPayload&&) noexcept;
    RuntimeConfigPayload& operator=(RuntimeConfigPayload&&) noexcept;

    RuntimeConfigPayload(const RuntimeConfigPayload&) = delete;
    RuntimeConfigPayload& operator=(const RuntimeConfigPayload&) = delete;

    bool Parse(const std::string& payload, std::string& error);
    bool Serialize(std::string& payload, std::string& error) const;

    void SetString(const std::string& key, const std::string& value);
    void SetBoolean(const std::string& key, bool value);
    void SetInteger(const std::string& key, int64_t value);
    void SetUnsigned(const std::string& key, uint64_t value);
    void SetStringArray(const std::string& key, const std::vector<std::string>& values);
    bool AppendString(const std::string& key, const std::string& value, std::string& error);
    bool UpsertEnvironment(const std::string& value, std::string& error);

    bool GetString(const std::string& key, std::string& value, bool& present, std::string& error) const;
    bool GetBoolean(const std::string& key, bool& value, bool& present, std::string& error) const;
    bool GetInteger(const std::string& key, int64_t& value, bool& present, std::string& error) const;
    bool GetUnsigned(const std::string& key, uint64_t& value, bool& present, std::string& error) const;
    bool GetStringArray(const std::string& key, std::vector<std::string>& values, bool& present, std::string& error) const;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}
}
}
