#pragma once

#include <ncore/core/collection.h>
#include <ncore/resources/resource.h>

namespace nc {

class NCAPI Image : public IResource {
    NCLASS( Image, IResource )

public:
    Image( int w, int h, const void* rgba_pixels );

    size_t get_size_bytes() const override;
    uint32_t get_width() const;
    uint32_t get_height() const;
    std::span<const std::byte> get_pixels() const;
    void* get_raw();

private:
    int width;
    int height;
    BytesBuffer pixels;
};

} // namespace nc
