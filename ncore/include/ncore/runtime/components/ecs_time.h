#pragma once

#include <ncore.h>

namespace nc {

struct NCORE_API EcsTime {
    uint32_t ticks      = 0;
    int frame_count     = 0;
    double fps          = 0;
    double accumulator  = 0.0;

    NSTRUCT(
        EcsTime, NC_F( EcsTime, ticks ) NC_F( EcsTime, frame_count ) NC_F( EcsTime, fps ) NC_F( EcsTime, accumulator )
    )
};

} // namespace nc
