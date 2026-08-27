#pragma once

#include <ncore/core/collection.h>

#include "../rhi.h"
#include "render_storage.h"
#include "vertex_format.h"

namespace nc {

struct CanvasRenderItem {
    RID material;
    RID texture;
    DynamicArray<Vertex2D> verts;
    DynamicArray<uint16_t> indices;
    Rect2i clip = {};
};

struct WorldRenderItem {
    RID material;
    RID gpu_mesh;
    Mat4 transform;
    uint32_t instancing;
};

struct RenderContext {
    RID gfx_device_ctx; // The handle of current RHI device context for gfx ops
    PagedPool<WorldRenderItem> world_render_list;
    // List of pending canvas draw calls, cleared at the end of each frame
    PagedPool<CanvasRenderItem> canvas_render_list;
};

} // namespace nc
