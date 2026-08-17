#pragma once

namespace nc {

class WindowService;
class RenderService;
class ResourceService;
class InputService;

/**
 * @brief Convenience component containing graphics-related
 * services resolved from ServiceRegistry.
 *
 * May be used from ECS systems to interact with the engine.
 */
struct NCAPI GraphicsServices {
    WindowService* Window   = nullptr;
    RenderService* Renderer = nullptr;
    NSTRUCTV( GraphicsServices, NC_F( GraphicsServices, Window ), NC_F( GraphicsServices, Renderer ) )
};

/**
 * @brief Convenience component containing input/output-related
 * services resolved from ServiceRegistry.
 *
 * May be used from ECS systems to interact with the engine.
 */
struct NCAPI IoServices {
    ResourceService* Resources;
    InputService* Inputs;
    NSTRUCTV( IoServices, NC_F( IoServices, Resources ), NC_F( IoServices, Inputs ) )
};

} // namespace nc
