#pragma once

#include <cstdint>
#include <string>

namespace RdkWindowManager {

class RdkWindowManagerData {
public:
    RdkWindowManagerData()
        : _boolValue(false)
    {
    }

    RdkWindowManagerData(bool value)
        : _boolValue(value)
    {
    }

    RdkWindowManagerData& operator=(bool value)
    {
        _boolValue = value;
        return *this;
    }

private:
    bool _boolValue;
};

} // namespace RdkWindowManager
