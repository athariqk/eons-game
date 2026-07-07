#include <ncore/modules/video/camera.h>

namespace nc {

Camera::Camera() : position( 0.0f, 0.0f ), zoom( 1.0f ) {}

Camera::Camera( const Vec2& p_position, float p_zoom ) : position( p_position ), zoom( p_zoom ) {}

void Camera::move( const Vec2& delta )
{
    position.X += delta.X;
    position.Y += delta.Y;
}

void Camera::move( float dx, float dy )
{
    position.X += dx;
    position.Y += dy;
}

} // namespace nc
