/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2026 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace Utils {

// Parses a RALF metadata memory size such as "16M", "100B", "1g" or "512KB".
// Grammar: decimal digits, optionally followed by one of K, M, G or B
// (case-insensitive), optionally followed by B. Multipliers are 1024-based.
// Returns false on empty input, unknown suffix, trailing characters or overflow.
inline bool ParseMemorySize(const std::string& text, uint64_t& bytes)
{
    size_t pos = 0;
    uint64_t value = 0;
    while ((pos < text.size()) && (text[pos] >= '0') && (text[pos] <= '9')) {
        const uint64_t digit = static_cast<uint64_t>(text[pos] - '0');
        if (value > ((std::numeric_limits<uint64_t>::max() - digit) / 10)) {
            return false;
        }
        value = (value * 10) + digit;
        ++pos;
    }
    if (pos == 0) {
        return false;
    }

    uint64_t multiplier = 1;
    if (pos < text.size()) {
        switch (text[pos]) {
        case 'K': case 'k': multiplier = 1024ULL; break;
        case 'M': case 'm': multiplier = 1024ULL * 1024ULL; break;
        case 'G': case 'g': multiplier = 1024ULL * 1024ULL * 1024ULL; break;
        case 'B': case 'b': multiplier = 1; break;
        default: return false;
        }
        const bool unitIsByte = (text[pos] == 'B') || (text[pos] == 'b');
        ++pos;
        if ((pos < text.size()) && !unitIsByte && ((text[pos] == 'B') || (text[pos] == 'b'))) {
            ++pos;
        }
        if (pos != text.size()) {
            return false;
        }
    }

    if ((multiplier > 1) && (value > (std::numeric_limits<uint64_t>::max() / multiplier))) {
        return false;
    }
    bytes = value * multiplier;
    return true;
}

} // namespace Utils
