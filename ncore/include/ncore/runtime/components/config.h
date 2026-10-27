#pragma once

#include <ncore.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/utils/config.h>

namespace nc {

/**
 * @brief Singleton component exposing the application's parsed config to ECS systems.
 */
NC_COMPONENT_API( ConfigComponent )
{
    REFLECT ConfFile* config = nullptr;
};

} // namespace nc
