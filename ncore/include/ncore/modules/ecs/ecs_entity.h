#pragma once

#include <cstdint>

namespace ncore {

using EcsEntityId = uint64_t;
using EcsComponentId = EcsEntityId;

inline constexpr EcsEntityId INVALID_ENTITY_ID = static_cast<EcsEntityId>(-1);

} // namespace ncore
