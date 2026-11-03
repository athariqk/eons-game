#include <ncore/runtime/resources/image.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/servicedb.h>
#include <ncore/services/video/render_service.h>

#include "service_helpers.h"

namespace nc {

Image::Image( int w, int h, const void* rgba_pixels ) : width( w ), height( h )
{
    set_dimension( w, h );
    set_data( rgba_pixels );
}

Image::~Image()
{
    release();
}

ResourceFormatID Image::get_format_id() const
{
    return "nimg";
}

size_t Image::get_size_bytes() const
{
    return pixels.size();
}

uint32_t Image::get_width() const
{
    return width;
}

uint32_t Image::get_height() const
{
    return height;
}

Span<const std::byte> Image::get_pixels() const
{
    return pixels;
}

void* Image::get_raw()
{
    return pixels.data();
}

RID Image::prepare( Scene& p_scene )
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        return prepared_rid;

    RenderService* renderer_now = p_scene.get_app_ctx()->services.resolve<RenderService>();
    if (!renderer_now || width <= 0 || height <= 0 || pixels.empty())
        return prepared_rid;

    release();

    Texture2DDesc desc{};
    desc.width      = static_cast<uint32_t>( width );
    desc.height     = static_cast<uint32_t>( height );
    desc.format     = gfx::TextureFormat::RGBA8_UNORM_SRGB;
    desc.data       = Span<const std::byte>( pixels.data(), pixels.size() );
    desc.updatable  = false;
    desc.debug_name = StringView( filepath );

    RID rid = renderer_now->texture_2d_create( desc );
    if (!rid.is_valid())
        return prepared_rid;

    renderer = renderer_now;
    set_prepared( rid, renderer_now->get_device_revision() );
    return prepared_rid;
}

void Image::release()
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        renderer->destroy_rid( prepared_rid );
    prepared_rid = RID::INVALID;
    renderer     = nullptr;
}

void Image::set_dimension( int w, int h )
{
    width  = w;
    height = h;
}

void Image::set_data( const void* rgba_pixels )
{
    NC_ASSERT( width > 0 && height > 0 );
    auto* p = static_cast<const std::byte*>( rgba_pixels );
    pixels.assign( p, p + static_cast<size_t>( width * height * 4 ) );
}

} // namespace nc
