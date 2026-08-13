#pragma once

namespace nc {

class WindowService;
class RenderService;
class ResourceService;
class InputService;

struct NCAPI GraphicsServices {
    WindowService* Window   = nullptr;
    RenderService* Renderer = nullptr;
    NSTRUCTV( GraphicsServices, NC_F( GraphicsServices, Window ) NC_F( GraphicsServices, Renderer ) )
};

struct NCAPI IoServices {
    ResourceService* Resources;
    InputService* Inputs;
    NSTRUCTV( IoServices, NC_F( IoServices, Resources ) NC_F( IoServices, Inputs ) )
};

} // namespace nc
