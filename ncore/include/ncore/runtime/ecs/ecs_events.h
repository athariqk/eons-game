#pragma once

#include "ecs_entity.h"

namespace nc {

namespace EcsCoreEvent {
const EcsEntity OnAdd          = 300;
const EcsEntity OnRemove       = 301;
const EcsEntity OnSet          = 302;
const EcsEntity OnDelete       = 303;
const EcsEntity OnDeleteTarget = 304;
const EcsEntity OnTableCreate  = 305;
const EcsEntity OnTableDelete  = 306;

} // namespace EcsCoreEvent

} // namespace nc
