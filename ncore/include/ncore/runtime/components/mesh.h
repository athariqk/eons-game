#pragma once

#include <ncore/core/rid.h>

namespace nc {

struct NCAPI MeshComponent {
    RID Source             = 0;
    RID Instance           = 0;
    uint32_t InstanceCount = 1;

    NSTRUCTV(
        MeshComponent,
        NC_F( MeshComponent, Source ) NC_F( MeshComponent, Instance ) NC_F( MeshComponent, InstanceCount )
    )
};

} // namespace nc
