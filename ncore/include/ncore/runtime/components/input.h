#pragma once

#include <ncore/core/vector.h>

namespace nc {

struct NCAPI InputComponent {
    Vec3 Direction         = Vec3(); // normalized deltas of horizontal and vertical axis.
    float Magnitude        = 5.0f;   // scales direction - units/s.
    Vec3 AngularDelta      = Vec3(); // yaw, pitch, and roll deltas in degrees.
    float RollRate         = 65.0f;  // deg/s.
    float MouseSensitivity = 0.15f;  // deg/px.

    NSTRUCTV(
        InputComponent,
        NC_F( InputComponent, Direction ), NC_F( InputComponent, Magnitude ),
        NC_F( InputComponent, AngularDelta ), NC_F( InputComponent, RollRate ),
        NC_F( InputComponent, MouseSensitivity )
    )
};

} // namespace nc
