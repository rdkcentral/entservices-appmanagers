#pragma once

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace WPEFramework {
namespace Plugin {
namespace RDKAMServiceLiteUtils {

inline std::string EscapeJsonString(const std::string& input)
{
    std::ostringstream escaped;
    escaped << std::hex;

    for (const unsigned char ch : input) {
        switch (ch) {
        case '"':  escaped << "\\\""; break;
        case '\\': escaped << "\\\\"; break;
        case '\b': escaped << "\\b";  break;
        case '\f': escaped << "\\f";  break;
        case '\n': escaped << "\\n";  break;
        case '\r': escaped << "\\r";  break;
        case '\t': escaped << "\\t";  break;
        default:
            if (ch < 0x20) {
                escaped << "\\u"
                        << std::setw(4)
                        << std::setfill('0')
                        << static_cast<int>(ch);
            } else {
                escaped << static_cast<char>(ch);
            }
            break;
        }
    }

    return escaped.str();
}

inline std::string TrimCopy(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

inline bool HasMeaningfulInput(const std::string& input)
{
    const std::string trimmed = TrimCopy(input);
    return !trimmed.empty() && trimmed != "{}";
}

inline std::string BuildStubResponse(const std::string& fields,
                                     const std::string* queryParams = nullptr,
                                     const std::string* body        = nullptr)
{
    std::ostringstream json;
    json << "{" << fields;

    bool hasEcho = false;
    if ((queryParams != nullptr) && HasMeaningfulInput(*queryParams)) {
        json << ",\"echo\":{\"queryParams\":\"" << EscapeJsonString(*queryParams) << "\"";
        hasEcho = true;
    }

    if ((body != nullptr) && HasMeaningfulInput(*body)) {
        if (!hasEcho) {
            json << ",\"echo\":{";
            hasEcho = true;
        } else {
            json << ",";
        }
        json << "\"body\":\"" << EscapeJsonString(*body) << "\"";
    }

    if (hasEcho) {
        json << "}";
    }

    json << "}";
    return json.str();
}

} // namespace RDKAMServiceLiteUtils
} // namespace Plugin
} // namespace WPEFramework
