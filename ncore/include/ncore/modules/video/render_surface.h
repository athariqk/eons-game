#pragma once

#include <ncore/kernel/object.h>
#include <ncore/kernel/structures.h>

namespace nc {

/**
 * @brief IRenderSurface owns a single swap chain and drives
 * presentation for that resource.
 */
class IRenderSurface : public NcObject {
    NCLASS( IRenderSurface, NcObject )

public:
    virtual void begin_frame()                  = 0;
    virtual void end_frame()                    = 0;
    virtual void set_clear_color( Color color ) = 0;
    virtual void set_vsync( bool enabled )      = 0;
    virtual void set_render_size( Vec2 size )   = 0;
    virtual Vec2 get_surface_size() const       = 0;
};

} // namespace nc
