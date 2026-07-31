#pragma once

#include <ncore/core/rid.h>

namespace nc {

struct NCAPI EcsMeshInstance {
    RID source            = 0;
    RID instance            = 0;
    uint32_t instance_count = 1;

    NSTRUCT(
        EcsMeshInstance,
        NC_F( EcsMeshInstance, source ) NC_F( EcsMeshInstance, instance ) NC_F( EcsMeshInstance, instance_count )
    )
};

} // namespace nc
