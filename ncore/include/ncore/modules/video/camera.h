#pragma once

#include <ncore/kernel/structures.h>

namespace ncore {

//! \brief Default 2D camera implementation
class NCORE_API Camera {
public:
    Camera();
    Camera( const Vec2& position, float zoom = 1.0f );
    ~Camera() = default;

    // ICamera interface
    Vec2 get_position() const
    {
        return position;
    }
    void set_position( const Vec2& p_position )
    {
        position = p_position;
    }

    float get_zoom() const
    {
        return zoom;
    }
    void set_zoom( float p_zoom )
    {
        zoom = p_zoom;
    }

    // Camera controls
    void move( const Vec2& delta );
    void move( float dx, float dy );

private:
    Vec2 position;
    float zoom = 1.0f;
};

} // namespace ncore
