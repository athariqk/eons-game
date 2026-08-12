#pragma once

#include <ncore/core/vector.h>

namespace nc {

struct NCAPI InputComponent {
    Vec3 direction          = Vec3(); // normalized deltas of horizontal and vertical axis.
    float magnitude         = 5.0f;   // scales direction - units/s.
    Vec3 angular_delta      = Vec3(); // yaw, pitch, and roll deltas in degrees.
    float roll_rate         = 65.0f;  // deg/s.
    float mouse_sensitivity = 0.15f;  // deg/px.

    NSTRUCTV(
        InputComponent,
        NC_F( InputComponent, direction ) NC_F( InputComponent, magnitude ) NC_F( InputComponent, angular_delta )
            NC_F( InputComponent, roll_rate ) NC_F( InputComponent, mouse_sensitivity )
    )
};

} // namespace nc
