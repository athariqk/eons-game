// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <ncore/runtime/resources/mesh.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/servicedb.h>
#include <ncore/services/video/render_service.h>

#include "service_helpers.h"

namespace nc {

namespace {

/**
 * @brief Compute mesh bounds from raw vertex data (position = first 3 floats).
 */
AABB bounds_from_vertices( Span<const std::byte> p_vertices, uint32_t p_stride )
{
    if (p_stride < sizeof( float ) * 3 || p_vertices.size() < p_stride)
        return AABB{};

    const auto* first = reinterpret_cast<const float*>( p_vertices.data() );
    Vec3f lo( first[0], first[1], first[2] );
    Vec3f hi = lo;

    const size_t count = p_vertices.size() / p_stride;
    for (size_t i = 1; i < count; ++i) {
        const auto* v = reinterpret_cast<const float*>( p_vertices.data() + i * p_stride );
        lo            = Vec3f( std::min( lo.x, v[0] ), std::min( lo.y, v[1] ), std::min( lo.z, v[2] ) );
        hi            = Vec3f( std::max( hi.x, v[0] ), std::max( hi.y, v[1] ), std::max( hi.z, v[2] ) );
    }
    return AABB( lo, hi );
}

} // namespace

//------------------------------------------------------------------------------

Mesh::Mesh( const MeshDesc& p_desc ) : desc( p_desc )
{
    on_deserialized();
}

Mesh::~Mesh()
{
    release();
}

void Mesh::on_deserialized()
{
    // synthesize a whole-mesh layer when no mesh layer were provided.
    if (desc.layer_count == 0 && !desc.indices.empty()) {
        auto& layer       = desc.layers[0];
        layer.first_index = 0;
        layer.index_count = static_cast<uint32_t>( desc.indices.size() );
        layer.base_vertex = 0;
        layer.bounds      = bounds_from_vertices(
            Span<const std::byte>( desc.vertices.data(), desc.vertices.size() ), desc.vertex_stride
        );
        desc.layer_count = 1;
    }

    NC_ASSERT_MSG( desc.layer_count > 0, "Mesh must have at least one layer" );
}

ResourceFormatID Mesh::get_format_id() const
{
    return "nmsh";
}

size_t Mesh::get_size_bytes() const
{
    return desc.vertices.size() + desc.indices.size();
}

RID Mesh::prepare( Scene& p_scene )
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        return prepared_rid;

    RenderService* renderer_now = p_scene.get_app_ctx()->services.resolve<RenderService>();
    if (!renderer_now || desc.vertices.empty() || desc.indices.empty() || desc.vertex_stride == 0)
        return prepared_rid;

    release();

    MeshUploadDesc upload{};
    upload.vertices      = Span<const std::byte>( desc.vertices.data(), desc.vertices.size() );
    upload.indices       = Span<const uint32_t>( desc.indices.data(), desc.indices.size() );
    upload.vertex_stride = desc.vertex_stride;
    upload.layers        = Span<const MeshLayerDesc>( desc.layers, desc.layer_count );

    RID rid = renderer_now->mesh_create( upload );
    if (!rid.is_valid())
        return prepared_rid;

    renderer = renderer_now;
    set_prepared( rid, renderer_now->get_device_revision() );
    return prepared_rid;
}

void Mesh::release()
{
    if (is_prepared_alive( renderer, prepared_rid, service_revision ))
        renderer->destroy_rid( prepared_rid );
    prepared_rid = RID::INVALID;
    renderer     = nullptr;
}

Span<const MeshLayerDesc> Mesh::get_layers() const
{
    return Span<const MeshLayerDesc>( desc.layers, desc.layer_count );
}

Span<const std::byte> Mesh::get_vertices( const uint8_t p_layer_index ) const
{
    const auto& layer   = desc.layers[p_layer_index];
    const size_t offset = static_cast<size_t>( layer.base_vertex ) * desc.vertex_stride;
    NC_ASSERT( offset <= desc.vertices.size() );
    return Span<const std::byte>( desc.vertices.data() + offset, desc.vertices.size() - offset );
}

Span<const uint32_t> Mesh::get_indices( const uint8_t p_layer_index ) const
{
    const auto& layer = desc.layers[p_layer_index];
    return Span<const uint32_t>( desc.indices.data() + layer.first_index, layer.index_count );
}

AABB Mesh::get_bounds( const uint8_t p_layer_index ) const
{
    return desc.layers[p_layer_index].bounds;
}

uint32_t Mesh::get_vertex_stride() const
{
    return desc.vertex_stride;
}

size_t Mesh::vertex_count( const uint8_t p_layer_index ) const
{
    auto vertices = get_vertices( p_layer_index );
    return desc.vertex_stride ? vertices.size() / desc.vertex_stride : 0;
}

size_t Mesh::index_count( const uint8_t p_layer_index ) const
{
    return desc.layers[p_layer_index].index_count;
}

constexpr Array<Vertex3D, 24> CUBE_VERTS = {
    //  px,    py,    pz,    nx,    ny,    nz,    tx,   ty,   tz,   tw,   u,    v,    u2,   v2,   color
    Vertex3D{ -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF }
};
constexpr Array<uint32_t, 36> CUBE_INDICES = {
    // Front Face (Z = -1)
    2, 0, 1, 2, 3, 0,
    // Bottom Face (Y = -1)
    6, 4, 5, 6, 7, 4,
    // Right Face (X = +1)
    10, 8, 9, 10, 11, 8,
    // Top Face (Y = +1)
    14, 12, 13, 14, 15, 12,
    // Left Face (X = -1)
    18, 16, 17, 18, 19, 16,
    // Back Face (Z = +1)
    22, 20, 21, 22, 23, 20
};

CubeMesh::CubeMesh() :
    Mesh(
        MeshDesc{
            .vertices = DynamicArray<std::byte>(
                reinterpret_cast<std::byte const*>( CUBE_VERTS.data() ),
                reinterpret_cast<std::byte const*>( CUBE_VERTS.data() + CUBE_VERTS.size() )
            ),
            .indices       = DynamicArray<uint32_t>( CUBE_INDICES.data(), CUBE_INDICES.data() + CUBE_INDICES.size() ),
            .vertex_stride = sizeof( Vertex3D ),
            .material      = {},
        }
    )
{}

PlaneMesh::PlaneMesh( uint32_t x_segments, uint32_t z_segments ) : Mesh( build_mesh_desc( x_segments, z_segments ) ) {}

MeshDesc PlaneMesh::build_mesh_desc( uint32_t x_segments, uint32_t z_segments )
{
    NC_ASSERT_MSG( x_segments >= 1 && z_segments >= 1, "PlaneMesh segments must be >= 1" );

    const uint32_t vert_count = ( x_segments + 1 ) * ( z_segments + 1 );

    BytesBuffer vertices( static_cast<size_t>( vert_count ) * sizeof( Vertex3D ) );
    auto* verts = reinterpret_cast<Vertex3D*>( vertices.data() );

    for (uint32_t j = 0; j <= z_segments; j++) {
        for (uint32_t i = 0; i <= x_segments; i++) {
            const float x = -1.0f + 2.0f * ( static_cast<float>( i ) / static_cast<float>( x_segments ) );
            const float z = 1.0f - 2.0f * ( static_cast<float>( j ) / static_cast<float>( z_segments ) );
            const float u = static_cast<float>( i ) / static_cast<float>( x_segments );
            const float v = static_cast<float>( j ) / static_cast<float>( z_segments );

            verts[j * ( x_segments + 1 ) + i] =
                Vertex3D{ x, 0.0f, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, u, v, u, v, 0xFFFFFFFF };
        }
    }

    DynamicArray<uint32_t> indices;
    indices.reserve( static_cast<size_t>( x_segments ) * z_segments * 6 );

    for (uint32_t j = 0; j < z_segments; j++) {
        for (uint32_t i = 0; i < x_segments; i++) {
            const uint32_t v00 = static_cast<uint32_t>( j * ( x_segments + 1 ) + i );
            const uint32_t v01 = static_cast<uint32_t>( ( j + 1 ) * ( x_segments + 1 ) + i );
            const uint32_t v10 = static_cast<uint32_t>( j * ( x_segments + 1 ) + i + 1 );
            const uint32_t v11 = static_cast<uint32_t>( ( j + 1 ) * ( x_segments + 1 ) + i + 1 );

            // CCW from +Y (outward).
            indices.push_back( v00 );
            indices.push_back( v11 );
            indices.push_back( v01 );
            indices.push_back( v00 );
            indices.push_back( v10 );
            indices.push_back( v11 );
        }
    }

    return MeshDesc{
        .vertices      = std::move( vertices ),
        .indices       = std::move( indices ),
        .vertex_stride = sizeof( Vertex3D ),
        .material      = {},
    };
}

} // namespace nc
