// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <cstddef>
#include <cstdint>

#include <ncore/core/collection.h>
#include <ncore/services/video/gfx_types.h>
#include <ncore/types/typedb.h>

namespace nc {

/**
 * @brief Description of a 2D texture upload.
 */
struct NCAPI Texture2DDesc {
    uint32_t width            = 0;
    uint32_t height           = 0;
    gfx::TextureFormat format = gfx::TextureFormat::RGBA8_UNORM;
    /// May be empty for an uninitialized (or later-updated) texture.
    Span<const std::byte> data;
    bool updatable = false;
    StringView debug_name;
};

/**
 * @brief Neutral description of a cube-map upload.
 *
 * `faces`
 */
struct NCAPI TextureCubeDesc {
    uint32_t face_size        = 0;
    gfx::TextureFormat format = gfx::TextureFormat::RGBA8_UNORM;
    /// Holds 6 contiguous faces, +X -X +Y -Y +Z -Z, each `face_size * face_size * 4` bytes of RGBA8.
    Span<const std::byte> faces;
    StringView debug_name;
};

} // namespace nc
