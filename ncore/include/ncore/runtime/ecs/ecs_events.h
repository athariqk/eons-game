#pragma once

#include "ecs_entity.h"

namespace nc {

namespace ecs {
inline constexpr EcsEntity ON_ADD           = 300;
inline constexpr EcsEntity ON_REMOVE        = 301;
inline constexpr EcsEntity ON_SET           = 302;
inline constexpr EcsEntity ON_DELETE        = 303;
inline constexpr EcsEntity ON_DELETE_TARGET = 304;
inline constexpr EcsEntity ON_TABLE_CREATE  = 305;
inline constexpr EcsEntity ON_TABLE_DELETE  = 306;

} // namespace ecs

} // namespace nc
