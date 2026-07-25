#pragma once

#include <ncore/core/structures.h>
#include <ncore/runtime/ecs_feature.h>

namespace nc {

struct NCAPI RenderState {
    Vec2 display_size{};
    NSTRUCT( RenderState, NC_F( RenderState, display_size ) )
};

class EcsRenderFeature : public EcsFeature {
    NCLASS( EcsRenderFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override;
};

} // namespace nc
