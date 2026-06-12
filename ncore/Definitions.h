#pragma once

#include <array>
#include <bitset>
#include <cstdint>

#include "utils/Macro.h"
#include "modules/ecs/Component.h"

namespace ncore {

using EntityID = std::size_t;
using ComponentID = std::size_t;
using Group = std::size_t;

constexpr std::size_t max_components = 32;
constexpr std::size_t max_groups = 32;

using ComponentBitSet = std::bitset<max_components>;
using GroupBitset = std::bitset<max_groups>;
using ComponentArray = std::array<Component *, max_components>;

using ResourceID = size_t;

} // namespace ncore
