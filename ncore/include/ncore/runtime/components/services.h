#pragma once

#include <ncore.h>
#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>

namespace nc {

class WindowService;
class RenderService;
class InputService;
class ResourceLoader;

/**
 * @brief Convenience component containing video-related
 * services resolved from ServiceRegistry.
 *
 * May be used from ECS systems to interact with the engine.
 */
NC_COMPONENT_API( VideoServices )
{
    REFLECT WindowService* Window   = nullptr;
    REFLECT RenderService* Renderer = nullptr;
};

/**
 * @brief Convenience component containing input/output-related
 * services resolved from ServiceRegistry.
 *
 * May be used from ECS systems to interact with the engine.
 */
NC_COMPONENT_API( IOServices )
{
    REFLECT ResourceLoader* Resources = nullptr;
    REFLECT InputService* Inputs      = nullptr;
};

} // namespace nc
