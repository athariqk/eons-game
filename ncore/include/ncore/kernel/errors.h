#pragma once

#include <cstdint>

namespace ncore {

enum class Error : uint8_t { OK = 0, FAIL, FATAL };
inline constexpr uint8_t MAX_ERRORS = 3;

} // namespace ncore
