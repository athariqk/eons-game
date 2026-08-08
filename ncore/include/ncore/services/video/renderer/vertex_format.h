#pragma once

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

VertexLayout get_vertex2d_layout();
VertexLayout get_vertex3d_layout();

VertexLayout get_vertex_layout_by_name( const std::string& name );

template<typename T>
VertexLayout get_vertex_layout_for();

} // namespace nc
