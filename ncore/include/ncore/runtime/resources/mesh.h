// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/services/video/renderer/mesh_desc.h>
#include <ncore/structs/aabb.h>

#include "material.h"

namespace nc {

struct REFLECT NCAPI MeshDesc {
    constexpr static size_t MAX_LAYERS = 32;

    REFLECT BytesBuffer vertices;
    REFLECT DynamicArray<uint32_t> indices;
    REFLECT uint32_t vertex_stride           = 0;
    REFLECT uint8_t layer_count              = 0;  // (optional) number of valid entries in layers[].
    REFLECT MeshLayerDesc layers[MAX_LAYERS] = {}; // (optional) index ranges for each layer.
    REFLECT Ref<Material> material;                // the material bound to the entire geometry.
};

/**
 * @brief Mesh is a static geometrical data containing vertex attributes.
 */
class NCAPI Mesh : public IResource {
    NCLASS( Mesh, IResource )

public:
    Mesh() = default;
    Mesh( const MeshDesc& p_desc );
    ~Mesh() override;

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    RID prepare( Scene& p_scene ) override;

    void on_deserialized() override;

    Span<const MeshLayerDesc> get_layers() const;
    Span<const std::byte> get_vertices( const uint8_t p_layer_index = 0 ) const;
    Span<const uint32_t> get_indices( const uint8_t p_layer_index = 0 ) const;
    AABB get_bounds( const uint8_t p_layer_index = 0 ) const;
    size_t vertex_count( const uint8_t p_layer_index = 0 ) const;
    size_t index_count( const uint8_t p_layer_index = 0 ) const;
    uint32_t get_vertex_stride() const;

    /**
     * @brief Get the material bound to this mesh.
     */
    Ref<Material> get_material() const
    {
        return desc.material;
    }

protected:
    void release() override;

    REFLECT MeshDesc desc;

    RenderService* renderer = nullptr;
};

/**
 * @brief A primitve cube mesh.
 */
class NCAPI CubeMesh : public Mesh {
public:
    CubeMesh();
};

/**
 * @brief A primitve plane mesh.
 *
 * The plane lies on the XZ plane (normal +Y) with a unit size of 2x2 (-1..1),
 * subdivided into x_segments * z_segments quads.
 */
class NCAPI PlaneMesh : public Mesh {
public:
    PlaneMesh( uint32_t x_segments = 1, uint32_t z_segments = 1 );

private:
    MeshDesc build_mesh_desc( uint32_t x_segments, uint32_t z_segments );
};

} // namespace nc
