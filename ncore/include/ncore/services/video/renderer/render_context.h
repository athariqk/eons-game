#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/matrix.h>
#include <ncore/core/rect.h>
#include <ncore/core/rid.h>

#include "vertex_format.h"

namespace nc {

struct CanvasItem {
    RID material;
    RID texture;
    Rect2i clip;
    DynamicArray<Vertex2D> verts;
    DynamicArray<uint16_t> indices;
};

struct SpatialItem {
    RID material;
    RID gpu_mesh;
    Mat4 transform;
};

struct RenderContext {
    RIDPool<CanvasItem> canvas_items;
    RIDPool<SpatialItem> spatial_items;

    DynamicArray<Vertex2D> canvas_verts_staging;
    DynamicArray<uint16_t> canvas_indices_staging;

    template<class T>
    struct SortableItem {
        T* item;
        uint64_t sort_key;
    };

    struct SpatialDrawCmd : public SortableItem<SpatialItem> {
        uint32_t instancing;
        uint32_t index_count;
    };

    struct CanvasDrawCmd : public SortableItem<CanvasItem> {
        Rect2i clip_override;
        RID texture_override;
        Span<const Vertex2D> verts_override;
        Span<const uint16_t> indices_override;
        uint32_t start_vert = 0;
        uint32_t start_idx  = 0;
        uint32_t idx_count  = 0;
        uint32_t z_order    = 0; // higher draws on top; submission order is preserved within a z
    };

    // List of pending item draw calls, cleared at the end of each frame
    BumpAllocator<SpatialDrawCmd> spatial_draw_cmds{ 4096 };
    BumpAllocator<CanvasDrawCmd> canvas_draw_cmds{ 4096 };
    // Index arrays into the draw command arenas, sorted by sort_key each frame. Sorting
    // indices avoids copying whole command structs (which embed spans/strings).
    DynamicArray<uint32_t> spatial_sort_indices;
    DynamicArray<uint32_t> canvas_sort_indices;
    uint32_t canvas_override_vert_count = 0;
    uint32_t canvas_override_idx_count  = 0;
    uint32_t canvas_staging_vert_count  = 0;
    uint32_t canvas_staging_idx_count   = 0;

    // true when the current pass has a depth-stencil view bound (selects the material PSO permutation)
    bool pass_has_depth = false;

    void clear()
    {
        spatial_draw_cmds.reset();
        canvas_draw_cmds.reset();
        spatial_sort_indices.clear();
        canvas_sort_indices.clear();
        canvas_verts_staging.clear();
        canvas_indices_staging.clear();
        canvas_override_vert_count = 0;
        canvas_override_idx_count  = 0;
        canvas_staging_vert_count  = 0;
        canvas_staging_idx_count   = 0;
    }
};

} // namespace nc
