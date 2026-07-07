#pragma once

#include <ncore/runtime/ecs_feature.h>

namespace nc {

class EcsPhysicsFeature : public EcsFeature {
    NCLASS( EcsPhysicsFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override
    {
        ( void ) world;
    }
};

} // namespace nc
