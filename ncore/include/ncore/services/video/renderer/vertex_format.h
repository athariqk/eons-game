#pragma once

#include "../gfx_types.h"

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

gfx::VertexLayout get_vertex2d_layout();
gfx::VertexLayout get_vertex3d_layout();

gfx::VertexLayout get_vertex_layout_by_name( StringView name );

template<typename T>
gfx::VertexLayout get_vertex_layout_for();

} // namespace nc
