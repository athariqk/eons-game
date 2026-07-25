#pragma once

#include <ncore/core/rid.h>
#include <ncore/modules/video/renderer/geometry.h>

namespace nc {

struct NCAPI EcsMeshRenderer {
    RID gpu_mesh;
    RID model_buffer;
    uint32_t index_count   = 0;
    uint32_t vertex_stride = 0;
    VertexLayout vert_layout;

    NSTRUCT(
        EcsMeshRenderer,
        NC_F( EcsMeshRenderer, gpu_mesh ) NC_F( EcsMeshRenderer, model_buffer ) NC_F( EcsMeshRenderer, index_count )
            NC_F( EcsMeshRenderer, vertex_stride ) NC_F( EcsMeshRenderer, vert_layout )
    )
};

} // namespace nc
