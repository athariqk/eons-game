#include <ncore/modules/video/viewport.h>

namespace nc {

Viewport::Viewport( float ppm ) : pixels_per_meter( ppm ) {}

Viewport::Viewport( Vec4 rect ) : view_rect( rect ) {}

void Viewport::set_rect( Vec4 rect )
{
    view_rect = rect;
}

void Viewport::set_position( float X, float Y )
{
    view_rect.X = X;
    view_rect.Y = Y;
}

void Viewport::set_size( float width, float height )
{
    view_rect.w = width;
    view_rect.h = height;
}

Vec2 Viewport::world_to_screen( const Vec2& worldPos ) const
{
    Vec2 screenPos;
    screenPos.X = ( worldPos.X - camera.get_position().X ) * pixels_per_meter * camera.get_zoom();
    screenPos.Y = ( worldPos.Y - camera.get_position().Y ) * pixels_per_meter * camera.get_zoom();
    screenPos.X += view_rect.w / 2.0f + view_rect.X;
    screenPos.Y += view_rect.h / 2.0f + view_rect.Y;
    return screenPos;
}

Vec2 Viewport::screen_to_world( const Vec2& screenPos ) const
{
    Vec2 worldPos;
    worldPos.X = ( ( screenPos.X - view_rect.X - view_rect.w / 2.0f ) / camera.get_zoom() ) / pixels_per_meter +
                 camera.get_position().X;
    worldPos.Y = ( ( screenPos.Y - view_rect.Y - view_rect.h / 2.0f ) / camera.get_zoom() ) / pixels_per_meter +
                 camera.get_position().Y;
    return worldPos;
}

bool Viewport::get_is_point_visible( const Vec2& worldPos ) const
{
    const auto screenPos = world_to_screen( worldPos );
    return screenPos.X >= view_rect.X && screenPos.X <= view_rect.X + view_rect.w && screenPos.Y >= view_rect.Y &&
           screenPos.Y <= view_rect.Y + view_rect.h;
}

bool Viewport::get_is_rect_visible( const Vec2& worldPos, const Vec2& size ) const
{
    Vec2 worldMin = screen_to_world( get_position() );
    Vec2 worldMax = screen_to_world( Vec2( view_rect.X + view_rect.w, view_rect.Y + view_rect.h ) );
    return worldPos.X + size.X >= worldMin.X && worldPos.X <= worldMax.X && worldPos.Y + size.Y >= worldMin.Y &&
           worldPos.Y <= worldMax.Y;
}

} // namespace nc
