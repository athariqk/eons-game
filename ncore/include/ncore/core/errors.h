#pragma once

#include <cstdint>

namespace nc {

enum class Error : uint8_t {
    OK = 0,
    FAIL,
    FATAL,
    ERR_INVALID_PARAMETER,
    ERR_FILE_CANT_WRITE,
    COUNT
};

} // namespace nc
