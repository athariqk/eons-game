#pragma once

#include <ncore/core/collection.h>
#include <ncore/modules/video/renderer/geometry.h>
#include <ncore/resources/resource.h>

#include "material_template.h"

namespace nc {

class Mesh : public IResource {
    NCLASS( Mesh, IResource )

public:
    Mesh() = default;

    template<typename TVertex>
    explicit Mesh( Geometry<TVertex> geometry )
    {
        set_geometry( std::move( geometry ) );
    }

    template<typename TVertex>
    void set_geometry( Geometry<TVertex> geometry )
    {
        vert_layout   = get_vertex_layout_for<TVertex>();
        topology      = geometry.topology;
        vertex_stride = static_cast<uint32_t>( sizeof( TVertex ) );

        auto vb = geometry.vertex_bytes();
        vertices.assign( vb.begin(), vb.end() );

        auto ib = geometry.index_bytes();
        indices.assign( ib.begin(), ib.end() );
    }

    struct SubMesh {
        uint32_t index_start;
        uint32_t index_count;
        Ref<MaterialTemplate> material;
    };

    void set_sub_meshes( Vector<SubMesh>&& p_sub_meshes );
    std::span<const SubMesh> get_sub_meshes() const;
    std::span<const std::byte> get_vertices() const;
    std::span<const std::byte> get_indices() const;
    const VertexLayout& get_vertex_layout() const;
    PrimitiveTopology get_topology() const;
    uint32_t get_vertex_stride() const;
    size_t vertex_count() const;
    size_t index_count() const;

private:
    BytesBuffer vertices;
    BytesBuffer indices;
    VertexLayout vert_layout;
    PrimitiveTopology topology = PrimitiveTopology::TRIANGLE_LIST;
    uint32_t vertex_stride     = 0;
    Vector<SubMesh> sub_meshes;
};

} // namespace nc
