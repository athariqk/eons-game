// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include "../io/resource.h"

namespace nc {

class RenderService;

/**
 * @brief Image contains RGBA pixel data.
 */
class NCAPI Image : public IResource {
    NCLASS( Image, IResource )

public:
    Image() = default;
    Image( int w, int h, const void* rgba_pixels );
    ~Image() override;

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    uint32_t get_width() const;
    uint32_t get_height() const;
    Span<const std::byte> get_pixels() const NC_LIFETIME_BOUND;
    void* get_raw() NC_LIFETIME_BOUND;

    RID prepare( Scene& p_scene ) override;

    void set_dimension( int w, int h );
    void set_data( const void* rgba_pixels );

private:
    void release() override;

    REFLECT int width  = 0;
    REFLECT int height = 0;
    REFLECT BytesBuffer pixels;

    RenderService* renderer = nullptr;
};

} // namespace nc
