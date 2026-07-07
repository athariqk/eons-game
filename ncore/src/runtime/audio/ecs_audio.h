#pragma once

#include <ncore/runtime/ecs_feature.h>

namespace nc {

class EcsAudioFeature : public EcsFeature {
    NCLASS( EcsAudioFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override
    {
        ( void ) world;
    }
};

} // namespace nc
