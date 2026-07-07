#include <ncore/modules/video/resources/image.h>

namespace nc {

Image::Image( int w, int h, const void* rgba_pixels ) : width( w ), height( h )
{
    auto* p = static_cast<const std::byte*>( rgba_pixels );
    pixels.assign( p, p + static_cast<size_t>( w * h * 4 ) );
}

size_t Image::get_size_bytes()
{
    return pixels.size();
}

int Image::get_width() const
{
    return width;
}

int Image::get_height() const
{
    return height;
}

std::span<const std::byte> Image::get_pixels() const
{
    return pixels;
}

} // namespace nc
