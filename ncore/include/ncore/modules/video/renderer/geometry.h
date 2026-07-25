#pragma once

#include <span>

#include "../rhi_types.h"

namespace nc {

struct Vertex2D {
    float x, y;
    float u, v;
    uint32_t color;
};

struct Vertex3D {
    float px, py, pz;
    float nx, ny, nz;
    float tx, ty, tz, tw;
    float u, v;
    float u2, v2;
    uint32_t color;
};

struct SkinnedVertex3D {
    float px, py, pz;
    float nx, ny, nz;
    float tx, ty, tz, tw;
    float u, v;
    float u2, v2;
    uint32_t color;
    uint16_t weights[4];
    uint16_t bones[4];
};

VertexLayout get_vertex2d_layout();
VertexLayout get_vertex3d_layout();
VertexLayout get_skinned_vertex3d_layout();

VertexLayout get_vertex_layout_by_name( const std::string& name );

template<typename T>
VertexLayout get_vertex_layout_for();

template<typename TVertex>
struct Geometry {
    Vector<TVertex> vertices;
    Vector<uint16_t> indices;
    PrimitiveTopology topology = PrimitiveTopology::TRIANGLE_LIST;

    size_t vertex_count() const
    {
        return vertices.size();
    }
    size_t index_count() const
    {
        return indices.size();
    }

    std::span<const std::byte> vertex_bytes() const
    {
        return std::as_bytes( std::span{ vertices } );
    }

    std::span<const std::byte> index_bytes() const
    {
        return std::as_bytes( std::span{ indices } );
    }
};

} // namespace nc
