#pragma once

#include <ncore/core/collection.h>

#include "../rhi.h"
#include "renderer_storage.h"
#include "vertex_format.h"

namespace nc {

struct CanvasRenderItem {
    RID material;
    DynArray<Vertex2D> verts;
    DynArray<uint16_t> indices;
    Rect clip = {};
};

struct WorldRenderItem {
    RID material;
    RID gpu_mesh;
    Mat4 transform;
    uint32_t count;
};

struct RendererContext {
    IRHI* rhi;               // Access to the render hardware interface
    RendererStorage storage; // Access to shared cross-renderer GPU resources
    RID gfx_device_ctx;      // The handle of current RHI device context for gfx ops
    PagedPool<WorldRenderItem> world_render_list;
    // List of pending canvas draw calls, cleared at the end of each frame
    PagedPool<CanvasRenderItem> canvas_render_list;
};

} // namespace nc
