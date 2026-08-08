#pragma once

namespace nc {

class WindowService;
class RenderService;
class ResourceService;
class InputService;

struct NCAPI GraphicsServices {
    WindowService* window   = nullptr;
    RenderService* renderer = nullptr;
    NSTRUCT( GraphicsServices, NC_F( GraphicsServices, window ) NC_F( GraphicsServices, renderer ) )
};

struct NCAPI IoServices {
    ResourceService* resources;
    InputService* inputs;
    NSTRUCT( IoServices, NC_F( IoServices, resources ) NC_F( IoServices, inputs ) )
};

} // namespace nc
