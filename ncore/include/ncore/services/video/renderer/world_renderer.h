#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/matrix.h>
#include <ncore/core/rid.h>

#include "../rhi.h"

namespace nc {

class WorldRenderer {
public:
    WorldRenderer();

    void set_context( IRHI* p_rhi )
    {
        m_rhi = p_rhi;
    }

    void submit_mesh(
        RID vertex_buffer, RID index_buffer, uint32_t index_count, RID pso, RID srb, const Mat4& model_matrix
    );

    void flush();
    void clear();

private:
    RID allocate_model_ubo();

    struct WorldDrawCmd {
        RID vertex_buffer;
        RID index_buffer;
        uint32_t index_count;
        uint32_t first_index;
        RID pso;
        RID srb;
        RID model_ubo;
        uint64_t sort_key;
    };

    IRHI* m_rhi;
    DynamicArray<WorldDrawCmd> draw_queue;
    DynamicArray<RID> model_ubo_pool;
    size_t ubo_index = 0;
};

} // namespace nc
