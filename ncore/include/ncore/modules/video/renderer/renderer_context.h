#pragma once

#include "../rhi.h"
#include "renderer_storage.h"

namespace nc {

struct CanvasRenderItem {
    RID material;
    Vector<Vertex2D> verts;
    Vector<uint16_t> indices;
    Vec4 clip = {};
};

struct RendererContext {
    IRHI* rhi;
    RendererStorage storage;
    RID gfx_device_ctx;
    Vector<CanvasRenderItem> canvas_render_list;
};

} // namespace nc
