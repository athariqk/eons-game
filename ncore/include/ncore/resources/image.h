#pragma once

#include <ncore/core/collection.h>
#include <ncore/resources/resource.h>

namespace nc {

/**
 * @brief Image contains RGBA pixel data.
 */
class NCAPI Image : public IResource {
    NCLASS( Image, IResource )

public:
    Image( int w, int h, const void* rgba_pixels );

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    uint32_t get_width() const;
    uint32_t get_height() const;
    Span<const std::byte> get_pixels() const;
    void* get_raw();

private:
    int width;
    int height;
    BytesBuffer pixels;
};

} // namespace nc
