#pragma once

#include <cstdint>

namespace ncore {

using EcsEntity = uint64_t;

inline constexpr EcsEntity INVALID_ENTITY_ID = static_cast<EcsEntity>(-1);

} // namespace ncore
