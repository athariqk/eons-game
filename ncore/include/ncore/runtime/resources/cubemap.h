// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include "image.h"

namespace nc {

/**
 * @brief CubeMap represents cube-mapped image.
 */
class NCAPI CubeMap : public IResource {
    NCLASS( CubeMap, IResource )

public:
    CubeMap() = default;
    ~CubeMap() override;

    /**
     * @brief Convert an equirectangular image into 6 cube faces.
     *
     * Assumes the source image's top row corresponds to zenith (+Y) and the
     * horizontal center (u = 0.5) corresponds to the +X direction.
     *
     * Face order: +X, -X, +Y, -Y, +Z, -Z (Vulkan/Diligent cubemap order).
     * Output faces are RGBA8, bilinearly sampled from the source.
     *
     * @param equirect  Source equirectangular image (2:1 aspect recommended).
     * @param p_face_size Output face resolution (width == height).
     */
    CubeMap( const Ref<Image>& equirect, uint32_t p_face_size = 0 );

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    RID prepare( Scene& p_scene ) override;

    /** Output face resolution (width == height), or 0 if unset. */
    uint32_t get_face_size() const;

    /**
     * @brief Pixel data of one cube face (face_size * face_size * 4 bytes), RGBA8.
     * @param face 0..5 in +X, -X, +Y, -Y, +Z, -Z order.
     */
    Span<const std::byte> get_face_data( uint32_t face ) const;

private:
    void release() override;

    REFLECT int face_size = 0;
    // 6 faces stored contiguously (face_size * face_size * 4 bytes each).
    REFLECT BytesBuffer pixels;

    RenderService* renderer = nullptr;
};

} // namespace nc
