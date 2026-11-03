// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <cstddef>
#include <cstdint>

#include <ncore/core/collection.h>
#include <ncore/structs/aabb.h>
#include <ncore/types/typedb.h>

namespace nc {

/**
 * @brief One index range of a mesh.
 *
 * AABB is in mesh space.
 */
struct REFLECT NCAPI MeshLayerDesc {
    REFLECT uint32_t first_index = 0;
    REFLECT uint32_t index_count = 0;
    REFLECT int32_t base_vertex  = 0;
    REFLECT AABB bounds;
};

/**
 * @brief Description of a GPU mesh upload.
 *
 * Buffers are immutable once uploaded. `layers` may be empty, in which case the
 * whole index buffer is treated as a single layer.
 */
struct NCAPI MeshUploadDesc {
    Span<const std::byte> vertices;
    Span<const uint32_t> indices;
    uint32_t vertex_stride = 0;
    Span<const MeshLayerDesc> layers;
};

} // namespace nc
