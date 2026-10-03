#pragma once

#include "ecs_entity.h"

namespace nc {

namespace EcsCoreEvent {
inline constexpr EcsEntity OnAdd          = 300;
inline constexpr EcsEntity OnRemove       = 301;
inline constexpr EcsEntity OnSet          = 302;
inline constexpr EcsEntity OnDelete       = 303;
inline constexpr EcsEntity OnDeleteTarget = 304;
inline constexpr EcsEntity OnTableCreate  = 305;
inline constexpr EcsEntity OnTableDelete  = 306;

} // namespace EcsCoreEvent

} // namespace nc
