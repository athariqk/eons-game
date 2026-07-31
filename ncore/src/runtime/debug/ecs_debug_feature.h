#pragma once

#include <ncore/runtime/ecs_feature.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

class EcsDebugFeature : public EcsFeature {
    NCLASS( EcsDebugFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override;
};

} // namespace nc
