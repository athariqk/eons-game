#pragma once

#include <ncore/runtime/ecs_feature.h>

namespace nc {

class EcsGraphicsFeature : public EcsFeature {
    NCLASS( EcsGraphicsFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override;
};

} // namespace nc
