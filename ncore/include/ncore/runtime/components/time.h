#pragma once

#include <ncore.h>

namespace nc {

struct NCAPI TimeComponent {
    uint32_t Ticks     = 0;
    int FrameCount     = 0;
    double FPS         = 0;
    double Accumulator = 0.0;

    NSTRUCTV(
        TimeComponent, NC_F( TimeComponent, Ticks ), NC_F( TimeComponent, FrameCount ),
        NC_F( TimeComponent, FPS ), NC_F( TimeComponent, Accumulator )
    )
};

} // namespace nc
