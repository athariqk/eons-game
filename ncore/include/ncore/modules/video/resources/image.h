#pragma once

#include <ncore/kernel/collection.h>
#include <ncore/modules/resource/resource.h>

namespace nc {

class Image : public IResource {
    NCLASS( Image, IResource )

public:
    Image( int w, int h, const void* rgba_pixels );

    size_t get_size_bytes() const override;
    int get_width() const;
    int get_height() const;
    std::span<const std::byte> get_pixels() const;
    void* get_raw();

private:
    int width;
    int height;
    BytesBuffer pixels;
};

} // namespace nc
