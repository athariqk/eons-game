#pragma once

#include <ncore/runtime/ecs_feature.h>

namespace nc {

class EcsBaseFeature : public EcsFeature {
    NCLASS( EcsBaseFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override;
};

} // namespace nc
