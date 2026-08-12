#pragma once

#include <ncore/core/rid.h>

namespace nc {

struct NCAPI MeshComponent {
    RID source              = 0;
    RID instance            = 0;
    uint32_t instance_count = 1;

    NSTRUCTV(
        MeshComponent,
        NC_F( MeshComponent, source ) NC_F( MeshComponent, instance ) NC_F( MeshComponent, instance_count )
    )
};

} // namespace nc
