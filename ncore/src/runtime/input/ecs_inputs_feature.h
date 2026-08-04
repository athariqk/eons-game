#pragma once

#include <ncore/runtime/ecs_feature.h>

namespace nc {

class EcsInputsFeature : public EcsFeature {
    NCLASS( EcsInputsFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override;
};

} // namespace nc
