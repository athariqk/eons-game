#pragma once

#include "ecs_entity.h"

namespace nc::ecs {

/**
 * @brief Event emitted when a component ID is added to an entity.
 */
inline constexpr EcsEntity ON_ADD = 300;
/**
 * @brief Event emitted when a component ID is removed from an entity.
 */
inline constexpr EcsEntity ON_REMOVE = 301;
/**
 * @brief Event emitted when a component is set on an entity.
 *
 * This includes both adding a new component and updating an existing one.
 */
inline constexpr EcsEntity ON_SET           = 302;
inline constexpr EcsEntity ON_DELETE        = 303;
inline constexpr EcsEntity ON_DELETE_TARGET = 304;
inline constexpr EcsEntity ON_TABLE_CREATE  = 305;
inline constexpr EcsEntity ON_TABLE_DELETE  = 306;

} // namespace nc::ecs
