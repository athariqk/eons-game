#pragma once

namespace nc {

class WindowService;
class RenderService;
class ResourceService;
class InputService;

/**
 * @brief Convenience component containing video-related
 * services resolved from ServiceRegistry.
 *
 * May be used from ECS systems to interact with the engine.
 */
struct NCAPI VideoServices {
    WindowService* Window = nullptr;
    RenderService* Gfx    = nullptr;
    NSTRUCTV( VideoServices, NC_F( VideoServices, Window ), NC_F( VideoServices, Gfx ) )
};

/**
 * @brief Convenience component containing input/output-related
 * services resolved from ServiceRegistry.
 *
 * May be used from ECS systems to interact with the engine.
 */
struct NCAPI IOServices {
    ResourceService* Resources;
    InputService* Inputs;
    NSTRUCTV( IOServices, NC_F( IOServices, Resources ), NC_F( IOServices, Inputs ) )
};

} // namespace nc
