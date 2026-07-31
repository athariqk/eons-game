#pragma once

#include "../ecs_entity.h"

namespace nc {

namespace EcsCoreEvent {
const EcsEntityId OnAdd          = 300;
const EcsEntityId OnRemove       = 301;
const EcsEntityId OnSet          = 302;
const EcsEntityId OnDelete       = 303;
const EcsEntityId OnDeleteTarget = 304;
const EcsEntityId OnTableCreate  = 305;
const EcsEntityId OnTableDelete  = 306;

} // namespace EcsCoreEvent

} // namespace nc
