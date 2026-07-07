#pragma once

#include <ncore/runtime/ecs_feature.h>
#include <ncore/runtime/ecs_world.h>

namespace nc {

/**
 * @brief EcsRuntimeFeature is a default feature bundle which
 * provides core engine functionalities for the ECS runtime.
 * This contains for example: rendering, physics, and others.
 *
 * You may bypass this altogether and just load whichever
 * features you need via Scene.get_ecs().load_feature().
 */
class NCORE_API EcsRuntimeFeature : public EcsFeature {
    NCLASS( EcsRuntimeFeature, EcsFeature )

public:
    EcsRuntimeFeature() {}
    void build( EcsWorld& world ) override;
};

} // namespace nc
