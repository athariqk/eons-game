#pragma once

#include <ncore/runtime/ecs_feature.h>

namespace nc {

class WindowModule;
class RenderModule;

class EcsWindowFeature : public EcsFeature {
    NCLASS( EcsWindowFeature, EcsFeature )

public:
    void build( EcsWorld& world ) override;
};

} // namespace nc
