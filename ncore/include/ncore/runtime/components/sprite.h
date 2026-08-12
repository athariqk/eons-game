#pragma once

#include <ncore/core/color.h>
#include <ncore/core/rid.h>

namespace nc {

struct NCAPI SpriteComponent {
    RID source;
    RID texture;
    Color tint{ 255, 255, 255, 255 };

    NSTRUCTV(
        SpriteComponent, NC_F( SpriteComponent, source ) NC_F( SpriteComponent, texture ) NC_F( SpriteComponent, tint )
    )
};

struct EcsCircleDraw {
    float radius = 1.0f;
    Color color{ 0, 0, 0, 255 };
    bool filled = false;
    bool edge   = false;
};

} // namespace nc
