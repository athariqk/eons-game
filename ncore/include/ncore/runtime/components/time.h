#pragma once

#include <ncore.h>

namespace nc {

struct NCAPI TimeComponent {
    uint32_t ticks     = 0;
    int frame_count    = 0;
    double fps         = 0;
    double accumulator = 0.0;

    NSTRUCTV(
        TimeComponent, NC_F( TimeComponent, ticks ) NC_F( TimeComponent, frame_count ) NC_F( TimeComponent, fps )
                           NC_F( TimeComponent, accumulator )
    )
};

} // namespace nc
